#pragma once
#include <rtc/rtc.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <thread>
#include "sl_lidar.h" 
#include "sl_lidar_driver.h"
#include "LidarDataSignal.hpp"
#include <cmath>

using std::shared_ptr;

class LidarHandler
{
public:
	LidarHandler();
	~LidarHandler();
	void stopLidar();
	void getLidarData();
	void start(shared_ptr<rtc::DataChannel>&);
private:
	static void setupLidar(shared_ptr<rtc::DataChannel> track, bool& startLidar, sl::ILidarDriver* driver);
	static int convertRawDataToCoordinates(sl_lidar_response_measurement_node_hq_t* node, LidarDataCoordinate* coords, int count);
	inline static sl::ILidarDriver* driver = NULL;
	std::thread lidarTask;
	bool startLidar;
};