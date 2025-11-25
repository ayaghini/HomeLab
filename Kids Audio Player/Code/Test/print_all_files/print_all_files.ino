#include <SD.h>
#include <SPI.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// -------------------------------------
// SD CARD PINS
// -------------------------------------
#define SD_SCK 36
#define SD_MISO 37
#define SD_MOSI 35
#define SD_CS 8

// -------------------------------------
// TFT DISPLAY PINS
// -------------------------------------
#define TFT_CS 5
#define TFT_RST 6
#define TFT_DC 9
#define TFT_MOSI 12
#define TFT_SCLK 11

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
 Serial.begin(115200);
  delay(300);

  // ----- Init Display -----
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Init SD...");

  // ----- Init SD -----
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS, SPI, 4000000)) {
    tft.println("SD FAIL!");
    while (1)
      ;
  }

  Serial.println("SD OK!");

  File root = SD.open("/");
  while (true) {
    File file = root.openNextFile();
    if (!file) break;
    Serial.println(file.name());
    tft.println(file.name());
  }
}

void loop() {}
