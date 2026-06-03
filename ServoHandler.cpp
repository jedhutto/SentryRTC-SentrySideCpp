#include "ServoHandler.h"
#include <iostream>
#include <cstdlib>

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
	int16_t currentPulseWidth[5] = { 1500, 1250, 1250, 1250, 1250 };
	int16_t maxPulseWidthChange[5] = { 10, 15, 20, 20, 20 };
	
	while (true) {	
		for (int i = 0; i < 5; i++) {
			if (500 <= ss.pulseWidths[i] && ss.pulseWidths[i] <= 2500) {
				read = false;
				if (currentPulseWidth[i] != ss.pulseWidths[i]) {
					float diff = abs(currentPulseWidth[i] - ss.pulseWidths[i]);
					if (diff > maxPulseWidthChange[i]) {
						if (currentPulseWidth[i] < ss.pulseWidths[i]) {
							currentPulseWidth[i] += maxPulseWidthChange[i];
						}
						else {
							currentPulseWidth[i] -= maxPulseWidthChange[i];
						}
					}
					else {
						currentPulseWidth[i] = ss.pulseWidths[i];
					}
					servoDriver.SetDutyCycle(i, currentPulseWidth[i]);
				}
			}
		}
		//if (500 <= ss.pulseWidth0 && ss.pulseWidth0 <= 2500) {
		//	read = false;
		//	if (currentPulseWidth[0] != ss.pulseWidth0){
		//		float diff = abs(currentPulseWidth[0] - ss.pulseWidth0);
		//		if (diff > maxPulseWidthChange[0]) {
		//			if (currentPulseWidth[0] < ss.pulseWidth0) {
		//				currentPulseWidth[0] += maxPulseWidthChange[0];
		//			}
		//			else {
		//				currentPulseWidth[0] -= maxPulseWidthChange[0];
		//			}
		//		}
		//		else {
		//			currentPulseWidth[0] = ss.pulseWidth0;
		//		}
		//		servoDriver.SetDutyCycle(channel, currentPulseWidth[0]);
		//	}
		//}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
	}
}
