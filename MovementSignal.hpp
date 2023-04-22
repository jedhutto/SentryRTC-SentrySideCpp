#pragma once
#include "Signal.hpp";

class MovementSignal : Signal
{
public:
	int16_t trackLeft;
	int16_t trackRight;
	MovementSignal() {
		id = this->Message;
	}
};
