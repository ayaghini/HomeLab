# EnviroSense Touch Functional Specification

## Purpose
EnviroSense Touch is a Wi-Fi connected environmental sensing device that measures temperature, humidity, tVOC, and eCO2. It provides a local TFT touchscreen display for live visualization and publishes telemetry over MQTT with Home Assistant (HA) auto-discovery support. It also supports OTA firmware updates for easy maintenance.

## Scope
This specification covers the device behavior, telemetry, display, time/weather features, user interaction, and integration requirements for the 2025 Home Assistant/MQTT version of EnviroSense Touch. The legacy 2020 implementation in `archive/` is out of scope.

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
- The display shall show time and a weather summary when available.
- The device shall blank the screen between 19:00 and 06:00 local time to reduce light, unless overridden by a touch event.
- A touch event shall temporarily force the display on for 5 seconds.
- Touch input shall be supported through XPT2046 for any UI interactions supported by firmware (currently used for display override only).
- The UI shall provide a more visually engaging layout with improved typography hierarchy, color palette, and iconography.
- The UI shall include a clear status area for Wi-Fi/MQTT connectivity and sensor health.
- The UI shall support smooth, non-blocking refreshes with partial redraws where possible to minimize flicker.
- Touching a sensor tile shall switch to a 24-hour history view for that metric.
- The history view shall display samples captured at 30-minute intervals (48 samples total).
- Touching anywhere on the history view shall return to the main dashboard.

### 3) Wi-Fi Connectivity
- The device shall connect to a configured Wi-Fi network at boot.
- The device shall maintain Wi-Fi connectivity and retry on failure.

### 4) MQTT Telemetry
- The device shall publish sensor readings to MQTT using the following topics:
  - `EnviroSenseTouch/ENVT_temperature` (°C)
  - `EnviroSenseTouch/ENVT_humidity` (% RH)
  - `EnviroSenseTouch/ENVT_tVoc` (ppb)
  - `EnviroSenseTouch/ENVT_equivalentCO2` (ppm)
- The device shall publish readings at a regular interval (default 30 seconds).
- MQTT payloads shall be numeric strings and published as retained messages.

### 5) Home Assistant Discovery
- The device shall publish MQTT Discovery messages so sensors auto-appear in HA.
- Sensor entities shall include correct units and device classes for temperature, humidity, tVOC, and eCO2.
- Discovery messages shall be published as retained messages to the following config topics:
  - `homeassistant/sensor/envirosense_touch_temperature/config`
  - `homeassistant/sensor/envirosense_touch_humidity/config`
  - `homeassistant/sensor/envirosense_touch_tvoc/config`
  - `homeassistant/sensor/envirosense_touch_eco2/config`

### 6) OTA Updates
- The device shall support OTA firmware updates via ArduinoOTA.
- OTA updates shall be available when the device is connected to Wi-Fi.

### 7) Time Synchronization
- The device shall obtain time using NTP (`pool.ntp.org`) and display local time.
- The device shall use a configured UTC offset (default UTC-7) for display timing.

### 8) Weather Integration
- The device shall optionally fetch weather data from OpenWeatherMap.
- Weather requests shall be made approximately every 30 minutes.
- The display shall show a textual weather condition and a temperature value (feels-like).

## Non-Functional Requirements
- Reliability: Sensor data publishing should continue after transient Wi-Fi/MQTT outages.
- Maintainability: Configuration values (Wi-Fi, MQTT, API keys) shall be set in a config header (`config.h` or equivalent).
- Usability: Display should remain readable and update without noticeable lag.
- Responsiveness: The main loop shall not block OTA handling for more than a few seconds.

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
- Wi-Fi credentials shall be configured in `secrets.h` or firmware source.
- MQTT broker details shall be configured in `secrets.h` or firmware source.
- OpenWeatherMap credentials (`apiKey`, `nameOfCity`) shall be configured if weather is enabled.
- The MQTT client ID and OTA hostname shall be configurable (default `EnviroSenseTouch` and `Enviro_Touch`).

## Data Definitions
- Temperature: float, °C
- Humidity: float, % RH
- tVOC: integer or float, ppb
- eCO2: integer or float, ppm
- Weather condition: string (e.g., `CLEAR`, `CLOUDY`, `RAIN`)
- Weather temperature: float, °C (feels-like)

## Acceptance Criteria
1. Device boots, connects to Wi-Fi, and obtains sensor readings.
2. TFT display shows live values for temperature, humidity, tVOC, and eCO2.
3. Time is displayed and updates via NTP.
4. Weather data is shown when OpenWeatherMap credentials are provided.
5. MQTT publishes values to all four specified topics at ~30-second intervals as retained messages.
6. Home Assistant auto-discovers all four sensors with correct units and device classes.
7. OTA update process completes successfully over Wi-Fi.

## Identified Improvements (from code review)
- Fix Home Assistant discovery JSON for temperature to include a properly quoted `"name"` field.
- Publish discovery messages with `device` metadata and unique IDs so HA can group sensors as a single device.
- Avoid blocking behavior in `reconnectMQTT()` to keep OTA handling responsive; use non-blocking retries.
- Call `mqttClient.loop()` continuously, not only on publish intervals, to maintain the connection.
- Remove the duplicate `ArduinoOTA.begin()` call.
- Fix `MeasureSht()` to correctly update `lastTemp/lastHumid` and remove the no-op line `temp, humid, lastTemp, lastHumid;`.
- Guard `Difrentiator()` against divide-by-zero when `a` is zero.
- Add basic MQTT LWT (availability) and publish status topic for health monitoring.
- Add configurable display sleep window and brightness control (PWM backlight).
- Add Wi-Fi/MQTT reconnection backoff and a reboot-on-stall watchdog.
- Cache weather results and draw icons for all supported conditions (including DRIZZLE and ATMOSPHERE).
- Improve UI visual design with a defined color system, icon set, typography scale, and layout grid to reduce clutter and improve legibility.
- Add subtle animation or transition on value changes (e.g., brief highlight or fade) without blocking the main loop.
- Fix touch wake behavior by properly reading touch coordinates and debouncing touches before setting the display override timer.
- Keep the display awake while touch is detected and extend the override timer on any valid touch input.
- Add a non-blocking Wi-Fi connection attempt with timeout and offline mode when credentials are not reachable.
- Provide visible connectivity status indicators on the UI for Wi-Fi and MQTT.
- Add a local on-device history buffer and history chart UI for 24-hour trends.

## Implementation Notes (Rev01)
- Added visual refresh and UI theme improvements with a header/status area and 2x2 data tiles.
- Implemented touch debounce and immediate display wake behavior.
- Added Wi-Fi connection timeout with diagnostic reporting and offline continuation.
- Added `secrets.example.h` template for safe sharing in GitHub.
- Implemented on-device 24-hour history (30-minute sampling) with touch-to-view charts.

## Out of Scope
- Legacy 2020 Node-RED/InfluxDB/Grafana version located in `archive/`.
- Any additional sensors not listed in this specification.
