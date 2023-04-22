#pragma once
#include "Signal.hpp";

class MessageSignal : Signal
{
public:
	MessageSignal() {
		id = this->Message;
	}
};
