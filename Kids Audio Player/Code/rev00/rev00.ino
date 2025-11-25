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

// -------------------------------------
// SEESAW ROTARY ENCODER
// -------------------------------------
Adafruit_seesaw ss;
#define SEESAW_ADDR 0x36

int32_t lastEncoderPos = 0;
bool lastButtonState = false;
bool buttonState = false;

// -------------------------------------
// MP3 LIST
// -------------------------------------
#define MAX_FILES 50
String mp3Files[MAX_FILES];
int mp3Count = 0;
int selectedIndex = 0;

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
  // if no max17048..
  // if (!maxlipo.begin()) {
  //   Serial.println(F("Couldnt find Adafruit MAX17048, looking for LC709203F.."));

  // } else {
  //   addr0x36 = true;
  //   Serial.print(F("Found MAX17048"));
  //   Serial.print(F(" with Chip ID: 0x"));
  //   Serial.println(maxlipo.getChipID(), HEX);
  // }
  Serial.println("Player Ready");
}



// --------------------------- LOOP -------------------------------
void loop() {
  handleEncoder();
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

// Draw JPG from SD to full screen
void drawJpgFromSD(const char* filename) {
  Serial.print("Loading JPG: ");
  Serial.println(filename);

  if (!SD.exists(filename)) {
    Serial.println("File not found!");
    return;
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

  if (currentButton == LOW && lastButtonState == HIGH) {
    handleEncoderPress();
  }

  lastButtonState = currentButton;
}

void UpdateDisplayListAndPosition(int d) {
  selectedIndex += d;
  if (selectedIndex < 0) selectedIndex = 0;
  if (selectedIndex >= mp3Count) selectedIndex = mp3Count - 1;
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

    // Now you can use it with your draw function
    drawJpgFromSD(jpgPath.c_str());


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
  tft.fillRect(0, 118, 15, 128, ST77XX_BLACK);
  tft.setCursor(0, 118);
  tft.setTextColor(ST77XX_YELLOW);
  //tft.print("Volume: ");
  tft.print(int(volumeLevel * 100));
  tft.println("%");

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
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);

  tft.print("YANA STORIES:");
  //tft.setCursor(0, 100);
  //tft.println(GetBatteryString());
  tft.println("----------------");

  for (int i = 0; i < mp3Count; i++) {
    if (i == selectedIndex) {
      tft.setTextColor(ST77XX_YELLOW, ST77XX_BLUE);
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    }
    tft.println(mp3Files[i]);
  }

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
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
}

String GetBatteryString() {
  String batt;
  if (addr0x36 == true) {
    batt = max17048();
  }
  
  return batt;
}


String max17048() {
  Serial.print(F("Batt Voltage: "));
  Serial.print(maxlipo.cellVoltage(), 3);
  Serial.println(" V");
  Serial.print(F("Batt Percent: "));
  Serial.print(maxlipo.cellPercent(), 1);
  Serial.println(" %");
  Serial.println();
  // Get the percentage as float
  float percent = maxlipo.cellPercent();

  // Convert to integer (truncates decimal)
  int intPercent = (int)percent;

  // Convert to string
  String percentStr = String(intPercent);
  return percentStr;
}
