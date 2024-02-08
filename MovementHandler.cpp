#include "MovementHandler.h"
#include <iostream>
#include <jetgpio.h>
#include <cstdlib>
/*

motor driver pinouts

pin 32 / GPIO 12 - ENA (Right) (Hardware PWM) NOT USING HARDWARE PWM THOUGH - SOMETHING'S BROKEN
pin  7 / GPIO  4 - IN1 (Right) (Up/Down) 
pin 29 / GPIO  5 - IN2 (Right) (Up/Down) 
pin 13 / GPIO 27 - IN3 (Left)  (Up/Down) 
pin 15 / GPIO 22 - IN4 (Left)  (Up/Down) 
pin 33 / GPIO 13 - ENB (Left)  (Hardware PWM) NOT USING HARDWARE PWM THOUGH - SOMETHING'S BROKEN

*/

const unsigned int ENA_PIN = 33;
const unsigned int IN1_PIN =  29;//13
const unsigned int IN2_PIN = 31;//15
const unsigned int IN3_PIN = 13;//29
const unsigned int IN4_PIN = 15;//31
const unsigned int ENB_PIN = 32;
//const unsigned int LED     = 26;
const unsigned int DUTY = 1000000;
const unsigned int FREQUENCY = 25000;

MovementHandler::MovementHandler(int pi)
{
	this->pi = pi;//pigpio_start(NULL,NULL);
	if (pi < 0) {
		std::cout << "Error initialising GPIO" << std::endl;
	}
	
	//set_mode(pi, ENA_PIN, PI_OUTPUT);
	gpioSetMode(IN1_PIN, JET_OUTPUT);
	gpioSetMode(IN2_PIN, JET_OUTPUT);
	gpioSetMode(IN3_PIN, JET_OUTPUT);
	gpioSetMode(IN4_PIN, JET_OUTPUT);
	//set_mode(pi, ENB_PIN, PI_OUTPUT);
	gpioWrite(IN1_PIN, 0);
	gpioWrite(IN2_PIN, 0);
	gpioWrite(IN3_PIN, 0);
	gpioWrite(IN4_PIN, 0);
	gpioPWM(ENA_PIN, 0);
	gpioPWM(ENB_PIN, 0);
	gpioSetPWMfrequency(ENA_PIN, FREQUENCY);
	gpioSetPWMfrequency(ENB_PIN, FREQUENCY);

	this->read = false;
}

void MovementHandler::start()
{
	this->mhThread = std::thread(&MovementHandler::MovementLoop, std::ref(read), std::ref(ms), std::ref(pi));
	this->mhThread.detach();
}

void MovementHandler::setSignal(MessageSignal)
{
	this->read = true;
}
 
void MovementHandler::setConfiguration(MovementConfigSignal)
{
}

void MovementHandler::MovementLoop(bool& read, MovementSignal& ms, int &pi)
{
	bool leftDirection = true,
		rightDirection = true;
	int missedMessage = 0;
	while (true) {
		if (read) {
			missedMessage = 0;
			read = false;

			int16_t trackLeft = ms.trackLeft - 127;
			int16_t trackRight = ms.trackRight - 127;

			if (true) {
				if (trackLeft >= 0) {
					leftDirection = true;
				}
				else {
					leftDirection = false;
					trackLeft = abs(trackLeft);
				}


				if (leftDirection) {
					gpioWrite(IN1_PIN, 1);
					gpioWrite(IN2_PIN, 0);
				}
				else {
					gpioWrite(IN1_PIN, 0);
					gpioWrite(IN2_PIN, 1);
				}
				auto temp = (DUTY / 127) * trackLeft;
				gpioPWM(ENA_PIN, trackLeft * 2);
			}
			if (true) {
				if (trackRight >= 0) {
					rightDirection = true;
				}
				else {
					rightDirection = false;
					trackRight = abs(trackRight);
				}

				if (rightDirection) {
					gpioWrite(IN3_PIN, 1);
					gpioWrite(IN4_PIN, 0);
				}
				else {
					gpioWrite(IN3_PIN, 0);
					gpioWrite(IN4_PIN, 1);
				}
				auto temp = (DUTY / 127) * trackRight;
				gpioPWM(ENB_PIN, trackRight * 2);
			}

			if ((0 <= ms.trackLeft && ms.trackLeft <= 127) && (0 <= ms.trackRight && ms.trackRight <= 127)) {
				//TODO: Am I missing something here? Why'd I leave this blank??
			}
		}
		else {
			if (missedMessage == 60) {
				gpioPWM(ENA_PIN, 0);
				gpioPWM(ENB_PIN, 0);
			}
			missedMessage++;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
	}
}

