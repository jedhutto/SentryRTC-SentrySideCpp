#pragma once
#include <rtc/rtc.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <thread>
#include "sl_lidar.h" 
#include "sl_lidar_driver.h"

using std::shared_ptr;

class LidarHandler
{
public:
	LidarHandler();
	~LidarHandler();
	void stopLidar();
	void getLidarData();
	void start(shared_ptr<rtc::Channel>&);
private:
	static void setupLidar();
	std::thread lidarTask;
	static bool startLidar;
};