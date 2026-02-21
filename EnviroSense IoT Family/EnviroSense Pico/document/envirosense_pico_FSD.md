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
- Connectivity: WiFi + MQTT
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
- The display shall render:
  - Gas resistance (blue, size 1)
  - AQI text (green, size 2)
  - Temperature (red, size 2)
  - Humidity (yellow, size 2)
  - Pressure (cyan, size 2)

### 6.4 AQI Classification
- AQI shall be derived from gas resistance:
  - > 30000 Ohms: `Good`
  - > 15000 Ohms: `Moderate`
  - Otherwise: `Poor`

### 6.5 MQTT Connectivity
- The firmware shall maintain an MQTT connection.
- If MQTT disconnects, the firmware shall attempt reconnection in a loop with a 5-second delay between attempts.
- On successful MQTT connect, the firmware shall publish Home Assistant MQTT discovery config topics.

### 6.6 MQTT Discovery
The firmware shall publish retained discovery config topics:
- `homeassistant/sensor/enviroSense_temperature/config`
- `homeassistant/sensor/enviroSense_humidity/config`
- `homeassistant/sensor/enviroSense_pressure/config`
- `homeassistant/sensor/enviroSense_gas/config`
- `homeassistant/sensor/enviroSense_aqi/config`

### 6.7 MQTT State Publishing
- The firmware shall publish retained state topics at `mqttInterval`:
  - `EnviroSense/temperature` (float, 2 decimals)
  - `EnviroSense/humidity` (float, 2 decimals)
  - `EnviroSense/pressure` (integer hPa)
  - `EnviroSense/gas` (integer Ohms)
  - `EnviroSense/aqi` (string: `Good`, `Moderate`, `Poor`)

## 7. Timing Requirements
- `sensorInterval`: 10000 ms
- `displayInterval`: 2000 ms (defined but not used; display updates are event-driven)
- `mqttInterval`: 10000 ms

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
- `sensorInterval` (default: 10000 ms)
- `mqttInterval` (default: 10000 ms)

## 11. Assumptions and Constraints
- WiFi and MQTT broker are available and stable.
- Home Assistant MQTT integration is enabled.
- Gas resistance is used as a proxy for air quality (no external calibration or IAQ algorithm).
- Display update logic is based on change thresholds, not fixed refresh rate.

## 12. Open Items / Future Enhancements
- Replace gas-based AQI proxy with BSEC IAQ calculation.
- Add battery/power-saving modes.
- Provide OTA firmware updates.
- Add display of WiFi/MQTT status.
- Implement `displayInterval` or remove unused timer.
