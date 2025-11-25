# DIY Yoto Player with ESP32

My kid came home asking for a **Yoto**, and it seemed like a great idea — so why not DIY it?  

Update 1

The first hardware version is now complete
(/Images/IMG_8956.jpeg)

If you want to build your own:

All required parts are listed in this document.

The STL enclosure file is available under the CAD folder.

The enclosure is fully 3D-printable.
I printed mine on a Prusa and personalized it with my daughter’s name and her favorite animal (a cat).

Feel free to customize the model and add your loved one’s name or artwork to make it personal.

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


## SD Card

I am using the Adafruit microSD card breakout board:
[https://www.adafruit.com/product/4682](https://www.adafruit.com/product/4682)

### Wiring

| SD Pin | ESP32-S3 Pin          |
| ------ | --------------------- |
| CLK    | GPIO36 (SCK)          |
| SO     | GPIO37 (MISO)         |
| SI     | GPIO35 (MOSI)         |
| CS     | GPIO8 (A5 on Feather) |

### Initialization Code

Using the default Arduino `SD` library.
Below is the SPI setup that works and allows listing files:

```cpp
#define REASSIGN_PINS
int sck  = 36;
int miso = 37;
int mosi = 35;
int cs   = 8;

SPI.begin(sck, miso, mosi, cs);
SD.begin(cs);
```

With this setup, files on the SD card can be confirmed as readable.

---

## Rotary Encoder

Using the Adafruit I2C QT Rotary Encoder:
[https://learn.adafruit.com/adafruit-i2c-qt-rotary-encoder/arduino](https://learn.adafruit.com/adafruit-i2c-qt-rotary-encoder/arduino)

Loading the default Adafruit example sketch confirms the device works correctly.

The full application code (rotary, LCD, and SD handling) is stored in the project’s `code/` folder and functions correctly.

---

## MP3 Playback Using I²S

### Option 1 – ESP32-audioI2S

A popular library for simple MP3 playback:

[https://github.com/schreibfaul1/ESP32-audioI2S/tree/master](https://github.com/schreibfaul1/ESP32-audioI2S/tree/master)

This works, but users often struggle with how to properly implement the streaming loop. A useful reference:

[https://forum.arduino.cc/t/esp32-how-to-play-multiple-audio-files-from-sd/1039033/46](https://forum.arduino.cc/t/esp32-how-to-play-multiple-audio-files-from-sd/1039033/46)

#### Working Example

```cpp
#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"

#define SD_CS     5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

#define I2S_DOUT 26
#define I2S_BCLK 27
#define I2S_LRC  25

const char* audioFiles[] = {
  "/audio1.mp3",
  "/audio2.mp3"
};

int button1 = 12;
int button2 = 14;

Audio audio;

void setup() {
  pinMode(button1, INPUT_PULLDOWN);
  pinMode(button2, INPUT_PULLDOWN);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(1'000'000);

  Serial.begin(115200);
  SD.begin(SD_CS);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
}

void loop() {
  audio.loop();

  if (digitalRead(button1) == HIGH) {
    Serial.println("BUTTON 1 PRESSED");
    audio.setVolume(5);
    audio.connecttoFS(SD, audioFiles[0]);
  }

  if (digitalRead(button2) == HIGH) {
    Serial.println("BUTTON 2 PRESSED");
    audio.setVolume(5);
    audio.connecttoFS(SD, audioFiles[1]);
  }
}
```

---

## Option 2 – `arduino-audio-tools` (Recommended)

Library from Phil Schatzmann:

[https://github.com/pschatzmann/arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools)

### Installation

Clone into your Arduino libraries folder:

```bash
git clone https://github.com/pschatzmann/arduino-audio-tools.git
git clone https://github.com/pschatzmann/arduino-libhelix.git
```

Note: Depending on Arduino version, you may need to rename `MP3Decoder.h` if a conflict occurs.

### MAX98357A Wiring

Using the Adafruit MAX98357A:

| MAX98357A Pin | ESP32-S3 Pin |
| ------------- | ------------ |
| VIN           | 5V           |
| GND           | GND          |
| LRC           | GPIO16 (A2)  |
| BCLK          | GPIO15 (A3)  |
| DIN           | GPIO14 (A4)  |

### AudioTools I²S Setup

```cpp
auto config = i2s.defaultConfig(TX_MODE);
config.pin_ws   = 16;
config.pin_bck  = 15;
config.pin_data = 14;
i2s.begin(config);
```

### Working Example: MP3 From SD Card

```cpp
#include <SPI.h>
#include <SD.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

#define SD_SCK   36
#define SD_MISO  37
#define SD_MOSI  35
#define SD_CS     8

I2SStream i2s;
EncodedAudioStream decoder(&i2s, new MP3DecoderHelix());
StreamCopy copier;
File audioFile;

void setup() {
  Serial.begin(115200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);

  // Mount SD
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 4’000’000)) {
    while (1);
  }

  audioFile = SD.open("/lookingglass1.mp3");

  // Configure I2S
  auto config = i2s.defaultConfig(TX_MODE);
  config.pin_ws   = 16;
  config.pin_bck  = 15;
  config.pin_data = 14;
  i2s.begin(config);

  decoder.begin();
  copier.begin(decoder, audioFile);
}

void loop() {
  if (!copier.copy()) {
    // Finished playing
  }
}
```

---

# Summary

* SD breakout wired to custom SPI pins on ESP32-S3.
* Adafruit I2C rotary encoder confirmed working.
* MAX98357A amplifier works with both libraries.
* Both ESP32-audioI2S and arduino-audio-tools can play MP3s from SD over I²S.
