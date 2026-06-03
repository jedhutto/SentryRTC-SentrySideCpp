#include "MovementHandler.h"
#include <iostream>
#include <cstdlib>

MovementHandler::MovementHandler(int pi, PCA9685* motorDriver)
{
	this->pi = pi;
	this->motorDriver = motorDriver;
	if (pi < 0) {
		std::cout << "Error initialising GPIO" << std::endl;
	}

	this->read = false;
}

void MovementHandler::start()
{
	this->mhThread = std::thread(&MovementHandler::MovementLoop, std::ref(read), std::ref(ms), std::ref(pi), motorDriver);
	this->mhThread.detach();
}

void MovementHandler::setSignal(MessageSignal)
{
	this->read = true;
}
 
void MovementHandler::setConfiguration(MovementConfigSignal)
{
}

void MovementHandler::MovementLoop(bool& read, MovementSignal& ms, int &pi, PCA9685* motorDriver)
{
	const int frontLeftIn1Channel  = 6;
	const int frontLeftIn2Channel  = 7;
	const int rearLeftIn1Channel   = 5;
	const int rearLeftIn2Channel   = 4;
	const int frontRightIn1Channel = 3;
	const int frontRightIn2Channel = 2;
	const int rearRightIn1Channel  = 0;
	const int rearRightIn2Channel  = 1;

	const int maxDutyCycle = 4095;

	auto setSideOutput = [motorDriver](int firstIn1, int firstIn2, int secondIn1, int secondIn2, bool forward, int dutyCycle)
		{
			if (motorDriver == nullptr)
			{
				return;
			}

			int in1Duty = forward ? dutyCycle : 0;
			int in2Duty = forward ? 0 : dutyCycle;

			motorDriver->SetRawDutyCycle(firstIn1, in1Duty);
			motorDriver->SetRawDutyCycle(firstIn2, in2Duty);
			motorDriver->SetRawDutyCycle(secondIn1, in1Duty);
			motorDriver->SetRawDutyCycle(secondIn2, in2Duty);
		};

	auto stopAllMotors = [motorDriver, frontLeftIn1Channel, frontLeftIn2Channel, rearLeftIn1Channel, rearLeftIn2Channel, frontRightIn1Channel, frontRightIn2Channel, rearRightIn1Channel, rearRightIn2Channel]()
		{
			if (motorDriver == nullptr)
			{
				return;
			}

			motorDriver->SetRawDutyCycle(frontLeftIn1Channel, 0);
			motorDriver->SetRawDutyCycle(frontLeftIn2Channel, 0);
			motorDriver->SetRawDutyCycle(rearLeftIn1Channel, 0);
			motorDriver->SetRawDutyCycle(rearLeftIn2Channel, 0);
			motorDriver->SetRawDutyCycle(frontRightIn1Channel, 0);
			motorDriver->SetRawDutyCycle(frontRightIn2Channel, 0);
			motorDriver->SetRawDutyCycle(rearRightIn1Channel, 0);
			motorDriver->SetRawDutyCycle(rearRightIn2Channel, 0);
		};

	bool leftDirection = true,
		rightDirection = true;
	int missedMessage = 0;
	while (true) {
		if (read) {
			missedMessage = 0;
			read = false;

			int16_t trackLeft = ms.trackLeft - 127;
			int16_t trackRight = ms.trackRight - 127;

			if (trackLeft >= 0) {
				leftDirection = true;
			}
			else {
				leftDirection = false;
				trackLeft = abs(trackLeft);
			}

			int leftDutyCycle = (maxDutyCycle * trackLeft) / 127;
			setSideOutput(frontLeftIn1Channel, frontLeftIn2Channel, rearLeftIn1Channel, rearLeftIn2Channel, leftDirection, leftDutyCycle);

			if (trackRight >= 0) {
				rightDirection = true;
			}
			else {
				rightDirection = false;
				trackRight = abs(trackRight);
			}

			int rightDutyCycle = (maxDutyCycle * trackRight) / 127;
			setSideOutput(frontRightIn1Channel, frontRightIn2Channel, rearRightIn1Channel, rearRightIn2Channel, rightDirection, rightDutyCycle);
		}
		else {
			if (missedMessage == 60) {
				stopAllMotors();
			}
			missedMessage++;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
	}
}

