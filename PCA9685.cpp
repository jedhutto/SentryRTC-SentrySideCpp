#include "PCA9685.h"
#include <cmath>
#include <unistd.h>

PCA9685::PCA9685(int pi, int bus, uint8_t address, int channel)
{
    this->pi = pi;
    this->bus = bus;
    this->address = address;

    handle = i2c_open(pi, bus, address, NULL);

    WriteReg(MODE1, AI | ALLCALL);
    WriteReg(MODE2, OCH | OUTDRV);

    usleep(5);

    uint8_t mode = ReadReg(MODE1);
    WriteReg(MODE1, mode & ~SLEEP);

    usleep(5);

    SetDutyCycle(-1, 0);
    SetFrequency(50);
}

PCA9685::~PCA9685()
{
    Cancel();
}

float PCA9685::GetFrequency()
{
    return frequency;
}

bool PCA9685::SetFrequency(double frequency)
{
    int prescale = int(round(25000000.0 / (4096.0 * frequency)) - 1);

    if (prescale < 3) 
    {
        prescale = 3;
    }
    else if (prescale > 255)
    {
        prescale = 255;
    }

    int mode = ReadReg(MODE1);
    WriteReg(MODE1, (mode & ~SLEEP) | SLEEP);
    WriteReg(PRESCALE, prescale);
    WriteReg(MODE1, mode);

    usleep(5);

    WriteReg(MODE1, mode | RESTART);

    this->frequency = (25000000.0 / 4096.0) / (prescale + 1);
    pulseWidth = (1000000.0 / this->frequency);

    i2c_write_byte_data(pi, handle, MODE2, SUBADR3);
    return true;
}

bool PCA9685::SetDutyCycle(int channel, int usec)
{
    if(500 <= usec && usec <= 2500)
    {
        //unsigned int pulseWidthValue = static_cast<unsigned int>(usec * 4096.0 / (1000000.0 / frequency));
        unsigned int pulseWidthValue = (usec * 4096.0 / (1000000.0 / frequency));
        i2c_write_word_data(pi, handle, LED0_ON_L + 4 * channel, 0);
        i2c_write_word_data(pi, handle, LED0_OFF_L + 4 * channel, pulseWidthValue);
    }

    return true;
}

bool PCA9685::SetDutyCyclePercent(int channel, float percent)
{
    if (0 <= percent && percent <= 1) 
    {
        SetDutyCycle(channel, 500 + 2000*percent);
    }
    //int steps = int(round(percent * (4096.0 / 100.0)));
    //int on, off;
    //if (steps < 0)
    //{
    //    on = 0;
    //    off = 4096;
    //}
    //else if (steps > 4095)
    //{
    //    on = 4096;
    //    off = 0;
    //}
    //else {
    //    on = 0;
    //    off = steps;
    //}
    //
    //if (channel >= 0 && channel <= 15)
    //{
    //    char buf[] = { on & 0xFF, on >> 8, off & 0xFF, off >> 8 };
    //    i2c_write_i2c_block_data(pi, handle, LED0_ON_L + 4 * channel, buf, 4);
    //    return true;
    //}
    //return false;
}

bool PCA9685::SetPulseWidth(int channel, float width)
{
    return SetDutyCycle(channel, (width / pulseWidth) * 100.0);
}

bool PCA9685::Cancel()
{
    SetDutyCycle(-1, 0);
    i2c_close(pi, handle);
}

int PCA9685::WriteReg(int reg, uint8_t byte)
{
    return i2c_write_byte_data(pi, handle, reg, byte);
}

uint8_t PCA9685::ReadReg(int reg)
{
    return i2c_read_byte_data(pi, handle, reg);
}
