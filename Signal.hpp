#pragma once
#include <cstdint>
class Signal
{
public:
    int16_t id;

    enum SignalType : short
    {
        Movement = 0,
        DataStream = 1,
        Interact = 2,
        MovementConfig = 3,
        CameraConfig = 4,
        Message = 5
    };
};
