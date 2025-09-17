following this video:
https://www.youtube.com/watch?v=YmqtMTO5NVc

BME680 sensor: https://randomnerdtutorials.com/esp32-bme680-sensor-arduino/
ESP8266: https://randomnerdtutorials.com/getting-started-with-esp8266-wifi-transceiver-review/#esp8266-arduino-ide
 running example from Adafruit BME680 library the sensor works. 

 another good source: https://community.home-assistant.io/t/ikea-vindriktning-air-quality-sensor/324599

 Seems like a common aproach among hte community is to use esphome, I am going to gice it a try. 

 for esp home almost all goes as normal and default, for BME 680 though, look at the life saver post here:
 https://community.home-assistant.io/t/full-guide-to-read-bme680-via-bsec2-on-esp8266/778505

 this is the yaml file:
 esphome:
  name: ikea-aq-sensor
  friendly_name: Ikea AQ Sensor
  platformio_options:
    build_flags:
      - -DPIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED

esp8266:
  board: esp01_1m

# Enable logging
logger:

# Enable Home Assistant API
api:
  encryption:
    key: "3xvWUpZtHJHkEPjzQRUruf4ZNYPAUsYPhxdDGztg6Do="

ota:
  - platform: esphome
    password: "17542af9dcd98ff1ca6eb9740d3a8bf6"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

  # Enable fallback hotspot (captive portal) in case wifi connection fails
  ap:
    ssid: "Ikea-Aq-Sensor Fallback Hotspot"
    password: "StRhBzoV4FQh"

captive_portal:

i2c:

bme68x_bsec2_i2c:
  address: 0x77
  model: bme680
  operating_age: 28d
  sample_rate: LP
  supply_voltage: 3.3V

sensor:
  - platform: bme68x_bsec2
    temperature:
      name: "BME68x Temperature"
    pressure:
      name: "BME68x Pressure"
    humidity:
      name: "BME68x Humidity"
    iaq:
      name: "BME68x IAQ"
      id: iaq
    co2_equivalent:
      name: "BME68x CO2 Equivalent"
    breath_voc_equivalent:
      name: "BME68x Breath VOC Equivalent"

text_sensor:
  - platform: bme68x_bsec2
    iaq_accuracy:
      name: "BME68x IAQ Accuracy"
  - platform: template
    name: "BME68x IAQ Classification"
    lambda: |-
      if ( int(id(iaq).state) <= 50) {
        return {"Excellent"};
      }
      else if (int(id(iaq).state) >= 51 && int(id(iaq).state) <= 100) {
        return {"Good"};
      }
      else if (int(id(iaq).state) >= 101 && int(id(iaq).state) <= 150) {
        return {"Lightly polluted"};
      }
      else if (int(id(iaq).state) >= 151 && int(id(iaq).state) <= 200) {
        return {"Moderately polluted"};
      }
      else if (int(id(iaq).state) >= 201 && int(id(iaq).state) <= 250) {
        return {"Heavily polluted"};
      }
      else if (int(id(iaq).state) >= 251 && int(id(iaq).state) <= 350) {
        return {"Severely polluted"};
      }
      else if (int(id(iaq).state) >= 351) {
        return {"Extremely polluted"};
      }
      else {
        return {"error"};
      }