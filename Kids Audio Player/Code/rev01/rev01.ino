#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_seesaw.h>
#include <TJpg_Decoder.h>
//#include <JpgDecoder.h>

#include <AudioTools.h>
#include <AudioTools/Disk/AudioSourceSD.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>

#include <Wire.h>
#include "Adafruit_MAX1704X.h"
//#include "Adafruit_LC709203F.h"

// -------------------------------------
// BATTERY
// -------------------------------------
Adafruit_MAX17048 maxlipo;
//Adafruit_LC709203F lc;
bool addr0x36 = true;
bool batteryOk = false;
bool i2cHasMax17048 = false;
bool i2cHasLc709203 = false;

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

#define TFT_W 128
#define TFT_H 128
const int LINE_H = 11;
const int LIST_TOP = 18;
const int LIST_LEFT = 4;
const int ITEMS_PER_PAGE = 9;

// UI colors
uint16_t UI_BG;
uint16_t UI_PANEL;
uint16_t UI_TEXT;
uint16_t UI_SUBTEXT;
uint16_t UI_ACCENT;
uint16_t UI_HIGHLIGHT;

// -------------------------------------
// SEESAW ROTARY ENCODER
// -------------------------------------
Adafruit_seesaw ss;
#define SEESAW_ADDR 0x36

int32_t lastEncoderPos = 0;
bool lastButtonState = false;
bool buttonState = false;
unsigned long lastButtonChangeMs = 0;
const unsigned long buttonDebounceMs = 120;

// -------------------------------------
// MP3 LIST
// -------------------------------------
#define MAX_FILES 50
String mp3Files[MAX_FILES];
int mp3Count = 0;
int selectedIndex = 0;
int scrollOffset = 0;

// -------------------------------------
// AUDIO PLAYBACK
// -------------------------------------
I2SStream i2s;         // output
MP3DecoderHelix mp3;   // decoder
AudioSourceSD source;  // SD card audio
VolumeStream volume(i2s);
AudioPlayer player(source, i2s, mp3);  // The main player
float volumeLevel = 0.3;
// Volume change (we'll apply immediately from main loop; if problematic, send requests)
volatile float audioVolumeRequested = -1.0f;  // -1 means no request

enum PlaybackState { STOPPED = 0,
                     PLAYING = 1,
                     PAUSED = 2 };

volatile PlaybackState playbackState = STOPPED;
volatile bool audioPlayRequested = false;
String audioPath;
// For safety copy path to a C-string buffer that the audio task will use
char audioPathBuf[256] = { 0 };  // ensure large enough path support

// Request flags (main sets; audioTask consumes)
//volatile bool audioPlayRequested = false;
volatile bool audioStopRequested = false;
volatile bool audioPauseRequested = false;
volatile bool audioResumeRequested = false;
volatile bool uiNeedsRefresh = false;

// Create a Task object
// name = "audio", stack size = e.g. 10 000, priority = 1, core = 0 (or 1)
audio_tools::Task audioTask("audio", 12000, 1, 0);

void startStop(bool, int, void*) {
  player.setActive(!player.isActive());
}

// Callback used by TJpg_Decoder to draw blocks to the TFT
bool outputJpgCallback(int16_t x, int16_t y,
                       uint16_t w, uint16_t h,
                       uint16_t* bitmap) {
  // Push directly to the TFT
  tft.drawRGBBitmap(x, y, bitmap, w, h);
  return true;
}

void initI2C() {
#if defined(TFT_I2C_POWER)
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
#elif defined(PIN_I2C_POWER)
  pinMode(PIN_I2C_POWER, OUTPUT);
  digitalWrite(PIN_I2C_POWER, HIGH);
#endif
  Wire.begin(SDA, SCL);
}

void scanI2C() {
  i2cHasMax17048 = false;
  i2cHasLc709203 = false;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      if (addr == 0x36) i2cHasMax17048 = true;
      if (addr == 0x0B) i2cHasLc709203 = true;
    }
  }

  Serial.print("[I2C] MAX17048: ");
  Serial.println(i2cHasMax17048 ? "found" : "not found");
  Serial.print("[I2C] LC709203: ");
  Serial.println(i2cHasLc709203 ? "found" : "not found");
}

