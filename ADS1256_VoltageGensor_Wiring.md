# ADS1256 Voltage Sensor Wiring Guide

## Architecture

The system now uses a **shared ADS1256 class** that can be accessed by multiple handlers:
- **Thread-safe** with mutex protection
- **8 channels available** (AIN0-AIN7)
- **Reusable** for voltage, current, and other analog sensors

Current usage:
- **VoltageHandler** → AIN0 (voltage sensor)
- **Future handlers** → AIN1-AIN7 (see `Adding_Multiple_Sensors.md`)

## Hardware Connections

### Voltage Sensor to ADS1256
- **Sensor "+"** → External 5V or 3.3V power supply
- **Sensor "-"** → GND (common ground with Jetson and ADS1256)
- **Sensor "s"** → ADS1256 **AIN0** (analog input channel 0)

### ADS1256 to Jetson Nano (SPI2)
The code is configured to use **SPI2** (channel 1 in jetgpio):

| ADS1256 Pin | Jetson Pin | Description |
|-------------|------------|-------------|
| VCC         | Pin 1 (3.3V) | Power supply |
| GND         | Pin 6 (GND) | Ground |
| SCLK        | Pin 13 (SPI2_SCK) | SPI Clock |
| DIN (MOSI)  | Pin 37 (SPI2_MOSI) | SPI Master Out Slave In |
| DOUT (MISO) | Pin 22 (SPI2_MISO) | SPI Master In Slave Out |
| CS          | Pin 18 (SPI2_CS0) | Chip Select |
| DRDY        | Pin 7 (GPIO) | Data Ready signal (optional but recommended) |

## ADS1256 Configuration

The VoltageHandler is configured with:
- **SPI Mode**: Mode 1 (CPOL=0, CPHA=1)
- **SPI Speed**: 1 MHz
- **Data Rate**: 10 SPS (samples per second)
- **Input Channel**: AIN0 (single-ended vs AINCOM)
- **PGA Gain**: 1x
- **Voltage Reference**: 2.5V (internal)

## Voltage Calculation

The sensor has a **5x voltage divider**, so:
- Input voltage range: 0-16.5V (with 3.3V system)
- Sensor output: 0-3.3V
- ADS1256 reads: 24-bit signed value
- Formula: `Actual Voltage = (ADC_Value / 8388607) × 2.5V × 5`

## Reading Frequency

The voltage is read and transmitted **once per second** (1 Hz), as specified in your requirements.

## Notes

1. **DRDY Pin**: Currently configured to pin 7. The code includes a waitDRDY function but uses a simple delay. For better performance, you can enable DRDY monitoring.

2. **SPI Channel**: Using SPI2 (jetgpio channel 1). If you need SPI1, change the parameter in main.cpp from `VoltageHandler(pi, 1)` to `VoltageHandler(pi, 0)`.

3. **Voltage Reference**: The code assumes 2.5V internal reference. If using external VREF, adjust the `VREF` constant in `VoltageHandler.cpp`.

4. **Calibration**: The ADS1256 performs self-calibration on startup for accuracy.

## Testing

Monitor the console output to see voltage readings:
```
Voltage: 12.34V (Raw: 4194303) at 1234567890123
```

The data is automatically sent via the RTC data channel to the controller.
