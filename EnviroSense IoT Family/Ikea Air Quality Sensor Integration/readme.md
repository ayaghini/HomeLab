# ESPHome IKEA Air Quality Sensor Setup

I originally bought this sensor back in 2021 but never finished the project. Now I finally have some time to get back to it.  

I followed this helpful video:  
👉 https://www.youtube.com/watch?v=YmqtMTO5NVc

---

### References
- BME680 sensor: https://randomnerdtutorials.com/esp32-bme680-sensor-arduino/  
- ESP8266: https://randomnerdtutorials.com/getting-started-with-esp8266-wifi-transceiver-review/#esp8266-arduino-ide  
- General IKEA Air Quality Sensor thread: https://community.home-assistant.io/t/ikea-vindriktning-air-quality-sensor/324599  
- Full BME680 + ESP8266 BSEC2 guide (life saver!):  
  https://community.home-assistant.io/t/full-guide-to-read-bme680-via-bsec2-on-esp8266/778505  

---

### Using ESPHome
It seems like the most common approach in the community is to use **ESPHome**, so I’m giving that a try. Most of the setup is straightforward, but for the **BME680** you’ll need to follow the BSEC2 guide above.

---

### Using ESP-01
I had a few **ESP-01** modules lying around, so I used one for this project.  
Keep in mind:
- Very basic module with only **2 usable GPIOs**  
- Operates only at **3.3V**  
- Both `3.3V` and `EN` pins must be tied to **3.3V**  

For **I²C connections**, I used this reference:  
👉 https://community.home-assistant.io/t/esp01-i2c/364059  

Configuration in ESPHome:
```yaml
i2c:
  sda: GPIO2
  scl: GPIO0
```

---

### Connecting to the IKEA Sensor
I used the **Rx pin** on the ESP-01 to connect to the IKEA **REST pin**.  

Configuration in ESPHome:
```yaml
uart:
  rx_pin: GPIO3
  baud_rate: 9600
```

---

### Final Note
The complete configuration can be found in the file:  
📄 `ikea_aq_sensor_esp01.yaml`
