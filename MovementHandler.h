#pragma once
#include "MovementSignal.hpp";
#include <thread>
#include "MessageSignal.hpp"
#include "MovementConfigSignal.hpp"

class MovementHandler
{
public:
	MovementHandler(int pi);

	MovementSignal ms;
	bool read;
	std::thread mhThread;
	int pi;
	void start();
	void setSignal(MessageSignal);
	void setConfiguration(MovementConfigSignal);
private:
	static void MovementLoop(bool&, MovementSignal&, int&);
};