#pragma once
#include "MovementSignal.hpp";
#include <thread>
#include "MessageSignal.hpp"
#include "MovementConfigSignal.hpp"
#include "PCA9685.h"

class MovementHandler
{
public:
	MovementHandler(int pi, PCA9685* motorDriver);

	MovementSignal ms;
	bool read;
	std::thread mhThread;
	int pi;
	void start();
	void setSignal(MessageSignal);
	void setConfiguration(MovementConfigSignal);
private:
	PCA9685* motorDriver;
	static void MovementLoop(bool&, MovementSignal&, int&, PCA9685*);
};