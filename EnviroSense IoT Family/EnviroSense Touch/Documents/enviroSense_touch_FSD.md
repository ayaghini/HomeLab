# EnviroSense Touch Functional Specification

## Purpose
EnviroSense Touch is a Wi-Fi connected environmental sensing device that measures temperature, humidity, tVOC, and eCO2. It provides a local TFT touchscreen display for live visualization and publishes telemetry over MQTT with Home Assistant (HA) auto-discovery support. It also supports OTA firmware updates for easy maintenance.

## Scope
This specification covers the device behavior, telemetry, user interaction, and integration requirements for the 2025 Home Assistant/MQTT version of EnviroSense Touch. The legacy 2020 implementation in `archive/` is out of scope.

## Users and Use Cases
- Home users who want real-time indoor air quality monitoring.
- Home Assistant users who want sensors auto-discovered and available without manual configuration.
- MQTT users who want raw telemetry for dashboards or automation.

Primary use cases:
- View live sensor readings on the TFT display.
- Ingest sensor readings in Home Assistant via MQTT Discovery.
- Subscribe to MQTT topics for custom integrations.
- Update firmware over the air.

## Functional Requirements

### 1) Sensor Data Acquisition
- The device shall sample the following sensors:
  - Temperature (SHT30)
  - Humidity (SHT30)
  - tVOC (SGP30)
  - eCO2 (SGP30)
- The device shall acquire data over I2C at address `0x45` for SHT30.
- The device shall compute or obtain tVOC (ppb) and eCO2 (ppm) from SGP30.

### 2) On-Device Display
- The device shall present live readings on a TFT display (ILI9341).
- The display shall update to reflect the latest readings.
- Touch input shall be supported through XPT2046 for any UI interactions supported by firmware (if implemented).

### 3) Wi-Fi Connectivity
- The device shall connect to a configured Wi-Fi network at boot.
- The device shall maintain Wi-Fi connectivity and retry on failure.

### 4) MQTT Telemetry
- The device shall publish sensor readings to MQTT using the following topics:
  - `EnviroSenseTouch/ENVT_temperature` (°C)
  - `EnviroSenseTouch/ENVT_humidity` (% RH)
  - `EnviroSenseTouch/ENVT_tVoc` (ppb)
  - `EnviroSenseTouch/ENVT_equivalentCO2` (ppm)
- The device shall publish readings at a regular interval (defined in firmware configuration).

### 5) Home Assistant Discovery
- The device shall publish MQTT Discovery messages so sensors auto-appear in HA.
- Sensor entities shall include correct units and device classes for temperature, humidity, tVOC, and eCO2.

### 6) OTA Updates
- The device shall support OTA firmware updates via ArduinoOTA.
- OTA updates shall be available when the device is connected to Wi-Fi.

## Non-Functional Requirements
- Reliability: Sensor data publishing should continue after transient Wi-Fi/MQTT outages.
- Maintainability: Configuration values (Wi-Fi, MQTT, API keys) shall be set in a config header (`config.h` or equivalent).
- Usability: Display should remain readable and update without noticeable lag.

## Hardware Requirements
- MCU: ESP8266 (D1 Mini recommended)
- Display: Adafruit ILI9341 TFT
- Touch Controller: XPT2046
- Sensors:
  - SGP30 (tVOC, eCO2)
  - SHT30 (Temperature, Humidity) at I2C address `0x45`
- Optional: Backlight dimming via PWM on TFT `BL/LED` pin.

## Wiring Requirements

### TFT Display (ILI9341 SPI)
- CS -> D0
- DC -> D8
- RST -> not connected (set `-1` in code)
- SCK -> D5
- MOSI -> D7
- MISO -> D6
- VCC -> 3.3V
- GND -> GND

### Touch Controller (XPT2046 SPI)
- CS -> D3
- SCK -> D5 (shared with TFT)
- MOSI -> D7 (shared with TFT)
- MISO -> D6 (shared with TFT)
- VCC -> 3.3V
- GND -> GND

### Sensors (I2C)
- SDA -> D2
- SCL -> D1
- SHT30 address -> `0x45`

## External Dependencies
- Adafruit GFX
- Adafruit ILI9341
- XPT2046 Touchscreen
- Adafruit SGP30
- SHT3X
- PubSubClient (MQTT)
- ArduinoOTA
- NTPClient
- ArduinoJson

## Configuration
- Wi-Fi credentials shall be configured in `config.h` or firmware source.
- MQTT broker details shall be configured in `config.h` or firmware source.
- OpenWeatherMap credentials are referenced in the README and shall be configured if used by firmware.

## Data Definitions
- Temperature: float, °C
- Humidity: float, % RH
- tVOC: integer or float, ppb
- eCO2: integer or float, ppm

## Acceptance Criteria
1. Device boots, connects to Wi-Fi, and obtains sensor readings.
2. TFT display shows live values for temperature, humidity, tVOC, and eCO2.
3. MQTT publishes values to all four specified topics.
4. Home Assistant auto-discovers all four sensors with correct units and device classes.
5. OTA update process completes successfully over Wi-Fi.

## Out of Scope
- Legacy 2020 Node-RED/InfluxDB/Grafana version located in `archive/`.
- Any additional sensors not listed in this specification.

