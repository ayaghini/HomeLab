# EnviroSense Touch

EnviroSense Touch is the bigger sibling of the EnviroSense Pico project.  
Originally designed back in 2020, this project provided environmental sensing with MQTT integration for **Node-RED, InfluxDB, and Grafana**.  

Now in 2025, the project has been updated to integrate directly with **Home Assistant**, while still keeping the flexibility of MQTT telemetry.  
The original implementation can still be found in the [`archive`](./archive) folder.

---

## Features
- Environmental monitoring with:
  - Temperature  
  - Humidity  
  - tVOC (Total Volatile Organic Compounds)  
  - eCO₂ (Equivalent CO₂)  

- TFT touchscreen display for **live data visualization**  
- Wi-Fi enabled  
- MQTT publishing of sensor data  
- **Home Assistant discovery** support (sensors auto-configure on HA)  
- OTA updates for easy firmware upgrades  

---

## MQTT Topics

The device publishes to the following topics:

- `EnviroSenseTouch/ENVT_temperature` – Temperature in °C  
- `EnviroSenseTouch/ENVT_humidity` – Relative Humidity in %  
- `EnviroSenseTouch/ENVT_tVoc` – tVOC in ppb  
- `EnviroSenseTouch/ENVT_equivalentCO2` – eCO₂ in ppm  

---

## Home Assistant Integration

This version leverages MQTT Discovery for seamless Home Assistant setup.  
Once connected, the sensors will automatically appear in HA with correct units and device classes.

---

## Hardware

- **MCU**: ESP8266 (D1 Mini recommended)  
- **Display**: Adafruit ILI9341 TFT  
- **Touch Controller**: XPT2046  
- **Sensors**:  
  - SGP30 (tVOC & eCO₂)  
  - SHT30 (Temperature & Humidity, I²C address `0x45`)  

Optional:
- Backlight dimming via PWM on the `BL/LED` pin of the TFT (if broken out on your display board).  

---

## 🔌 Wiring

### TFT Display (ILI9341 SPI)
| ILI9341 Pin | D1 Mini Pin |
|-------------|-------------|
| CS          | D0          |
| DC          | D8          |
| RST         | Not connected (`-1` in code) |
| SCK         | D5          |
| MOSI        | D7          |
| MISO        | D6          |
| VCC         | 3.3V        |
| GND         | GND         |

### Touch Controller (XPT2046 SPI)
| XPT2046 Pin | D1 Mini Pin |
|-------------|-------------|
| CS          | D3          |
| SCK         | D5 (shared with TFT) |
| MOSI        | D7 (shared with TFT) |
| MISO        | D6 (shared with TFT) |
| VCC         | 3.3V        |
| GND         | GND         |

### Sensors
- **SGP30**: I²C (`SDA` = D2, `SCL` = D1)  
- **SHT30**: I²C (`SDA` = D2, `SCL` = D1, address `0x45`)  

---

## Project Structure
```
.
├── src/               # Main Arduino/PlatformIO source
├── archive/           # Old codebase (Node-RED, Influx, Grafana version from 2020)
├── include/           # Header files
├── lib/               # Libraries (if local copies needed)
└── README.md          # This file
```

---

## Getting Started

1. Clone this repo:
   ```bash
   git clone https://github.com/<your-username>/EnviroSenseTouch.git
   ```
2. Install required libraries (via Arduino Library Manager or PlatformIO):
   - Adafruit GFX  
   - Adafruit ILI9341  
   - XPT2046 Touchscreen  
   - Adafruit SGP30  
   - SHT3X  
   - PubSubClient (MQTT)  
   - ArduinoOTA  
   - NTPClient  
   - ArduinoJson  
3. Configure Wi-Fi, MQTT, and OpenWeatherMap credentials in `config.h` (or directly in the source).  
4. Flash to your ESP8266.  
5. Open Home Assistant → the device should auto-discover and add sensors.  

---

## Archive

The `archive` folder contains the **legacy 2020 implementation**, which integrated with:
- Node-RED
- InfluxDB
- Grafana

This is kept for reference and historical purposes.  

---

## Screenshots (Optional)
_(Add photos of your TFT display and HA integration here)_

---

## License
MIT – feel free to use, modify, and improve.

---
