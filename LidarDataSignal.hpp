#pragma once
#include "Signal.hpp";
#include "sl_lidar.h" 
#include <rtc/rtc.hpp>

typedef struct LidarDataCoordinate {
	float x;
	float y;
	bool isEnd = false;
} LidarDataCoordinate;

class LidarDataSignal : Signal
{
public:
	LidarDataCoordinate LidarData[8192];
	LidarDataSignal() {
		id = this->LidarDataArray;
	}
};