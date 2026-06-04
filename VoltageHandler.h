#pragma once
#include <rtc/rtc.hpp>
#include <thread>
#include "VoltageDataSignal.hpp"
#include "ADS1256.h"

using std::shared_ptr;

class VoltageHandler
{
public:
	VoltageHandler(int pi, ADS1256& adc, uint8_t channel = 0);
	~VoltageHandler();
	void start(shared_ptr<rtc::DataChannel>&);

private:
	static void monitorVoltage(shared_ptr<rtc::DataChannel> dataChannel, bool& isRunning, ADS1256* adc, uint8_t channel);

	int pi;
	ADS1256* adc;
	uint8_t channel;
	std::thread voltageThread;
	bool isRunning;
};
