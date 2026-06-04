#pragma once
#include "Signal.hpp"
#include <cstdint>

class VoltageDataSignal : Signal
{
public:
	float voltage;
	int64_t timestamp;

	VoltageDataSignal() {
		id = this->VoltageData;
		voltage = 0.0f;
		timestamp = 0;
	}
};
