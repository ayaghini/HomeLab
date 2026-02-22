# EnviroSense Pico - Functional Specification Document (FSD)

## 1. Purpose
EnviroSense Pico is a compact air quality monitoring device that reads environmental data from a BME680 sensor, displays key metrics on a small TFT display, and publishes the readings to Home Assistant via MQTT with auto-discovery.

## 2. Scope
This specification covers the behavior of the EnviroSense Pico firmware running on an ESP8266-class board (NodeMCU/Wemos D1 Mini class), wired to a BME680 sensor over I2C and an ST7735 TFT display over SPI. The device connects to WiFi, publishes MQTT state topics, and registers Home Assistant MQTT Discovery config topics.

Out of scope:
- Mechanical enclosure or PCB details
- Power management for battery operation
- Home Assistant dashboard configuration beyond discovery

## 3. System Overview
Functional blocks:
- Sensor acquisition: BME680 temperature, humidity, pressure, gas resistance
- Data processing: temperature offset, gas-based AQI classification
- Display: 80x160 TFT, color-coded readings
- Connectivity: WiFi + MQTT, WiFi fallback AP + setup portal
- Home Assistant integration: MQTT Discovery

## 4. Hardware Requirements
- Microcontroller: ESP8266 (NodeMCU, Wemos D1 Mini, or equivalent)
- Sensor: BME680 (I2C)
- Display: ST7735 80x160 TFT (SPI)
- Power: 5V USB (3.3V for sensor)

### 4.1 Wiring
BME680 (I2C):
- VIN -> 3.3V
- GND -> GND
- SCL -> D1
- SDA -> D2

ST7735 TFT (SPI):
- CS -> D3
- DC -> D4
- MOSI -> D7
- SCLK -> D5
- RST -> RX

## 5. Software Dependencies
Required Arduino libraries:
- Adafruit_BME680
- Adafruit_GFX
- Adafruit_ST7735
- ESP8266WiFi
- PubSubClient

Configuration file:
- `secrets.h` providing `ssid`, `password`, `mqtt_server`, `mqtt_port`, `mqtt_user`, `mqtt_pass`

## 6. Functional Requirements
### 6.1 Startup and Initialization
- The firmware shall initialize Serial at 115200 baud.
- The firmware shall connect to the configured WiFi network.
- If WiFi does not connect within `wifiConnectTimeout`, the firmware shall start an Access Point and configuration portal.
- The firmware shall initialize the ST7735 display with `INITR_MINI160x80`.
- The firmware shall initialize the BME680 sensor and configure oversampling and gas heater settings.
- The firmware shall configure the MQTT client with the broker address and port.

### 6.2 Sensor Acquisition
- The firmware shall read BME680 data on a fixed interval (`sensorInterval`).
- The firmware shall compute:
  - Temperature with a static offset (`tempOffset`).
  - Humidity (percent).
  - Pressure in hPa.
  - Gas resistance in Ohms.
- The firmware shall output sensor values to Serial for debugging each read cycle.

### 6.3 Display Updates
- The firmware shall update the TFT display when any of the following changes occur:
  - Temperature changes by >= `tempThreshold`.
  - Humidity changes by >= `humidityThreshold`.
  - Gas resistance changes by >= `gasThreshold`.
  - AQI text category changes.
- The display shall render a 3x2 grid of labeled cells:
  - TEMP, HUM, PRES, GAS, AQI, STAT.
- The display shall render:
  - Temperature (orange, size 1)
  - Humidity (cyan, size 1)
  - Pressure (light blue, size 1)
  - Gas resistance (blue, size 1)
  - AQI text (green, size 1)
  - MQTT status (white, size 1)
- The header shall show WiFi and MQTT status dots.

### 6.4 AQI Classification
- AQI shall be derived from gas resistance:
  - > 30000 Ohms: `Good`
  - > 15000 Ohms: `Moderate`
  - Otherwise: `Poor`

