#pragma once
#include <pigpiod_if2.h>

class PCA9685
{
public:
	PCA9685(int pi, int bus = 1, uint8_t address = 0x40, int channel = 0);
	~PCA9685();
	bool SetFrequency(double frequency);
	float GetFrequency();
	bool SetDutyCycle(int channel, int usec);
	bool SetDutyCyclePercent(int channel, float percent);
	bool SetPulseWidth(int channel, float width);
	bool Cancel();
	int WriteReg(int reg, uint8_t byte);
	uint8_t ReadReg(int reg);
private:
	uint8_t MODE1 = 0x00;
	uint8_t MODE2 = 0x01;
	uint8_t SUBADR1 = 0x02;
	uint8_t SUBADR2 = 0x03;
	uint8_t SUBADR3 = 0x04;
	uint8_t PRESCALE = 0xFE;
	uint8_t LED0_ON_L = 0x06;
	uint8_t LED0_ON_H = 0x07;
	uint8_t LED0_OFF_L = 0x08;
	uint8_t LED0_OFF_H = 0x09;
	uint8_t ALL_LED_ON_L = 0xFA;
	uint8_t ALL_LED_ON_H = 0xFB;
	uint8_t ALL_LED_OFF_L = 0xFC;
	uint8_t ALL_LED_OFF_H = 0xFD;

	uint8_t RESTART = 1 << 7;
	uint8_t AI = 1 << 5;
	uint8_t SLEEP = 1 << 4;
	uint8_t ALLCALL = 1 << 0;

	uint8_t OCH = 1 << 3;
	uint8_t OUTDRV = 1 << 2;

	double frequency;
	float pulseWidth;

	int pi;
	int bus;
	uint8_t address;
	unsigned int handle;
	int channel;
};