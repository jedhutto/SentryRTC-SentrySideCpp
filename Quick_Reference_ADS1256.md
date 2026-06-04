# Quick Reference: ADS1256 Multi-Sensor System

## Current Setup (Single Voltage Sensor)

```cpp
// In main.cpp
ADS1256 adc = ADS1256(pi, 1, 7);  // SPI2, DRDY on pin 7
VoltageHandler voltageHandler = VoltageHandler(pi, adc, ADS1256::AIN0);

// In ConfigurePeer, onDataChannel callback:
voltageHandler.start(dc);
```

## Adding a Second Voltage Sensor (e.g., Battery Monitor)

```cpp
// 1. Create second handler in main()
ADS1256 adc = ADS1256(pi, 1, 7);
VoltageHandler voltageHandler = VoltageHandler(pi, adc, ADS1256::AIN0);
VoltageHandler batteryHandler = VoltageHandler(pi, adc, ADS1256::AIN1);  // Channel 1

// 2. Add to ConfigurePeer parameters
int ConfigurePeer(..., VoltageHandler& voltageHandler, VoltageHandler& batteryHandler, ...)

// 3. Start in onDataChannel callback
voltageHandler.start(dc);
batteryHandler.start(dc);

// 4. Wire battery voltage divider output to ADS1256 AIN1
```

## Adding Current Sensor (e.g., ACS712)

See detailed example in `Adding_Multiple_Sensors.md`

**Quick steps:**
1. Add `AmperageData = 9` to Signal.hpp enum
2. Create AmperageDataSignal.hpp (copy VoltageDataSignal pattern)
3. Create AmperageHandler.h/.cpp (copy VoltageHandler pattern)
4. Adjust voltage-to-current formula for ACS712
5. Update main.cpp to instantiate and start handler

```cpp
AmperageHandler currentHandler = AmperageHandler(pi, adc, ADS1256::AIN2);
currentHandler.start(dc);
```

## ADS1256 Shared Instance

**One ADS1256 instance** → **Multiple handlers** (up to 8 channels)

```
		 ┌──────────────┐
		 │   ADS1256    │ ← Single instance (thread-safe)
		 │   (SPI2)     │
		 └──────┬───────┘
				│
	   ┌────────┴─────────┬──────────┬─────────┐
	   │                  │          │         │
   ┌───▼────┐      ┌─────▼──┐   ┌──▼───┐  ┌──▼───┐
   │ AIN0   │      │  AIN1  │   │ AIN2 │  │ ...  │
   │Voltage │      │Battery │   │Current│  │      │
   └────────┘      └────────┘   └──────┘  └──────┘
```

## Available Channels

| Channel | Status | Suggested Use |
|---------|--------|--------------|
| AIN0    | ✅ Used | Voltage sensor (5x divider, 0-16.5V) |
| AIN1    | 🆓 Free | Battery voltage |
| AIN2    | 🆓 Free | Current sensor (ACS712) |
| AIN3    | 🆓 Free | Temperature (with conversion) |
| AIN4    | 🆓 Free | Available |
| AIN5    | 🆓 Free | Available |
| AIN6    | 🆓 Free | Available |
| AIN7    | 🆓 Free | Available |

## Key Features

✅ **Thread-safe**: Multiple async handlers can share one ADC  
✅ **24-bit precision**: High accuracy readings  
✅ **Configurable gain**: 1x to 64x for different voltage ranges  
✅ **8 channels**: No need for multiple ADC chips  
✅ **Easy to extend**: Just create new handler using existing pattern  

## Common Sensor Types

| Sensor Type | Channel | Formula | Example |
|-------------|---------|---------|---------|
| Voltage (5x divider) | AIN0 | `V_actual = V_adc × 5` | Current setup |
| Current (ACS712-05) | Any | `I = (V - 2.5) / 0.185` | 5A max |
| Current (ACS712-20) | Any | `I = (V - 2.5) / 0.100` | 20A max |
| Battery (direct) | Any | `V = V_adc` | 0-2.5V max |
| Temperature (LM35) | Any | `T = V_adc × 100` | °C |

## Wiring Checklist

- [ ] ADS1256 VCC → Jetson 3.3V (Pin 1)
- [ ] ADS1256 GND → Jetson GND (Pin 6)
- [ ] ADS1256 SCLK → Pin 13 (SPI2_SCK)
- [ ] ADS1256 DIN → Pin 37 (SPI2_MOSI)
- [ ] ADS1256 DOUT → Pin 22 (SPI2_MISO)
- [ ] ADS1256 CS → Pin 18 (SPI2_CS0)
- [ ] ADS1256 DRDY → Pin 7 (optional but recommended)
- [ ] Sensor outputs → ADS1256 AIN0-AIN7

## Troubleshooting

**No readings**: Check SPI wiring and DRDY connection  
**Incorrect values**: Verify voltage divider ratio and sensor specs  
**Conflicts**: Ensure all sensors share common ground  
**Thread issues**: ADS1256 class handles locking automatically  

## Documentation Files

- `ADS1256_VoltageGensor_Wiring.md` - Hardware wiring guide
- `Adding_Multiple_Sensors.md` - Complete code examples
- `ADS1256_Refactoring_Summary.md` - Technical architecture details
- `Quick_Reference_ADS1256.md` - This file
