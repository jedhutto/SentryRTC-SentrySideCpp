#pragma once
#include "Signal.hpp";

class ServoSignal : Signal
{
public:
	int16_t pulseWidth; 
	int16_t channel; 
	ServoSignal() {
		id = this->CameraLook;
	}
};
