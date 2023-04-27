#include "CameraDataChannelHandler.h"
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>
#include <gst/gst.h>
#include <glib-unix.h>
#include <gst/app/gstappsink.h>

static GMainLoop* loop;

CameraDataChannelHandler::CameraDataChannelHandler()
{
	startCamera = false;
	
}

CameraDataChannelHandler::~CameraDataChannelHandler()
{
	this->cameraTask.join();
}

void CameraDataChannelHandler::StartCamera(shared_ptr<rtc::Track> &track)
{
	startCamera = true;
	this->cameraTask = std::thread(&CameraDataChannelHandler::StreamCameraFunction, std::ref(track), std::ref(startCamera));
	this->cameraTask.detach();
}

void CameraDataChannelHandler::StopCamera()
{
	startCamera = false;
}

bool CameraDataChannelHandler::IsRunning() {
	return startCamera;
}

bool CameraDataChannelHandler::Disconnect()
{
	g_main_loop_quit(loop);
	return true;
}

#define MAPPING "/ridgerun"
#define SERVICE "12345"

gboolean signal_handler(gpointer user_data)
{
	GMainLoop* loop = (GMainLoop*)user_data;

	g_print("Interrupt received, closing...\n");
	g_main_loop_quit(loop);

	return TRUE;
}

static gboolean my_bus_callback(GstBus* bus, GstMessage* message, gpointer data)
{
	g_print("Got %s message\n", GST_MESSAGE_TYPE_NAME(message));

	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_ERROR: {
		GError* err;
		gchar* debug;

		gst_message_parse_error(message, &err, &debug);
		g_print("Error: %s\n", err->message);
		g_error_free(err);
		g_free(debug);

		g_main_loop_quit(loop);
		break;
	}
	case GST_MESSAGE_EOS:
		/* end-of-stream */
		g_main_loop_quit(loop);
		break;
	default:
		/* unhandled message */
		break;
	}
	return TRUE;
}


