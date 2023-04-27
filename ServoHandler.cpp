#include "ServoHandler.h"
#include <pigpiod_if2.h>
#include <iostream>

ServoHandler::ServoHandler(int pi, int channel, PCA9685 pca9685)
{
	this->channel = channel;
	this->pi = pi;
	this->servoDriver = pca9685;
}

ServoHandler::~ServoHandler()
{
}

void ServoHandler::start()
{
	this->mhThread = std::thread(&ServoHandler::MovementLoop, std::ref(read), std::ref(ss), std::ref(servoDriver), std::ref(channel));
	this->mhThread.detach();
}

void ServoHandler::setSignal(ServoSignal)
{
}

void ServoHandler::MovementLoop(bool& read, ServoSignal& ss, PCA9685& servoDriver, int& channel)
{
	while (true) 
	{
		if (read && 500 <= ss.pulseWidth && ss.pulseWidth <= 2500) {
			read = false;

			servoDriver.SetDutyCycle(channel, ss.pulseWidth);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
	}
}
