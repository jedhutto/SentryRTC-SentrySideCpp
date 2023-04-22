#pragma once
#include <rtc/rtc.hpp>
using std::shared_ptr;
class ContolDataChannelHandler
{
public:
	short leftTrack;
	short rightTrack;
	bool stalled;
private:
	unsigned int messageCount;
	shared_ptr<rtc::DataChannel> dc;
};