// This is the function that the task runs
// === Audio Task: single owner of playbackState and player ===
void audioTaskLoop() {
  for (;;) {

    //
    // 1) PLAY REQUEST
    //
    if (audioPlayRequested) {
      audioPlayRequested = false;

      Serial.printf("[AudioTask] Received play request for: %s\n", audioPathBuf);

      // Stop any current playback just in case
      //player.end();

      //
      // Set source and prepare player
      //
      player.setPath(audioPathBuf);  // non-blocking
      //player.begin();                   // non-blocking init

      playbackState = PLAYING;

      Serial.println("[AudioTask] Player started (non-blocking)");

      vTaskDelay(pdMS_TO_TICKS(2));
    }


    //
    // 2) STOP REQUEST
    //
    if (audioStopRequested) {
      audioStopRequested = false;

      if (playbackState == PLAYING || playbackState == PAUSED) {
        Serial.println("[AudioTask] Stopping playback");

        player.end();
        playbackState = STOPPED;
        uiNeedsRefresh = true;
      }

      vTaskDelay(pdMS_TO_TICKS(2));
    }


    //
    // 3) PAUSE REQUEST
    //
    if (audioPauseRequested) {
      audioPauseRequested = false;

      if (playbackState == PLAYING) {
        Serial.println("[AudioTask] Pausing playback");

        // No built-in pause in AudioTools, so we just stop copying
        playbackState = PAUSED;
      }

      vTaskDelay(pdMS_TO_TICKS(2));
    }


    //
    // 4) RESUME REQUEST
    //
    if (audioResumeRequested) {
      audioResumeRequested = false;

      if (playbackState == PAUSED) {
        Serial.println("[AudioTask] Resuming playback");

        playbackState = PLAYING;
      }

      vTaskDelay(pdMS_TO_TICKS(2));
    }


    //
    // 5) MAIN COPY LOOP – NON BLOCKING
    //
    if (playbackState == PLAYING) {

      if (player.isActive()) {

        // Perform one chunk of decode + I2S output
        player.copy();

        // A short delay frees CPU so FreeRTOS stays happy
        vTaskDelay(pdMS_TO_TICKS(1));

      } else {
        // Playback finished naturally
        Serial.println("[AudioTask] Playback completed");
        player.stop();
        playbackState = STOPPED;
        uiNeedsRefresh = true;
      }
    }

    //
    // If nothing happening, idle a bit
    //
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}



// --------------------------- SETUP -------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

  initI2C();
  scanI2C();

  // ----- Init Display -----
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(2);
  initUiColors();
  tft.fillScreen(UI_BG);
  tft.setTextColor(UI_TEXT);
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
  //tft.println("SD OK");
  // Configure JPG decoder
  TJpgDec.setJpgScale(1);                  // full scale
  TJpgDec.setCallback(outputJpgCallback);  // register our drawing callback

  // Show startup image
  drawJpgFromSD("/startup.jpg");
  delay(3000);

  // Load mp3 list
  listMp3Files("/");
  displayMp3List();

  // ----- Init Seesaw Encoder -----
  if (!ss.begin(SEESAW_ADDR)) {
    tft.println("Encoder FAIL!");
    while (1)
      ;
  }
  ss.pinMode(24, INPUT_PULLUP);
  lastButtonState = ss.digitalRead(24);
  ss.setEncoderPosition(0);
  lastEncoderPos = 0;

  tft.println("Encoder OK");

  // ----- Init Audio I2S -----
  auto config = i2s.defaultConfig(TX_MODE);
  config.pin_ws = 16;
  config.pin_bck = 15;
  config.pin_data = 14;
  i2s.begin(config);

  // ---------- VOLUME --------------
  auto config_vol = volume.defaultConfig();
  config_vol.volume = 0.5;  // half the volume
  volume.begin(config_vol);
  //volume.begin();
  player.setVolume(volumeLevel);

  // ---------- PLAYER --------------
  player.begin();
  // Start the audio task
  audioTask.begin([]() {
    audioTaskLoop();
  });
  // ----- Init Battery Gauge -----
  if (i2cHasMax17048) {
    batteryOk = maxlipo.begin();
  } else {
    batteryOk = false;
  }
  if (batteryOk) {
    addr0x36 = true;
    Serial.print(F("Found MAX17048 with Chip ID: 0x"));
    Serial.println(maxlipo.getChipID(), HEX);
  } else {
    Serial.println(F("MAX17048 not detected."));
  }
  Serial.println("Player Ready");
}



