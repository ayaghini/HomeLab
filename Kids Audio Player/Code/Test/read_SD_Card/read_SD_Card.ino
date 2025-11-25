#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ------------------------------
// SD CARD CUSTOM PINS (your pins)
// ------------------------------
#define SD_SCK   36
#define SD_MISO  37
#define SD_MOSI  35
#define SD_CS     8

// ------------------------------
// ST7735 DISPLAY CUSTOM PINS
// ------------------------------
#define TFT_CS    5
#define TFT_RST   6
#define TFT_DC    9
#define TFT_MOSI 12
#define TFT_SCLK 11

// Create display object
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ----------------------------------
// MP3 filename storage
// ----------------------------------
#define MAX_FILES 50
String mp3Files[MAX_FILES];
int mp3Count = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ------------------------------
  // Init Display
  // ------------------------------
  tft.initR(INITR_144GREENTAB);   // works for 128x128 modules
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 0);
  tft.setTextSize(1);

  tft.println("Display OK");
  tft.println("Initializing SD...");

  // ------------------------------
  // Init SD with CUSTOM SPI
  // ------------------------------
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    tft.println("SD FAILED!");
    Serial.println("SD FAILED!");
    return;
  }

  tft.println("SD OK");
  Serial.println("SD OK");

  // Read MP3 files
  listMp3Files("/");

  // Show them on screen
  displayMp3List();
}

void loop() {
  // nothing here
}

// ------------------------------
// Read MP3 files from root
// ------------------------------
void listMp3Files(const char *dirname) {
  File root = SD.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("Root open failed");
    return;
  }

  mp3Count = 0;

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = String(file.name());
      String lower = name;
      lower.toLowerCase();

      if (lower.endsWith(".mp3")) {
        if (mp3Count < MAX_FILES) {
          mp3Files[mp3Count] = name;
          Serial.println("Found: " + name);
          mp3Count++;
        }
      }
    }
    file = root.openNextFile();
  }
}

// ------------------------------
// Display MP3 list on TFT
// ------------------------------
void displayMp3List() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);

  tft.println("MP3 Files:");
  tft.println("--------------------");

  for (int i = 0; i < mp3Count; i++) {
    tft.println(mp3Files[i]);
  }

  if (mp3Count == 0) {
    tft.println("No MP3 files found.");
  }
}
