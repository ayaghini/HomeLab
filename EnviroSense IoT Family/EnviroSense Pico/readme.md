# EnviroSense Pico

EnviroSense Pico is an air quality monitoring project using a BME680 sensor and ESP8266. It reads temperature, humidity, pressure, and gas resistance, then publishes the data to **Home Assistant** via MQTT, allowing you to visualize it easily on your dashboard.

---

## Features

* Reads **temperature, humidity, pressure, and gas resistance** from BME680.
* Publishes data via **MQTT** to Home Assistant.
* Supports **Home Assistant MQTT Discovery** for automatic dashboard integration.
* Lightweight code optimized for ESP8266.
* Configurable **display output** with an 80×160 TFT.
* Debugging output via Serial Monitor.
* Previously used Node-RED, InfluxDB, and Grafana (archived).

---

## Hardware

* **Microcontroller:** ESP8266 (e.g., NodeMCU, Wemos D1 Mini)
* **Sensor:** BME680 (temperature, humidity, pressure, gas)
* **Display:** 80×160 TFT (ST7735)
* **Power supply:** 5 V (USB)

Optional:

* Raspberry Pi with Home Assistant for MQTT broker.

---

## Wiring

**BME680:**

| Pin | ESP8266 Pin |
| --- | ----------- |
| VIN | 3.3 V       |
| GND | GND         |
| SCL | D1          |
| SDA | D2          |

**TFT Display (ST7735):**

| Pin  | ESP8266 Pin |
| ---- | ----------- |
| CS   | D3          |
| DC   | D4          |
| MOSI | D7          |
| SCLK | D5          |
| RST  | RX          |

---

## Software Setup

1. Install the following Arduino libraries:

   * `Adafruit BME680` or `bsec` for gas/IAQ readings
   * `Adafruit ST7735` for TFT display
   * `ESP8266WiFi` for WiFi
   * `PubSubClient` for MQTT

2. Configure your WiFi and MQTT credentials in `secrets.h` (not committed to Git):

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqtt_server = "MQTT_BROKER_IP";
const int mqtt_port = 1883;
const char* mqtt_user = "MQTT_USER";
const char* mqtt_pass = "MQTT_PASS";
```

3. Upload the code to your ESP8266.

4. Ensure Home Assistant is running with the MQTT broker enabled.

---

## Usage

* The device reads sensor data every 2 seconds and updates the TFT display if values change beyond set thresholds.
* Sensor readings are published to MQTT topics:

  * `EnviroSense/temperature`
  * `EnviroSense/humidity`
  * `EnviroSense/pressure`
  * `EnviroSense/gas`
  * `EnviroSense/aqi`
* Home Assistant automatically discovers the sensors via MQTT discovery.
* Check the Serial Monitor for debugging output.

---

## Notes

* The BME680 temperature may read slightly higher due to **self-heating**; a manual offset is applied in the code.
* Gas readings are used as a proxy for AQI (Air Quality Index).
* For battery-powered applications, consider reducing reading frequency to save power.
* The previous Node-RED + InfluxDB + Grafana setup is archived in the `archive` folder.

---

## License

This project is open-source. Feel free to modify and share under your preferred license.
