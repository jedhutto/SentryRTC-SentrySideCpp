//class is used to instantiate a RPLIDAR A1M8 and retrieve data from it.

#include "LidarHandler.h";
#include <chrono>
#include <thread>
#define PI 3.14159265
LidarHandler::LidarHandler()
{
	startLidar = false;
}

LidarHandler::~LidarHandler()
{
	startLidar = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	this->lidarTask.join();
}

void LidarHandler::start(shared_ptr<rtc::DataChannel> & dataChannel)
{
	startLidar = true;
	this->lidarTask = std::thread(&LidarHandler::setupLidar, std::ref(dataChannel), std::ref(startLidar));
	this->lidarTask.detach();
}

void LidarHandler::setupLidar(shared_ptr<rtc::DataChannel> dataChannel, bool& startLidar)
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
	sl::LidarScanMode current_scan_mode;
	drv->startScan(0, 1, 0, &current_scan_mode);
	
	size_t nodeCount = 8192;
	LidarDataSignal lidarData = LidarDataSignal();
	sl_lidar_response_measurement_node_hq_t rawLidarData[nodeCount];
	size_t test = sizeof(lidarData);
	std::chrono::milliseconds(10000);
	while (startLidar) {
		op_result = drv->grabScanDataHq(rawLidarData, nodeCount);
		int start_node = 0, end_node = 0;
		int i = 0;

		if (SL_IS_OK(op_result) && dataChannel->isOpen()) {
			int validCount = convertRawDataToCoordinates(rawLidarData, lidarData.LidarData, nodeCount);
			dataChannel->send(reinterpret_cast<const std::byte*>(&lidarData), test - sizeof(lidarData.LidarData[0]) * validCount);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
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

int LidarHandler::convertRawDataToCoordinates(sl_lidar_response_measurement_node_hq_t* node, LidarDataCoordinate* coords, int count)
{
	int i = 0, 
		skipped = 0;
	while (i < count) {
		if (node[i].dist_mm_q2 == 0) {
			skipped++;
			i++;
			continue;
		}
		coords[i - skipped].x = -1.0 * (node[i].dist_mm_q2 * cos((((node[i].angle_z_q14) / (16384.0*4)) * 360.0 + 142.5) * PI / 180.0));
		coords[i - skipped].y = (node[i].dist_mm_q2 * sin((((node[i].angle_z_q14) / (16384.0*4)) * 360.0 + 142.5) * PI / 180.0));

		coords[i - skipped].isEnd = false;
		i++;
	}
	coords[i - skipped - 1].isEnd = true;
	return i - skipped;
}

void LidarHandler::stopLidar()
{
}

void LidarHandler::getLidarData()
{
}

