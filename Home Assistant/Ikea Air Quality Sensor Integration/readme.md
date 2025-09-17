# ESPHome IKEA Air Quality Sensor Setup

I bought this back in 2021 and never got to finish the project, now I have some time to finally get back to this.

Following this video:  
https://www.youtube.com/watch?v=YmqtMTO5NVc

### References
- BME680 sensor: https://randomnerdtutorials.com/esp32-bme680-sensor-arduino/  
- ESP8266: https://randomnerdtutorials.com/getting-started-with-esp8266-wifi-transceiver-review/#esp8266-arduino-ide  
- Another good source: https://community.home-assistant.io/t/ikea-vindriktning-air-quality-sensor/324599  

It seems like a common approach among the community is to use ESPHome, so I’m going to give it a try.

For ESPHome almost all goes as normal and default, but for the BME680, look at this life saver post:  
https://community.home-assistant.io/t/full-guide-to-read-bme680-via-bsec2-on-esp8266/778505

