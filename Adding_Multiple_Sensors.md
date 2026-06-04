# Adding Multiple Sensors with ADS1256

The ADS1256 class has been extracted to support multiple handlers reading from different channels. This allows you to easily add voltage sensors, current sensors (amperage), or any other analog sensor.

## Current Architecture

```
ADS1256 (shared, thread-safe)
	├── VoltageHandler (Channel AIN0) - reads voltage every 1 second
	├── [Future] AmperageHandler (Channel AIN1) - can read current
	└── [Future] Additional handlers on AIN2-AIN7
```

## ADS1256 Features

- **8 single-ended channels** (AIN0 - AIN7) or 4 differential pairs
- **Thread-safe** with mutex protection for async handlers
- **24-bit precision** with programmable gain (1x to 64x)
- **Configurable data rate** (2.5 SPS to 30,000 SPS)

## How to Add an Amperage Sensor

### Example: ACS712 Current Sensor (5A, 20A, or 30A versions)

1. **Create AmperageDataSignal.hpp**
```cpp
#pragma once
#include "Signal.hpp"
#include <cstdint>

class AmperageDataSignal : Signal
{
public:
	float amperage;
	int64_t timestamp;

	AmperageDataSignal() {
		id = this->AmperageData;  // Add to Signal.hpp enum
		amperage = 0.0f;
		timestamp = 0;
	}
};
```

2. **Create AmperageHandler.h**
```cpp
#pragma once
#include <rtc/rtc.hpp>
#include <thread>
#include "AmperageDataSignal.hpp"
#include "ADS1256.h"

using std::shared_ptr;

class AmperageHandler
{
public:
	AmperageHandler(int pi, ADS1256& adc, uint8_t channel = 1);
	~AmperageHandler();
	void start(shared_ptr<rtc::DataChannel>&);

private:
	static void monitorAmperage(shared_ptr<rtc::DataChannel> dataChannel, 
								 bool& isRunning, ADS1256* adc, uint8_t channel);

	int pi;
	ADS1256* adc;
	uint8_t channel;
	std::thread amperageThread;
	bool isRunning;
};
```

3. **Create AmperageHandler.cpp**
```cpp
#include "AmperageHandler.h"
#include <iostream>
#include <chrono>
#include <thread>

AmperageHandler::AmperageHandler(int pi, ADS1256& adc, uint8_t channel)
{
	this->pi = pi;
	this->adc = &adc;
	this->channel = channel;
	this->isRunning = false;

	std::cout << "AmperageHandler initialized for channel " << (int)channel << std::endl;
}

AmperageHandler::~AmperageHandler()
{
	isRunning = false;
}

void AmperageHandler::start(shared_ptr<rtc::DataChannel>& dataChannel)
{
	isRunning = true;
	this->amperageThread = std::thread(&AmperageHandler::monitorAmperage, 
										std::ref(dataChannel), std::ref(isRunning), 
										this->adc, this->channel);
	this->amperageThread.detach();
}

void AmperageHandler::monitorAmperage(shared_ptr<rtc::DataChannel> dataChannel, 
									   bool& isRunning, ADS1256* adc, uint8_t channel)
{
	// ACS712-05B: 185 mV/A (5A version)
	// ACS712-20A: 100 mV/A (20A version)  
	// ACS712-30A: 66 mV/A (30A version)
	const float SENSITIVITY = 0.185f;  // V/A for 5A version
	const float VREF_OFFSET = 2.5f;    // ACS712 outputs 2.5V at 0A

	std::cout << "AmperageHandler monitoring started on channel " << (int)channel << std::endl;

	while (isRunning) {
		if (dataChannel->isOpen() && adc->IsInitialized()) {
			// Switch to the specified channel
			adc->SetInputChannel(channel, ADS1256::AINCOM);

			// Small delay to allow MUX to settle
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			// Read voltage from ADC
			float voltage = adc->ReadVoltage();

			// Convert voltage to amperage
			// Current = (Voltage - VRef_offset) / Sensitivity
			float amperage = (voltage - VREF_OFFSET) / SENSITIVITY;

			// Get current timestamp
			auto now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

			// Create and send signal
			AmperageDataSignal amperageData = AmperageDataSignal();
			amperageData.amperage = amperage;
			amperageData.timestamp = timestamp;

			dataChannel->send(reinterpret_cast<const std::byte*>(&amperageData), sizeof(amperageData));

			std::cout << "Amperage CH" << (int)channel << ": " << amperage 
					  << "A (Voltage: " << voltage << "V) at " << timestamp << std::endl;
		}

		// Sleep for 1 second (or adjust as needed)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	std::cout << "AmperageHandler monitoring stopped" << std::endl;
}
```

4. **Update Signal.hpp**
```cpp
enum SignalType : short
{
	Movement = 0,
	DataStream = 1,
	Interact = 2,
	MovementConfig = 3,
	CameraConfig = 4,
	Message = 5,
	CameraLook = 6,
	LidarDataArray = 7,
	VoltageData = 8,
	AmperageData = 9,  // Add this
};
```

5. **Update main.cpp**
```cpp
#include "AmperageHandler.h"

int main() {
	// ... existing initialization ...

	// Initialize shared ADS1256 ADC
	ADS1256 adc = ADS1256(pi, 1, 7);

	// Create voltage handler on AIN0
	VoltageHandler voltageHandler = VoltageHandler(pi, adc, ADS1256::AIN0);

	// Create amperage handler on AIN1
	AmperageHandler amperageHandler = AmperageHandler(pi, adc, ADS1256::AIN1);

	// ... in ConfigurePeer, add amperageHandler parameter ...

	// In onDataChannel callback:
	voltageHandler.start(dc);
	amperageHandler.start(dc);
}
```

## ADS1256 Channel Mapping

You can use up to 8 single-ended channels:

| Channel Constant | Physical Pin | Use Case Example |
|-----------------|--------------|------------------|
| ADS1256::AIN0   | AIN0         | Voltage sensor (current use) |
| ADS1256::AIN1   | AIN1         | Current sensor |
| ADS1256::AIN2   | AIN2         | Battery voltage |
| ADS1256::AIN3   | AIN3         | Temperature sensor (with conversion) |
| ADS1256::AIN4   | AIN4         | Available |
| ADS1256::AIN5   | AIN5         | Available |
| ADS1256::AIN6   | AIN6         | Available |
| ADS1256::AIN7   | AIN7         | Available |

## Thread Safety

The ADS1256 class includes mutex protection, so multiple handlers can safely:
- Switch channels with `SetInputChannel()`
- Read values with `ReadVoltage()` or `ReadRaw()`
- Configure settings independently

Each handler switches to its channel before reading, allowing multiple async handlers to share one ADC.

## Sensor Wiring

### Voltage Sensor (current - AIN0)
- Sensor output → ADS1256 AIN0

### Current Sensor (example - AIN1)
- ACS712 VCC → 5V
- ACS712 GND → GND
- ACS712 OUT → ADS1256 AIN1

### Additional sensors
- Connect to AIN2-AIN7 as needed
- All sensors share common GND with ADS1256 and Jetson

## Notes

- All handlers run asynchronously in separate threads
- The ADS1256 mutex ensures safe channel switching
- Each handler can have its own sampling rate (e.g., voltage at 1Hz, current at 10Hz)
- Voltage calculations are specific to each sensor type (adjust constants accordingly)
