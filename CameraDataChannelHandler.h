#pragma once
#include <rtc/rtc.hpp>
#include <thread>
//#include <opencv2/opencv.hpp>

using std::shared_ptr;
//using namespace std;

class CameraDataChannelHandler
{
public:
	CameraDataChannelHandler();
	~CameraDataChannelHandler();
	void StartCamera(shared_ptr<rtc::Track>&);
	void StopCamera();
	bool IsRunning();
	bool Disconnect();
private:
	std::thread cameraTask;
	static void StreamCameraFunction(shared_ptr<rtc::Track> track, bool &startCamera);
	bool startCamera;
};

struct EncoderSettings {
	std::string h264_level = "9";//15 for PC
	std::string h264_level_long = "5.1";//5.1 for PC
	std::string h264_profile = "1";
	std::string h264_profile_long = "constrained-baseline";
	std::string video_bitrate = "600000";
	std::string h264_i_frame_period = "40";
	std::string video_bitrate_mode = "0";
	std::string rtp_payload = "124";
	std::string rtp_pack_size = "1000";
};