// --------------------------- LOOP -------------------------------
void loop() {
  handleEncoder();
  if (uiNeedsRefresh && playbackState == STOPPED) {
    uiNeedsRefresh = false;
    displayMp3List();
  }
  // // 2) If a playback was requested, start it
  // if (playRequested) {
  //   playRequested = false;

  //   if (player.playPath(playPathStr.c_str())) {
  //     // start playing from beginning
  //     player.begin(0, true);
  //   } else {
  //     Serial.println("Error: could not play path");
  //     // optionally inform user on TFT
  //   }
  // }

  // // 3) Drive the playback pipeline
  // if (player.isActive()) {
  //   player.copy();
  // } else {
  //   // playback finished or nothing playing
  //   // Optionally: you can show "done" message, or allow UI again
  // }

  // // 4) Your other UI / logic code here, e.g. encoder navigation
}

void initUiColors() {
  UI_BG = tft.color565(10, 14, 24);
  UI_PANEL = tft.color565(18, 24, 40);
  UI_TEXT = ST77XX_WHITE;
  UI_SUBTEXT = tft.color565(140, 150, 170);
  UI_ACCENT = tft.color565(255, 180, 60);
  UI_HIGHLIGHT = tft.color565(30, 120, 220);
}

int getBatteryPercent() {
  if (!batteryOk) return -1;
  float percent = maxlipo.cellPercent();
  if (percent < 0) return -1;
  return (int)percent;
}

void drawHeaderBar(const char* title) {
  tft.fillRect(0, 0, TFT_W, 14, UI_PANEL);
  tft.setTextColor(UI_TEXT);
  tft.setTextSize(1);
  tft.setCursor(4, 3);
  tft.print(title);

  int batt = getBatteryPercent();
  tft.setTextColor(UI_SUBTEXT);
  tft.setCursor(92, 3);
  if (batt >= 0) {
    tft.print(batt);
    tft.print("%");
  } else {
    tft.print("--%");
  }
}

void drawPlaceholderArt(const String& title) {
  tft.fillScreen(UI_BG);
  drawHeaderBar("Now Playing");
  tft.drawRoundRect(10, 20, 108, 90, 8, UI_SUBTEXT);
  tft.setTextColor(UI_SUBTEXT);
  tft.setCursor(18, 60);
  tft.print("No Artwork");
  tft.setTextColor(UI_TEXT);
  tft.setCursor(10, 112);
  tft.setTextSize(1);
  tft.print(title);
}

// Draw JPG from SD to full screen
bool drawJpgFromSD(const char* filename) {
  Serial.print("Loading JPG: ");
  Serial.println(filename);

  if (!SD.exists(filename)) {
    Serial.println("File not found!");
    return false;
  }

  // Get JPEG dimensions
  uint16_t jpgWidth, jpgHeight;
  TJpgDec.getSdJpgSize(&jpgWidth, &jpgHeight, filename);


  Serial.print("JPG size: ");
  Serial.print(jpgWidth);
  Serial.print("x");
  Serial.println(jpgHeight);

  // Choose the largest scale factor that still fits 128x128
  uint8_t scaleFactor = 1;
  if (jpgWidth > 128 || jpgHeight > 128) {
    if (jpgWidth / 2 <= 128 && jpgHeight / 2 <= 128) scaleFactor = 2;
    else if (jpgWidth / 4 <= 128 && jpgHeight / 4 <= 128) scaleFactor = 4;
    else scaleFactor = 8;
  }

  TJpgDec.setJpgScale(scaleFactor);  // use library scaling

  Serial.print("Using scale factor: ");
  Serial.println(scaleFactor);

  // Clear screen
  tft.fillScreen(ST77XX_BLACK);

  // Draw the JPG at top-left (0,0).
  // TJpgDec automatically clips if too large.
  TJpgDec.drawSdJpg(0, 0, filename);

  Serial.println("JPG render complete.");
  return true;
}

