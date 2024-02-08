//class is used to instantiate a RPLIDAR A1M8 and retrieve data from it.

#include "LidarHandler.h";
#include <chrono>
#include <thread>

LidarHandler::LidarHandler()
{
	startLidar = false;
}

LidarHandler::~LidarHandler()
{
	startLidar = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(500);
	this->lidarTask.join();
}

void LidarHandler::start(shared_ptr<rtc::Channel> & rtcChannel)
{
	startLidar = true;
	this->lidarTask = std::thread(&LidarHandler::setupLidar, std::ref(rtcChannel), std::ref(startLidar));
	this->lidarTask.detach();
}

void LidarHandler::setupLidar()
{
	while (!startLidar) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	sl::Result<sl::IChannel*>* channel;
	sl::ILidarDriver* lidar;
	const char* opt_is_channel = NULL;
	const char* opt_channel = NULL;
	const char* opt_channel_param_first = "/dev/ttyUSB0";
	sl_u32 opt_channel_param_second = 0;
	sl_u32 baudrate = 115200;
	sl_result op_result;
	int opt_channel_type = sl::CHANNEL_TYPE_SERIALPORT;

	bool useArgcBaudrate = false;

	sl::IChannel* _channel;

	printf("Ultra simple LIDAR data grabber for SLAMTEC LIDAR.\n"
		"Version: %s\n", SL_LIDAR_SDK_VERSION);

	channel = new sl::Result<sl::IChannel*>(sl::createSerialPortChannel("/dev/ttyUSB0", 115200));
	sl::ILidarDriver* drv = *sl::createLidarDriver();
	if (!drv) {
		fprintf(stderr, "insufficient memory, exit\n");
		exit(-2);
	}
	sl_lidar_response_device_info_t devinfo;
	bool connectSuccess = false;

	_channel = (*sl::createSerialPortChannel(opt_channel_param_first, baudrate));
	if (SL_IS_OK((drv)->connect(_channel))) {
		op_result = drv->getDeviceInfo(devinfo);

		if (SL_IS_OK(op_result))
		{
			connectSuccess = true;
		}
		else {
			delete drv;
			drv = NULL;
		}
	}
	if (!connectSuccess) {
		fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n", opt_channel_param_first);
	}

	printf("SLAMTEC LIDAR S/N: ");
	for (int pos = 0; pos < 16; ++pos) {
		printf("%02X", devinfo.serialnum[pos]);
	}

	printf("\n"
		"Firmware Ver: %d.%02d\n"
		"Hardware Rev: %d\n"
		, devinfo.firmware_version >> 8
		, devinfo.firmware_version & 0xFF
		, (int)devinfo.hardware_version);

	drv->setMotorSpeed(0);
	drv->startScan(0, 1);

	while (startLidar) {
		sl_lidar_response_measurement_node_hq_t nodes[8192];
		size_t count = (int)(sizeof(nodes) / sizeof(nodes[0]));

		op_result = drv->grabScanDataHq(nodes, count);

		if (SL_IS_OK(op_result)) {
			drv->ascendScanData(nodes, count);
			//for (int pos = 0; pos < (int)count; ++pos) {
			//	/*printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
			//		(nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ? "S " : "  ",
			//		(nodes[pos].angle_z_q14 * 90.f) / 16384.f,
			//		nodes[pos].dist_mm_q2 / 4.0f,
			//		nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);*/
			//}
		}

		if (0) {
			break;
		}


	}
	drv->stop();

	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	
	drv->setMotorSpeed(0);

	if (drv) {
		delete drv;
		drv = NULL;
	}
	return;
}

void LidarHandler::stopLidar()
{
	// Stop the lidar
}

void LidarHandler::getLidarData()
{
	// Get the lidar data
}

