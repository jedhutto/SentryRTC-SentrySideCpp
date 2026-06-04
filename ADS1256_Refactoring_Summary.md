# ADS1256 Refactoring Summary

## Overview
The ADS1256 ADC functionality has been extracted from VoltageHandler into a reusable, thread-safe class that can be shared by multiple handlers running asynchronously.

## New Files Created

### ADS1256.h / ADS1256.cpp
**Purpose**: Reusable ADS1256 24-bit ADC driver class

**Key Features**:
- Thread-safe with mutex protection for async handlers
- 8 single-ended input channels (AIN0-AIN7)
- Configurable data rate (2.5 to 30,000 samples/second)
- Configurable PGA gain (1x to 64x)
- Self-calibration on initialization
- Register read/write access
- Raw ADC reading and voltage conversion

**Public Methods**:
```cpp
ADS1256(int pi, int spiChannel, int drdyPin = -1);
bool SetDataRate(uint8_t rate);
bool SetGain(uint8_t gain);
bool SetInputChannel(uint8_t positiveChannel, uint8_t negativeChannel);
bool PerformSelfCalibration();
int32_t ReadRaw();
float ReadVoltage();
```

**Constants**:
- Data rates: `DRATE_30000SPS` to `DRATE_2_5SPS`
- Gains: `GAIN_1` to `GAIN_64`
- Channels: `AIN0` to `AIN7`, `AINCOM`

## Modified Files

### VoltageHandler.h / VoltageHandler.cpp
**Changes**:
- Now takes `ADS1256&` reference and channel number in constructor
- Removed all ADS1256-specific code (registers, commands, SPI calls)
- Simplified to use ADS1256 class methods
- Added channel switching before reading
- Much cleaner and focused on voltage-specific logic

**New Constructor**:
```cpp
VoltageHandler(int pi, ADS1256& adc, uint8_t channel = 0);
```

### main.cpp
**Changes**:
- Added `#include "ADS1256.h"`
- Creates single `ADS1256` instance
- Passes ADS1256 reference to VoltageHandler

**Example**:
```cpp
// Initialize shared ADS1256 ADC on SPI2 (channel 1) with DRDY on pin 7
ADS1256 adc = ADS1256(pi, 1, 7);
VoltageHandler voltageHandler = VoltageHandler(pi, adc, ADS1256::AIN0);
```

## Documentation Files

### Adding_Multiple_Sensors.md
Complete guide with code examples for adding:
- Current/amperage sensors (ACS712 example)
- Additional voltage sensors
- Any analog sensor on remaining channels

### ADS1256_VoltageGensor_Wiring.md (updated)
- Added architecture overview
- Explains shared ADS1256 design
- References multi-sensor guide

## Benefits of This Refactoring

### 1. **Reusability**
- ADS1256 class can be used by multiple handlers
- No code duplication for new sensors
- Easy to add current, temperature, or other analog sensors

### 2. **Thread Safety**
- Mutex protection ensures safe concurrent access
- Multiple handlers can run async without conflicts
- Channel switching is protected

### 3. **Maintainability**
- ADS1256 logic centralized in one place
- Handlers focus on their specific sensor logic
- Easier to debug and update

### 4. **Scalability**
- Up to 8 channels available (currently using 1)
- Can add 7 more sensors without additional ADC hardware
- Each handler can have independent sampling rates

### 5. **Clean Architecture**
- Follows the same pattern as PCA9685 (shared by ServoHandler and MovementHandler)
- Separation of concerns: ADC hardware vs sensor logic
- Consistent with existing codebase style

## Usage Example: Adding a Second Sensor

```cpp
// In main.cpp:
ADS1256 adc = ADS1256(pi, 1, 7);

// Voltage sensor on channel 0
VoltageHandler voltageHandler = VoltageHandler(pi, adc, ADS1256::AIN0);

// Battery voltage on channel 1 (future)
VoltageHandler batteryHandler = VoltageHandler(pi, adc, ADS1256::AIN1);

// Current sensor on channel 2 (future)
AmperageHandler currentHandler = AmperageHandler(pi, adc, ADS1256::AIN2);

// All share the same ADS1256 instance, run async safely
```

## Technical Details

### SPI Configuration
- Mode: 1 (CPOL=0, CPHA=1)
- Speed: 1 MHz
- Bits per word: 8

### Default Settings
- Data rate: 10 SPS
- Gain: 1x
- Reference: 2.5V internal
- Input: Single-ended vs AINCOM

### Thread Safety Implementation
```cpp
std::mutex spiMutex;  // Protects SPI transactions
std::lock_guard<std::mutex> lock(spiMutex);  // Used in all SPI operations
```

## Migration Notes

### Breaking Changes
- VoltageHandler constructor signature changed
- Requires ADS1256 instance to be created first
- Channel parameter now required

### Backward Compatibility
- None (this is new functionality)
- Existing handlers (Servo, Movement, Lidar) unchanged

## Testing Checklist

- [x] Build successful
- [ ] Test voltage reading on AIN0
- [ ] Test DRDY signal (pin 7)
- [ ] Verify thread safety with multiple handlers (when added)
- [ ] Test channel switching
- [ ] Verify 1-second sampling rate
- [ ] Check data channel transmission

## Future Enhancements

1. **Add more sensor handlers** using remaining channels
2. **Implement differential input** mode (4 differential pairs)
3. **Add filtering/averaging** for noisy sensors
4. **Calibration** with known reference voltages
5. **Error handling** improvements
6. **Continuous reading mode** (RDATAC) for high-speed applications