// --------------------------- ENCODER HANDLER ---------------------
void handleEncoder() {
  int32_t encoderPos = ss.getEncoderPosition();
  //------------------------------------------------------------
  // ROTARY MOVEMENT
  //------------------------------------------------------------
  if (encoderPos != lastEncoderPos) {
    int delta = encoderPos - lastEncoderPos;  // correct direction
    lastEncoderPos = encoderPos;

    if (playbackState == PLAYING) {
      // When playing → use rotary for VOLUME
      adjustVolume(delta);
    } else {
      // STOPPED or PAUSED → scroll file list
      UpdateDisplayListAndPosition(delta);
    }
  }

  //------------------------------------------------------------
  // BUTTON PRESS
  //------------------------------------------------------------
  bool currentButton = ss.digitalRead(24);  // LOW = pressed
  if (currentButton != lastButtonState && (millis() - lastButtonChangeMs) > buttonDebounceMs) {
    lastButtonChangeMs = millis();
    if (currentButton == LOW && lastButtonState == HIGH) {
      handleEncoderPress();
    }
    lastButtonState = currentButton;
  }
}

void UpdateDisplayListAndPosition(int d) {
  selectedIndex += d;
  if (selectedIndex < 0) selectedIndex = 0;
  if (selectedIndex >= mp3Count) selectedIndex = mp3Count - 1;
  if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
  if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
    scrollOffset = selectedIndex - ITEMS_PER_PAGE + 1;
  }
  if (scrollOffset < 0) scrollOffset = 0;
  displayMp3List();
}

// Helper: convert enum to readable text
const char* playbackStateName(PlaybackState st) {
  switch (st) {
    case STOPPED: return "STOPPED";
    case PLAYING: return "PLAYING";
    case PAUSED: return "PAUSED";
    default: return "UNKNOWN";
  }
}

void handleEncoderPress() {
  Serial.print("[Encoder Press] Current playbackState = ");
  Serial.println(playbackStateName(playbackState));

  if (playbackState == STOPPED) {
    // --- REQUEST PLAYBACK ---
    String s = "/" + mp3Files[selectedIndex];
    Serial.print("[Encoder Press] Selected file: ");
    Serial.println(s);

    // Serialize the path into the global buffer
    if (s.length() < sizeof(audioPathBuf)) {
      s.toCharArray(audioPathBuf, sizeof(audioPathBuf));
    } else {
      Serial.println("[ERROR] audioPathBuf overflow risk!");
    }

    audioPlayRequested = true;

    Serial.println("[Encoder Press] -> audioPlayRequested = TRUE");

    //UI
    String jpgPath = getJpgPath(s);
    Serial.println("Corresponding JPG file: " + jpgPath);

    if (!drawJpgFromSD(jpgPath.c_str())) {
      drawPlaceholderArt(mp3Files[selectedIndex]);
    }


    // UI
    // tft.fillScreen(ST77XX_BLACK);
    // tft.setCursor(0, 0);
    // tft.setTextColor(ST77XX_GREEN);
    // tft.println("Play ->");
    // tft.setTextColor(ST77XX_WHITE);
    // tft.println(mp3Files[selectedIndex]);

  } else {

    // --- REQUEST STOP ---
    audioStopRequested = true;
    Serial.println("[Encoder Press] -> audioStopRequested = TRUE (stop playback)");
    uiNeedsRefresh = true;

    // Optional: local UI update
    // displayMp3List();
  }
}

String getJpgPath(const String& mp3Path) {
  // Copy the original string
  String jpgPath = mp3Path;

  // Find the last '.' to replace the extension
  int dotIndex = jpgPath.lastIndexOf('.');
  if (dotIndex != -1) {
    jpgPath = jpgPath.substring(0, dotIndex);  // Remove extension
  }

  // Add .jpg
  jpgPath += ".jpg";

  return jpgPath;
}

void adjustVolume(int direction) {
  volumeLevel += direction * 0.03;  // 2% step
  volumeLevel = constrain(volumeLevel, 0.0, 1.0);

  player.setVolume(volumeLevel);
  int barX = 6;
  int barY = TFT_H - 10;
  int barW = TFT_W - 12;
  int barH = 6;
  tft.fillRect(barX, barY, barW, barH, UI_PANEL);
  int fillW = (int)(barW * volumeLevel);
  tft.fillRect(barX, barY, fillW, barH, UI_ACCENT);

  Serial.printf("Volume = %.2f\n", volumeLevel);
}

