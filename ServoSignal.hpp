#pragma once
#include "Signal.hpp";

class ServoSignal : Signal
{
public:
	int16_t pulseWidths[5];
	ServoSignal() {
		id = this->CameraLook;
	}
};