void CameraDataChannelHandler::StreamCameraFunction(shared_ptr<rtc::Track> track, bool& startCamera)
{
	while (!startCamera) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	try {
		//gst_debug_set_active(TRUE);
		//gst_debug_set_default_threshold(GST_LEVEL_DEBUG);
		//gst_debug_set_default_threshold(GST_LEVEL_INFO);

		GstElement* pipeline, * videosrc, * enc, * pay, * appsink, * time, * videoscale, * videorate,* queue1, * queue2;
		GstCaps* caps1, * caps2, * capsScale;

		//GMainLoop* loop;
		GstBus* bus;
		guint bus_watch_id;
		
		// init GStreamer
		gst_init(NULL, NULL);
		loop = g_main_loop_new(NULL, FALSE);

		// setup pipeline
		pipeline = gst_pipeline_new("pipeline");
		//pipeline = gst_parse_launch("gst-launch-1.0 libcamerasrc camera-name=\"/base/soc/i2c0mux/i2c@1/ov5647@36\"", NULL);
		bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
		bus_watch_id = gst_bus_add_watch(bus, my_bus_callback, NULL);
		gst_object_unref(bus);


		videosrc = gst_element_factory_make("v4l2src", "videosrc");
		if (!videosrc) {
			g_warning("Failed to create video source");
			return;
		}
		g_object_set(G_OBJECT(videosrc), "device", "/dev/video0", NULL);
		
		videorate = gst_element_factory_make("videorate","videorate");
		enc = gst_element_factory_make("v4l2h264enc", "enc");
		
		//15 for 5.1 on PC
		//9 for 3.1 on samsung
		GstStructure* controls = gst_structure_new_from_string("controls, h264_level=15, h264_profile=1, video_bitrate=600000, h264_i_frame_period=20, video_bitrate_mode=0");
		g_object_set(G_OBJECT(enc), "extra-controls", controls, NULL);
		//g_object_set(G_OBJECT(enc), "bitrate", 512, NULL);
		if (!enc) {
			g_warning("Failed to create encoder");
			return;
		}
		if (!controls) {
			g_warning("Failed to create encoder controls");
			return;
		}

		time = gst_element_factory_make("timeoverlay", "time");
		//videoscale = gst_element_factory_make("videoscale", "videoscale");
		queue1 = gst_element_factory_make("queue", "queue1");
		queue2 = gst_element_factory_make("queue", "queue2");
		pay = gst_element_factory_make("rtph264pay", "pay");
		g_object_set(G_OBJECT(pay), "pt", 123, NULL);//123 for PC, 124 for android
		g_object_set(G_OBJECT(pay), "aggregate-mode", 1, NULL);
		g_object_set(G_OBJECT(pay), "config-interval", 1, NULL);
		g_object_set(G_OBJECT(pay), "mtu", 1200, NULL);//1200

		appsink = gst_element_factory_make("appsink", "appsink");

		caps1 = gst_caps_new_simple("video/x-raw",
			"format", G_TYPE_STRING, "UYVY",//UYVY I420 
			"width", G_TYPE_INT, 640,  //640   480 720 1920 240 1296   1640
			"height", G_TYPE_INT, 480, //480   320 480 1080 160 972	 922
			"framerate", GST_TYPE_FRACTION, 20, 1,
			NULL);
		if (!caps1)
		{
			g_warning("Failed to create caps");
			return;
		} 


		gst_bin_add_many(GST_BIN(pipeline), videosrc, time, enc, pay, appsink, videorate, queue1, queue2,NULL);
		//gst_bin_add_many(GST_BIN(pipeline), videosrc, time, enc, pay, appsink,NULL);
		
		if (!gst_element_link_filtered(videosrc, videorate, caps1)) {
			g_warning("Failed to link videosrc and time, caps1!");
			return;
		}
		if (!gst_element_link(videorate, time)) {
			g_warning("Failed to link videosrc and time, caps1!");
			return;
		}

		//if (!gst_element_link_filtered(videosrc, videoscale, caps1)) {
		//	g_warning("Failed to link videosrc and videoscale, caps1!");
		//	return;
		//}
		//
		//capsScale = gst_caps_new_simple("video/x-raw",
		//	"width", G_TYPE_INT, 1296/2,  //480 720 1920 240 1296
		//	"height", G_TYPE_INT, 972/2, //320 480 1080 160 972
		//	NULL);
		//
		//if (!gst_element_link_filtered(videoscale, time, capsScale)) {
		//	g_warning("Failed to link videoscale and time, caps1!");
		//	return;
		//}

		if (!gst_element_link(time, queue1)) {
			g_warning("Failed to link time and queue!");
			return;
		}
		
		if (!gst_element_link(queue1, enc)) {
			g_warning("Failed to link queue and conv!");
			return;
		}
	
		//if (!gst_element_link(time, enc)) {
		//	g_warning("Failed to link time and enc!");
		//	return;
		//}

		caps2 = gst_caps_new_simple("video/x-h264",
			"profile", G_TYPE_STRING, "constrained-baseline",
			"level", G_TYPE_STRING, "5.1",
			NULL);

		if (!gst_element_link_filtered(enc, queue2, caps2)) {
			g_warning("Failed to link enc and pay, and caps!!");
			return;
		}
		if (!gst_element_link_filtered(queue2, pay, caps2)) {
			g_warning("Failed to link enc and pay, and caps!!");
			return;
		}
		//if (!gst_element_link_filtered(enc, pay, caps2)) {
		//	g_warning("Failed to link enc and pay, and caps!!");
		//	return;
		//}
		if (!gst_element_link(pay, appsink)) {
			g_warning("Failed to link pay and appsink!");
			return;
		}

		gst_element_set_state(pipeline, GST_STATE_PLAYING);
		
		GstAppSinkCallbacks callbacks;
		callbacks.new_sample = [](GstAppSink* appsink, gpointer user_data) -> GstFlowReturn {
			/* Get the sample from the appsink */
			GstSample* sample = gst_app_sink_pull_sample(appsink);

			/* Get the buffer from the sample */
			GstBuffer* buffer = gst_sample_get_buffer(sample);

			/* Map the buffer for reading */
			GstMapInfo info;
			gst_buffer_map(buffer, &info, GST_MAP_READ);

			/* Get the track from the user data */
			std::shared_ptr<rtc::Track> track = *(std::shared_ptr<rtc::Track>*)user_data;

			/* Send the buffer data through the track */
			if (track->isOpen()) {
				if (!track->send(reinterpret_cast<const std::byte*>(info.data), (size_t)info.size)) {
					std::cout << "track send failed" << std::endl;
				}
			}
			/* Unmap the buffer */
			gst_buffer_unmap(buffer, &info);

			/* Release the sample */
			gst_sample_unref(sample);

			return GST_FLOW_OK;
		};

		gst_app_sink_set_callbacks(((GstAppSink*)appsink), &callbacks, &track, NULL);
		
		//auto sampleThread = std::thread([&] (GstElement * appsink, shared_ptr<rtc::Track> track, GMainLoop * loop) {
		//	bool trackOpened = false;
		//	GstSample* sample;
		//	static int framecount = 0;
		//	static int tries = 0;
		//	std::cout << "pulling first sample in 3 sec" << std::endl;
		//	
		//	while (track.get() == nullptr) {
		//		std::cout << "track is null" << std::endl;
		//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		//	}

		//	while (startCamera) {
		//		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		//		g_signal_emit_by_name(appsink, "try-pull-sample", 100, &sample);
		//		if (sample) {

		//			GstBuffer* buffer = gst_sample_get_buffer(sample);

		//			// Get packet data
		//			GstMapInfo map;
		//			gst_buffer_map(buffer, &map, GST_MAP_READ);
		//			
		//			if (track->isOpen()) {
		//				if (!track->send(reinterpret_cast<const std::byte*>(map.data), (size_t)map.size)) {
		//					std::cout << "track send failed" << std::endl;
		//				}
		//			}

		//			gst_buffer_unmap(buffer, &map);
		//			gst_sample_unref(sample);
		//		}
		//		else {
		//			if (tries == 0) {
		//				tries++;
		//			}
		//		}
		//	}
		//	try {
		//		g_main_loop_quit(loop);
		//		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		//	}
		//	catch(const std::exception& e){

		//	}

		//}, std::ref(appsink), track, std::ref(loop));
		
		g_main_loop_run(loop);
		//sampleThread.join();

		// clean up
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(pipeline));
		g_source_remove(bus_watch_id);
		g_main_loop_unref(loop);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

