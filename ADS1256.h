#pragma once
#include <cstdint>
#include <mutex>

class ADS1256
{
public:
	ADS1256();
	ADS1256(int pi, int spiChannel, int drdyPin = -1);
	~ADS1256();

	// Configuration methods
	bool SetDataRate(uint8_t rate);
	bool SetGain(uint8_t gain);
	bool SetInputChannel(uint8_t positiveChannel, uint8_t negativeChannel);
	bool PerformSelfCalibration();

	// Reading methods
	int32_t ReadRaw();
	float ReadVoltage();

	// Register access
	void WriteRegister(uint8_t reg, uint8_t value);
	uint8_t ReadRegister(uint8_t reg);

	// Status
	bool IsInitialized() const { return initialized; }
	int GetSpiHandle() const { return spiHandle; }

	// Data rate constants
	static const uint8_t DRATE_30000SPS = 0xF0;
	static const uint8_t DRATE_15000SPS = 0xE0;
	static const uint8_t DRATE_7500SPS  = 0xD0;
	static const uint8_t DRATE_3750SPS  = 0xC0;
	static const uint8_t DRATE_2000SPS  = 0xB0;
	static const uint8_t DRATE_1000SPS  = 0xA1;
	static const uint8_t DRATE_500SPS   = 0x92;
	static const uint8_t DRATE_100SPS   = 0x82;
	static const uint8_t DRATE_60SPS    = 0x72;
	static const uint8_t DRATE_50SPS    = 0x63;
	static const uint8_t DRATE_30SPS    = 0x53;
	static const uint8_t DRATE_25SPS    = 0x43;
	static const uint8_t DRATE_15SPS    = 0x33;
	static const uint8_t DRATE_10SPS    = 0x23;
	static const uint8_t DRATE_5SPS     = 0x13;
	static const uint8_t DRATE_2_5SPS   = 0x03;

	// Gain constants
	static const uint8_t GAIN_1  = 0x00;
	static const uint8_t GAIN_2  = 0x01;
	static const uint8_t GAIN_4  = 0x02;
	static const uint8_t GAIN_8  = 0x03;
	static const uint8_t GAIN_16 = 0x04;
	static const uint8_t GAIN_32 = 0x05;
	static const uint8_t GAIN_64 = 0x06;

	// Input channel constants (for single-ended measurements vs AINCOM)
	static const uint8_t AIN0 = 0;
	static const uint8_t AIN1 = 1;
	static const uint8_t AIN2 = 2;
	static const uint8_t AIN3 = 3;
	static const uint8_t AIN4 = 4;
	static const uint8_t AIN5 = 5;
	static const uint8_t AIN6 = 6;
	static const uint8_t AIN7 = 7;
	static const uint8_t AINCOM = 8;

private:
	// Register addresses
	static const uint8_t REG_STATUS = 0x00;
	static const uint8_t REG_MUX    = 0x01;
	static const uint8_t REG_ADCON  = 0x02;
	static const uint8_t REG_DRATE  = 0x03;

	// Commands
	static const uint8_t CMD_WAKEUP  = 0x00;
	static const uint8_t CMD_RDATA   = 0x01;
	static const uint8_t CMD_RDATAC  = 0x03;
	static const uint8_t CMD_SDATAC  = 0x0F;
	static const uint8_t CMD_RREG    = 0x10;
	static const uint8_t CMD_WREG    = 0x50;
	static const uint8_t CMD_SELFCAL = 0xF0;
	static const uint8_t CMD_SYNC    = 0xFC;
	static const uint8_t CMD_STANDBY = 0xFD;
	static const uint8_t CMD_RESET   = 0xFE;

	// Helper methods
	void WaitDRDY(int timeout_ms = 100);
	void SendCommand(uint8_t cmd);

	// Member variables
	int pi;
	int spiHandle;
	int drdyPin;
	bool initialized;
	float vref;  // Voltage reference (typically 2.5V)
	uint8_t currentGain;
	std::mutex spiMutex;  // Thread safety for async handlers
};
