#pragma once
#include "Signal.hpp";

class MovementConfigSignal : Signal
{
public:
	MovementConfigSignal() {
		id = this->MovementConfig;
	}
};
