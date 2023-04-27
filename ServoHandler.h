#pragma once
#include "ServoSignal.hpp"
#include <thread>
#include "ServoSignal.hpp"
#include "PCA9685.h"

class ServoHandler
{
public:

	ServoSignal ss;
	bool read;
	std::thread mhThread;
	int pi;
	int channel;
	PCA9685 servoDriver;
	ServoHandler(int pi, int channel, PCA9685 pca9685);
	~ServoHandler();
	void start();
	void setSignal(ServoSignal);
private:
	static void MovementLoop(bool&, ServoSignal&, PCA9685&, int&);
};