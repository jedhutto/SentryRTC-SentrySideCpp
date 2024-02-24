#include "CameraDataChannelHandler.h"
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>
#include <gst/gst.h>
#include <glib-unix.h>
#include <gst/app/gstappsink.h>

static GMainLoop* loop = nullptr;
static GstPipeline* gst_pipeline = nullptr;

CameraDataChannelHandler::CameraDataChannelHandler()
{
	startCamera = false;
	
}

CameraDataChannelHandler::~CameraDataChannelHandler()
{
	//startCamera = false;
	//this->cameraTask.join();
	loop = nullptr;
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

static void appsink_eos(GstAppSink* appsink, gpointer user_data)
{
	printf("app sink receive eos\n");
}

static GstFlowReturn new_buffer(GstAppSink* appsink, gpointer user_data)
{
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
}

void CameraDataChannelHandler::StreamCameraFunction(shared_ptr<rtc::Track> track, bool& startCamera)
{
	while (!startCamera) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	std::string pipelineString = "nvarguscamerasrc name=mysource ! video/x-raw(memory:NVMM),width=3280,height=2464,framerate=21/1,format=NV12 ! nvvidconv flip-method=2 ! video/x-raw(memory:NVMM),width=820,height=616,framerate=21/1,format=NV12 ! nvv4l2vp8enc preset-level=4 bitrate=1000000 ! rtpvp8pay pt=127 mtu=1200 ! appsink drop=1 max-buffers=4 name=appsink";

	system("systemctl restart nvargus-daemon");
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	try {
		gst_init(NULL, NULL);
		loop = g_main_loop_new(NULL, FALSE);

		//gst_debug_set_active(TRUE);
		//gst_debug_set_default_threshold(GST_LEVEL_DEBUG);
		GstElement * appsink;
		GstAppSinkCallbacks callbacks = { appsink_eos, NULL, new_buffer };

		
		GError* err = nullptr;
		gst_pipeline = (GstPipeline*)gst_parse_launch(pipelineString.c_str(), &err);
		if (err) {
			std::cout << std::endl << err->message << std::endl << std::endl;
			throw;
		}

		appsink = gst_bin_get_by_name(GST_BIN (gst_pipeline), "appsink");

		gst_app_sink_set_callbacks((GST_APP_SINK(appsink)), &callbacks, &track, NULL);

		gst_element_set_state((GstElement*)gst_pipeline, GST_STATE_PLAYING);
		g_main_loop_run(loop);

		// clean up
		gst_element_set_state((GstElement*)gst_pipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(gst_pipeline));
		//g_main_loop_unref(loop);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

