#pragma once
#include "Signal.hpp";

class ServoSignal : Signal
{
public:
	int16_t pulseWidth; 
	ServoSignal() {
		id = this->CameraLook;
	}
};
