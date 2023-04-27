#include "MovementHandler.h"
#include <iostream>
#include <pigpiod_if2.h>
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

const unsigned int ENA_PIN = 12;
const unsigned int IN1_PIN =  4;
const unsigned int IN2_PIN =  5;
const unsigned int IN3_PIN = 27;
const unsigned int IN4_PIN = 22;
const unsigned int ENB_PIN = 13;
const unsigned int LED     = 26;
const unsigned int DUTY = 1000000;
const unsigned int FREQUENCY = 25000;

MovementHandler::MovementHandler(int pi)
{
	this->pi = pi;//pigpio_start(NULL,NULL);
	if (pi < 0) {
		std::cout << "Error initialising GPIO" << std::endl;
	}
	
	set_mode(pi, ENA_PIN, PI_OUTPUT);
	set_mode(pi, IN1_PIN, PI_OUTPUT);
	set_mode(pi, IN2_PIN, PI_OUTPUT);
	set_mode(pi, IN3_PIN, PI_OUTPUT);
	set_mode(pi, IN4_PIN, PI_OUTPUT);
	set_mode(pi, ENB_PIN, PI_OUTPUT);

	set_PWM_dutycycle(pi, ENA_PIN, 0);
	set_PWM_dutycycle(pi, ENB_PIN, 0);
	set_PWM_frequency(pi, ENA_PIN, FREQUENCY);
	set_PWM_frequency(pi, ENB_PIN, FREQUENCY);

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
					gpio_write(pi, IN1_PIN, 1);
					gpio_write(pi, IN2_PIN, 0);
				}
				else {
					gpio_write(pi, IN1_PIN, 0);
					gpio_write(pi, IN2_PIN, 1);
				}
				auto temp = (DUTY / 127) * trackLeft;
				set_PWM_dutycycle(pi, ENA_PIN, trackLeft * 2);
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
					gpio_write(pi, IN3_PIN, 1);
					gpio_write(pi, IN4_PIN, 0);
				}
				else {
					gpio_write(pi, IN3_PIN, 0);
					gpio_write(pi, IN4_PIN, 1);
				}
				auto temp = (DUTY / 127) * trackRight;
				set_PWM_dutycycle(pi, ENB_PIN, trackRight * 2);
			}

			if ((0 <= ms.trackLeft && ms.trackLeft <= 127) && (0 <= ms.trackRight && ms.trackRight <= 127)) {
				//TODO: Am I missing something here? Why'd I leave this blank??
			}
		}
		else {
			if (missedMessage == 60) {
				set_PWM_dutycycle(pi, ENA_PIN, 0);
				set_PWM_dutycycle(pi, ENB_PIN, 0);
			}
			missedMessage++;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
	}
}

