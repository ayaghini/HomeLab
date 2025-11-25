#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_seesaw.h>

// -------------------------------------
// SD CARD PINS (your config)
// -------------------------------------
#define SD_SCK   36
#define SD_MISO  37
#define SD_MOSI  35
#define SD_CS     8

// -------------------------------------
// TFT DISPLAY PINS (your config)
// -------------------------------------
#define TFT_CS    5
#define TFT_RST   6
#define TFT_DC    9
#define TFT_MOSI 12
#define TFT_SCLK 11

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// -------------------------------------
// SEESAW ROTARY ENCODER
// -------------------------------------
Adafruit_seesaw ss;
#define SEESAW_ADDR 0x36     // default address

int32_t lastEncoderPos = 0;
bool lastButtonState = true;

// -------------------------------------
// MP3 LIST
// -------------------------------------
#define MAX_FILES 50
String mp3Files[MAX_FILES];
int mp3Count = 0;
int selectedIndex = 0;   // current highlighted item


// --------------------------- SETUP -------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  // ----- Init Display -----
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.println("Init SD...");

  // ----- Init SD -----
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 4000000)) {
    tft.println("SD FAIL!");
    while (1);
  }
  tft.println("SD OK");

  // Load mp3 list
  listMp3Files("/");
  displayMp3List();

  // ----- Init Seesaw Encoder -----
  if (!ss.begin(SEESAW_ADDR)) {
    Serial.println("ERROR: seesaw not found");
    tft.println("Encoder FAIL!");
    while (1);
  }

  // Enable encoder knob
  ss.pinMode(24, INPUT_PULLUP);
  lastButtonState = ss.digitalRead(24);

  // Reset encoder position
  ss.setEncoderPosition(0);
  lastEncoderPos = 0;

  tft.println("Encoder OK");
}



// --------------------------- LOOP -------------------------------
void loop() {
  handleEncoder();
}



// --------------------------- ENCODER HANDLER ---------------------
void handleEncoder() {
  int32_t encoderPos = ss.getEncoderPosition();

  // MOVEMENT (scroll)
  if (encoderPos != lastEncoderPos) {
    int delta = -encoderPos + lastEncoderPos;
    lastEncoderPos = encoderPos;

    selectedIndex -= delta;   // this feels natural
    if (selectedIndex < 0) selectedIndex = 0;
    if (selectedIndex >= mp3Count) selectedIndex = mp3Count - 1;

    displayMp3List();
  }

  // BUTTON PRESS
  bool buttonState = ss.digitalRead(24);
  if (!buttonState && lastButtonState) {
    Serial.print("SELECTED: ");
    Serial.println(mp3Files[selectedIndex]);

    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.println("Selected:");
    tft.println(mp3Files[selectedIndex]);

    // You can call playMP3(selectedIndex) here next step
  }

  lastButtonState = buttonState;
}



// --------------------------- MP3 LIST ----------------------------
void displayMp3List() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);

  tft.println(" MP3 Files:");
  tft.println("----------------");

  for (int i = 0; i < mp3Count; i++) {
    if (i == selectedIndex) {
      tft.setTextColor(ST77XX_YELLOW, ST77XX_BLUE);  // highlighted
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    }

    tft.println(mp3Files[i]);
  }

  // restore
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
}



// --------------------------- FIND MP3 ----------------------------
void listMp3Files(const char *dirname) {
  File root = SD.open(dirname);
  mp3Count = 0;

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = file.name();
      String low = name;
      low.toLowerCase();
      if (low.endsWith(".mp3")) {
        mp3Files[mp3Count] = name;
        mp3Count++;
        if (mp3Count >= MAX_FILES) break;
      }
    }
    file = root.openNextFile();
  }
}
