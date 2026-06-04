#include "VoltageHandler.h"
#include <iostream>
#include <chrono>
#include <thread>

VoltageHandler::VoltageHandler(int pi, ADS1256& adc, uint8_t channel)
{
	this->pi = pi;
	this->adc = &adc;
	this->channel = channel;
	this->isRunning = false;

	std::cout << "VoltageHandler initialized for channel " << (int)channel << std::endl;
}

VoltageHandler::~VoltageHandler()
{
	isRunning = false;
}

void VoltageHandler::start(shared_ptr<rtc::DataChannel>& dataChannel)
{
	isRunning = true;
	this->voltageThread = std::thread(&VoltageHandler::monitorVoltage, std::ref(dataChannel), std::ref(isRunning), this->adc, this->channel);
	this->voltageThread.detach();
}

void VoltageHandler::monitorVoltage(shared_ptr<rtc::DataChannel> dataChannel, bool& isRunning, ADS1256* adc, uint8_t channel)
{
	// Voltage divider constant (5x for the voltage sensor module)
	const float VOLTAGE_DIVIDER = 5.0f;

	std::cout << "VoltageHandler monitoring started on channel " << (int)channel << std::endl;

	while (isRunning) {
		if (dataChannel->isOpen() && adc->IsInitialized()) {
			// Switch to the specified channel
			adc->SetInputChannel(channel, ADS1256::AINCOM);

			// Small delay to allow MUX to settle
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			// Read voltage from ADC
			float adcVoltage = adc->ReadVoltage();

			// Apply voltage divider to get actual input voltage
			float voltage = adcVoltage * VOLTAGE_DIVIDER;

			// Get current timestamp in milliseconds
			auto now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

			// Create and populate signal
			VoltageDataSignal voltageData = VoltageDataSignal();
			voltageData.voltage = voltage;
			voltageData.timestamp = timestamp;

			// Send via data channel
			dataChannel->send(reinterpret_cast<const std::byte*>(&voltageData), sizeof(voltageData));

			std::cout << "Voltage CH" << (int)channel << ": " << voltage << "V (ADC: " << adcVoltage << "V) at " << timestamp << std::endl;
		}

		// Sleep for 1 second
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	std::cout << "VoltageHandler monitoring stopped" << std::endl;
}