### 6.5 MQTT Connectivity
- The firmware shall maintain an MQTT connection.
- If MQTT disconnects, the firmware shall attempt reconnection in a loop with a 5-second delay between attempts.
- On successful MQTT connect, the firmware shall publish Home Assistant MQTT discovery config topics.

### 6.6 WiFi Fallback Portal
- If WiFi fails to connect within `wifiConnectTimeout`, the firmware shall start AP mode with SSID `EnviroSensePico-<chipid>`.
- The firmware shall host a setup page at `http://192.168.4.1` for entering WiFi and MQTT credentials.
- Credentials shall be saved in LittleFS and persist across reboots.
- If a saved config exists, it shall be loaded at boot and used before `secrets.h` fallback.
- If no saved config exists and `secrets.h` has values, those values shall be written to LittleFS once.

### 6.7 MQTT Discovery
The firmware shall publish retained discovery config topics:
- `homeassistant/sensor/enviroSense_temperature/config`
- `homeassistant/sensor/enviroSense_humidity/config`
- `homeassistant/sensor/enviroSense_pressure/config`
- `homeassistant/sensor/enviroSense_gas/config`
- `homeassistant/sensor/enviroSense_aqi/config`

### 6.8 MQTT State Publishing
- The firmware shall publish retained state topics at `mqttInterval`:
  - `EnviroSense/temperature` (float, 2 decimals)
  - `EnviroSense/humidity` (float, 2 decimals)
  - `EnviroSense/pressure` (integer hPa)
  - `EnviroSense/gas` (integer Ohms)
  - `EnviroSense/aqi` (string: `Good`, `Moderate`, `Poor`)

## 7. Timing Requirements
- `sensorInterval`: 2000 ms
- `displayInterval`: 2000 ms (defined and used for minimum refresh cadence)
- `mqttInterval`: 60000 ms
- `wifiConnectTimeout`: 30000 ms

## 8. Error Handling
- If the BME680 initialization fails, the firmware shall halt in an infinite loop.
- If a sensor reading fails, the firmware shall log an error to Serial and return without updating values.
- If MQTT fails to connect, the firmware shall retry every 5 seconds.

## 9. Non-Functional Requirements
- Firmware shall be compatible with ESP8266 Arduino core.
- Memory usage shall remain within ESP8266 limits with the listed libraries.
- The display shall remain readable in indoor lighting.
- Network and broker credentials shall not be hardcoded in source (use `secrets.h`).

## 10. Configuration Parameters
Key configuration values in firmware:
- `tempOffset` (default: -3.06 C)
- `tempThreshold` (default: 1.0 C)
- `humidityThreshold` (default: 5.0 %)
- `gasThreshold` (default: 100000 Ohms)
- `sensorInterval` (default: 2000 ms)
- `displayInterval` (default: 2000 ms)
- `mqttInterval` (default: 60000 ms)
- `wifiConnectTimeout` (default: 30000 ms)
- `CONFIG_FILE` (default: `/config.txt` in LittleFS)

## 11. Assumptions and Constraints
- WiFi and MQTT broker are available and stable.
- Home Assistant MQTT integration is enabled.
- Gas resistance is used as a proxy for air quality (no external calibration or IAQ algorithm).
- Display update logic is based on change thresholds, not fixed refresh rate.

## 12. Open Items / Future Enhancements
- Replace gas-based AQI proxy with BSEC IAQ calculation.
- Add battery/power-saving modes.
- Provide OTA firmware updates.
- Implement `displayInterval` or remove unused timer.

## 13. Findings / Improvement Areas (Firmware Review)
1. MQTT discovery and state topics are not device-unique; multiple devices will collide on the same topics.
2. TFT reset is wired to `RX`, which can conflict with Serial usage.
3. FSD vs firmware drift exists (intervals and display details).
4. Heavy `String` usage in hot paths can fragment heap over long uptime.
5. Serial logging runs every sensor cycle; consider gating or rate limiting for production.
6. MQTT availability reflects only MQTT connection; WiFi loss may not publish `offline`.
7. Display refresh depends on thresholds; add a max-age refresh to avoid stale display.
