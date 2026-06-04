#include "ADS1256.h"
#include <jetgpio.h>
#include <iostream>
#include <thread>
#include <chrono>

ADS1256::ADS1256()
{
	this->pi = -1;
	this->spiHandle = -1;
	this->drdyPin = -1;
	this->initialized = false;
	this->vref = 2.5f;
	this->currentGain = GAIN_1;
}

ADS1256::ADS1256(int pi, int spiChannel, int drdyPin)
{
	this->pi = pi;
	this->drdyPin = drdyPin;
	this->initialized = false;
	this->vref = 2.5f;
	this->currentGain = GAIN_1;

	// Set DRDY pin as input if provided
	if (this->drdyPin >= 0) {
		gpioSetMode(this->drdyPin, JET_INPUT);
	}

	// Open SPI connection for ADS1256
	// SPI Mode 1 (CPOL=0, CPHA=1), 1 MHz speed, 8 bits per word
	this->spiHandle = spiOpen(spiChannel, 1000000, 1, 0, 8, 0, 0);

	if (this->spiHandle < 0) {
		std::cerr << "ADS1256: Failed to open SPI channel " << spiChannel << std::endl;
		return;
	}

	std::cout << "ADS1256: SPI initialized on channel " << spiChannel << std::endl;

	// Initialize ADS1256
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Reset ADS1256
	SendCommand(CMD_RESET);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Configure defaults
	SetDataRate(DRATE_10SPS);
	SetInputChannel(AIN0, AINCOM);  // Single-ended AIN0
	SetGain(GAIN_1);

	// Perform self-calibration
	PerformSelfCalibration();

	this->initialized = true;
	std::cout << "ADS1256: Initialized and calibrated" << std::endl;
}

ADS1256::~ADS1256()
{
	if (spiHandle >= 0) {
		spiClose(spiHandle);
	}
}

void ADS1256::SendCommand(uint8_t cmd)
{
	std::lock_guard<std::mutex> lock(spiMutex);
	char txBuf[1] = { (char)cmd };
	char rxBuf[1];
	spiXfer(spiHandle, txBuf, rxBuf, 1);
	std::this_thread::sleep_for(std::chrono::microseconds(10));
}

void ADS1256::WriteRegister(uint8_t reg, uint8_t value)
{
	std::lock_guard<std::mutex> lock(spiMutex);
	char txBuf[3];
	char rxBuf[3];

	txBuf[0] = CMD_WREG | (reg & 0x0F);  // WREG command + register address
	txBuf[1] = 0x00;  // Number of registers to write - 1 (0 = 1 register)
	txBuf[2] = value;

	spiXfer(spiHandle, txBuf, rxBuf, 3);
	std::this_thread::sleep_for(std::chrono::microseconds(10));
}

uint8_t ADS1256::ReadRegister(uint8_t reg)
{
	std::lock_guard<std::mutex> lock(spiMutex);
	char txBuf[3];
	char rxBuf[3];

	txBuf[0] = CMD_RREG | (reg & 0x0F);  // RREG command + register address
	txBuf[1] = 0x00;  // Number of registers to read - 1 (0 = 1 register)
	txBuf[2] = 0xFF;  // Dummy byte for reading

	spiXfer(spiHandle, txBuf, rxBuf, 3);
	std::this_thread::sleep_for(std::chrono::microseconds(10));

	return rxBuf[2];
}

void ADS1256::WaitDRDY(int timeout_ms)
{
	if (drdyPin < 0) {
		// No DRDY pin configured, use delay based on data rate
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return;
	}

	auto start = std::chrono::steady_clock::now();

	// Wait for DRDY to go LOW (data ready)
	while (gpioRead(drdyPin) == 1) {
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

		if (elapsed > timeout_ms) {
			std::cerr << "ADS1256: DRDY timeout" << std::endl;
			return;
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

bool ADS1256::SetDataRate(uint8_t rate)
{
	if (!initialized && spiHandle < 0) return false;

	WriteRegister(REG_DRATE, rate);
	return true;
}

bool ADS1256::SetGain(uint8_t gain)
{
	if (!initialized && spiHandle < 0) return false;
	if (gain > GAIN_64) return false;

	currentGain = gain;
	WriteRegister(REG_ADCON, gain);
	return true;
}

bool ADS1256::SetInputChannel(uint8_t positiveChannel, uint8_t negativeChannel)
{
	if (!initialized && spiHandle < 0) return false;
	if (positiveChannel > 7 || negativeChannel > 8) return false;

	// MUX register: High nibble = positive input, Low nibble = negative input
	uint8_t muxValue = (positiveChannel << 4) | negativeChannel;
	WriteRegister(REG_MUX, muxValue);

	return true;
}

bool ADS1256::PerformSelfCalibration()
{
	if (spiHandle < 0) return false;

	SendCommand(CMD_SELFCAL);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	return true;
}

int32_t ADS1256::ReadRaw()
{
	if (!initialized) {
		std::cerr << "ADS1256: Not initialized" << std::endl;
		return 0;
	}

	std::lock_guard<std::mutex> lock(spiMutex);

	// Send SYNC command to start conversion
	char syncCmd = CMD_SYNC;
	char dummy[1];
	spiXfer(spiHandle, &syncCmd, dummy, 1);
	std::this_thread::sleep_for(std::chrono::microseconds(5));

	// Send WAKEUP command
	char wakeupCmd = CMD_WAKEUP;
	spiXfer(spiHandle, &wakeupCmd, dummy, 1);
	std::this_thread::sleep_for(std::chrono::microseconds(25));

	// Wait for DRDY (data ready)
	WaitDRDY();

	// Send RDATA command and read 3 bytes
	char txBuf[4];
	char rxBuf[4];

	txBuf[0] = CMD_RDATA;
	txBuf[1] = 0xFF;  // Dummy bytes for reading 3-byte result
	txBuf[2] = 0xFF;
	txBuf[3] = 0xFF;

	spiXfer(spiHandle, txBuf, rxBuf, 4);

	// ADS1256 returns 24-bit signed value (3 bytes, MSB first)
	int32_t result = ((int32_t)rxBuf[1] << 16) | ((int32_t)rxBuf[2] << 8) | rxBuf[3];

	// Convert to signed 24-bit value
	if (result & 0x800000) {
		result |= 0xFF000000;  // Sign extend
	}

	return result;
}

float ADS1256::ReadVoltage()
{
	int32_t rawValue = ReadRaw();

	// ADC full scale is +/- VREF
	// For 24-bit ADC: max value is 2^23 - 1 = 8388607
	const int32_t ADC_MAX = 8388607;

	// Calculate voltage based on gain
	float gainMultiplier = 1.0f;
	switch (currentGain) {
		case GAIN_1:  gainMultiplier = 1.0f; break;
		case GAIN_2:  gainMultiplier = 2.0f; break;
		case GAIN_4:  gainMultiplier = 4.0f; break;
		case GAIN_8:  gainMultiplier = 8.0f; break;
		case GAIN_16: gainMultiplier = 16.0f; break;
		case GAIN_32: gainMultiplier = 32.0f; break;
		case GAIN_64: gainMultiplier = 64.0f; break;
	}

	// Voltage = (rawValue / ADC_MAX) * VREF / gain
	float voltage = ((float)rawValue / (float)ADC_MAX) * vref / gainMultiplier;

	return voltage;
}
