#include "ServoHandler.h"

ServoHandler::ServoHandler(int channel)
{
	this->channel = channel;
	pi = pigpio_start(NULL, NULL);
	servoDriver = PCA9685(pi);
	servoDriver.SetFrequency(50.0);
}

ServoHandler::~ServoHandler()
{
	servoDriver.~PCA9685();
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


		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 30));
	}
}