void updateNowPlayingDisplay() {
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.println(mp3Files[selectedIndex]);

  tft.setCursor(0, 20);
  switch (playbackState) {
    case PLAYING:
      tft.setTextColor(ST77XX_GREEN);
      tft.println("Playing");
      break;
    case PAUSED:
      tft.setTextColor(ST77XX_YELLOW);
      tft.println("Paused");
      break;
    case STOPPED:
      tft.setTextColor(ST77XX_RED);
      tft.println("Stopped");
      break;
  }
}

String formatTrackName(const String& filename) {
  String name = filename;
  int dot = name.lastIndexOf('.');
  if (dot > 0) {
    name = name.substring(0, dot);
  }

  // Trim to fit screen width (no wrap)
  const int maxWidth = TFT_W - (LIST_LEFT + 2);
  String out = name;
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(out, 0, 0, &x1, &y1, &w, &h);
  if (w <= maxWidth) return out;

  // Truncate and add "..."
  while (out.length() > 3) {
    out.remove(out.length() - 1);
    String test = out + "...";
    tft.getTextBounds(test, 0, 0, &x1, &y1, &w, &h);
    if (w <= maxWidth) {
      return test;
    }
  }
  return out;
}

// --------------------------- PLAY MP3 ----------------------------
void playMP3(int index) {
  audioPath = "/" + mp3Files[index];
  audioPlayRequested = true;
  playbackState = STOPPED;  // reset previous state

  // Update TFT
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_GREEN);
  tft.println("Play ->");
  tft.setTextColor(ST77XX_WHITE);
  tft.println(mp3Files[index]);
}

void stopPlayback() {
  if (playbackState == PLAYING || playbackState == PAUSED) {
    //player.end();
    player.stop();
    playbackState = STOPPED;
  }
}

void pausePlayback() {
  if (playbackState == PLAYING) {
    playbackState = PAUSED;
    tft.setCursor(0, 20);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println("Paused");
  }
}

void resumePlayback() {
  if (playbackState == PAUSED) {
    playbackState = PLAYING;
    tft.setCursor(0, 20);
    tft.setTextColor(ST77XX_GREEN);
    tft.println("Resuming");
  }
}



// ------------------------ DISPLAY LIST ---------------------------
void displayMp3List() {
  tft.fillScreen(UI_BG);
  drawHeaderBar("YANA STORIES");

  if (mp3Count == 0) {
    tft.setTextColor(UI_SUBTEXT);
    tft.setCursor(LIST_LEFT, LIST_TOP);
    tft.print("No MP3 files");
    return;
  }

  int start = scrollOffset;
  int end = min(mp3Count, start + ITEMS_PER_PAGE);

  tft.setTextWrap(false);
  for (int i = start; i < end; i++) {
    int row = i - start;
    int y = LIST_TOP + row * LINE_H;
    if (i == selectedIndex) {
      tft.fillRect(2, y - 1, TFT_W - 4, LINE_H, UI_HIGHLIGHT);
      tft.setTextColor(UI_TEXT);
    } else {
      tft.setTextColor(UI_SUBTEXT);
    }
    tft.setCursor(LIST_LEFT, y);
    tft.print(formatTrackName(mp3Files[i]));
  }

  tft.setTextColor(UI_TEXT);
}



// --------------------------- READ MP3 LIST ------------------------
void listMp3Files(const char* dirname) {
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
  // Simple sort A-Z
  for (int i = 0; i < mp3Count - 1; i++) {
    for (int j = i + 1; j < mp3Count; j++) {
      if (mp3Files[j] < mp3Files[i]) {
        String tmp = mp3Files[i];
        mp3Files[i] = mp3Files[j];
        mp3Files[j] = tmp;
      }
    }
  }
}

String GetBatteryString() {
  String batt;
  if (batteryOk) {
    batt = max17048();
  }
  
  return batt;
}


String max17048() {
  // Get the percentage as float
  float percent = maxlipo.cellPercent();

  // Convert to integer (truncates decimal)
  int intPercent = (int)percent;

  // Convert to string
  String percentStr = String(intPercent);
  return percentStr;
}
