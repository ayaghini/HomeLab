# DIY Yoto Player with ESP32

My kid came home asking for a **Yoto**, and it seemed like a great idea — so why not DIY it?  

---

## Step 1: Getting Started

I’m starting with the **Adafruit Feather ESP32-S3**.  
First, I uploaded the blink sketch to confirm the board works — ✅ success!  

Next, let’s move on to the **display**. I have a **1.44-inch 128x128 SPI display** on hand, so let’s see if we can get it working.

---

## Step 2: Display Module

**Module:** 1.44-inch SPI Module ST7735S  
**SKU:** MSP1443  
**Reference:** [LCD Wiki - 1.44-inch SPI Module ST7735S](https://www.lcdwiki.com/1.44inch_SPI_Module_ST7735S_SKU:MSP1443)

---

## Step 3: Planning for Audio

Later on, I plan to use the **I2S** capabilities of the ESP32 for audio output.  
A great read on that topic: [ESP32 I2S Explained by DroneBot Workshop](https://dronebotworkshop.com/esp32-i2s/)  

It looks like we can leverage **multiplexing** and choose whichever pins we prefer.

---

## Step 4: Display Pinout

Here’s how I connected the display to the ESP32:

| Pin | Function | ESP32 Pin |
|-----|-----------|------------|
| 5   | CS        | Chip Select |
| 6   | RESET     | Reset |
| 9   | A0 / DC   | Data/Command |
| 10  | LED       | Backlight |
| 11  | SCK       | Clock |
| 12  | SDA       | Data |

---

## Step 5: Code Changes (Adafruit ST7735 Library)

Using the **Adafruit ST7735** library example, I confirmed the display works with the following changes:

```cpp
#else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS   5
  #define TFT_RST  6  // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC   9
#endif

#define TFT_MOSI  12  // Data out
#define TFT_SCLK  11  // Clock out

// For ST7735-based displays, we will use this call
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
```

---

## ✅ Result

The display works! 🎉  
Next step: move on to **I2S audio playback** and integrate it into a **DIY Yoto-style player**.

---

**To be continued...**
