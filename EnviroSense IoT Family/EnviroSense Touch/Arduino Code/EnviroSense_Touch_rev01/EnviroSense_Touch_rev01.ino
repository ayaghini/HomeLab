//-----INCLUDES LIBRARIES---------------------------------------------------------
//--GENERAL LIBS
#include "secrets.h"
#include <Wire.h>
#include <SPI.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "PubSubClient.h"  // Connect and publish to the MQTT broker

//--WEMOS TFT 2.4 Touch----> https://www.wemos.cc/en/latest/d1_mini_shield/tft_2_4.html
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

//--WEMOS SGP30 CO2---->https://www.wemos.cc/en/latest/d1_mini_shield/sgp30.html
#include "Adafruit_SGP30.h"

//--WEMOS SHT30
#include <WEMOS_SHT3X.h>  //needs to be downloaded from: https://github.com/wemos/WEMOS_SHT3x_Arduino_Library


//--------------------------------------------------------------------------------
//-----DEFENITIONS-----
//--TFT 2.4
#define TFT_CS D0   //for D1 mini or TFT I2C Connector Shield (V1.1.0 or later)
#define TFT_DC D8   //for D1 mini or TFT I2C Connector Shield (V1.1.0 or later)
#define TFT_RST -1  //for D1 mini or TFT I2C Connector Shield (V1.1.0 or later)
#define TS_CS D3    //for D1 mini or TFT I2C Connector Shield (V1.1.0 or later)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TS_CS);

//--SGP30
Adafruit_SGP30 sgp;
bool isSgp;
int sgpCounter;
float tVoc = 0, eCo2 = 0, lasttVoc, lasteCo2;

//--SHT30
SHT3X sht30(0x45);
float temp = 0, humid = 0, lastTemp, lastHumid;
float tempOffset = 24.3 - 27.9;

//+++++++++SETUP PARAMETERS++++++++++
int state = 0;

//---------> TIME INETRVALS <---------------
long updateTimeInterval = 30000;           // interval at which to send (milliseconds)
long previousMillis = updateTimeInterval;  // will store last time data was sent
float displayThereshold = 20.0;

//---------> NTP Client <---------------
const long utcOffsetInSeconds = -25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
String timeToDisplay = "waiting to receive the time from server";

//---------> Weather Map API <---------
WiFiClient client;
const char server[] = "api.openweathermap.org";

int status = WL_IDLE_STATUS;
#define JSON_BUFF_DIMENSION 2500
unsigned long lastConnectionTime = 30 * 60 * 1000;  // last time you connected to the server, in milliseconds
const unsigned long postInterval = 30 * 60 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)
bool weatherUpdated = false;
String weatherForcast = "Updating..";
float weatherTemp = 0;
String weatherCondition = "UPDATING";

//---------> MQTT <---------
const char* clientID = "EnviroSenseTouch";

const char* temperature_topic = "EnviroSenseTouch/ENVT_temperature";
const char* humidity_topic = "EnviroSenseTouch/ENVT_humidity";
const char* tVoc_topic = "EnviroSenseTouch/ENVT_tVoc";
const char* eCO2_topic = "EnviroSenseTouch/ENVT_equivalentCO2";

PubSubClient mqttClient(client);
long intervalMQTT = 30 * 1000, lastTimeSentMQTT = intervalMQTT;

unsigned long touchOverrideTime = 0;     
const unsigned long touchOverrideDuration = 5000;
const unsigned long touchDebounceMs = 150;
unsigned long lastTouchMs = 0;
bool displayBlanked = false;
bool uiInitialized = false;

//---------> UI Styling <---------
uint16_t COLOR_BG;
uint16_t COLOR_PANEL;
uint16_t COLOR_TEXT;
uint16_t COLOR_SUBTEXT;
uint16_t COLOR_ACCENT_T;
uint16_t COLOR_ACCENT_H;
uint16_t COLOR_ACCENT_CO2;
uint16_t COLOR_ACCENT_TVOC;
uint16_t COLOR_STATUS_OK;
uint16_t COLOR_STATUS_BAD;
const int HEADER_H = 28;
const int TILE_GAP = 6;

struct UiTile {
  int x;
  int y;
  int w;
  int h;
  const char* label;
  uint16_t accent;
};

UiTile tileT, tileH, tileCO2, tileTVOC;

//---------> Function Prototypes <---------
void InitializeDisplay();
void InitializeWifi();
bool InitializeSgp();
void ArduinoOTAInitializer();
void StartApp();
void StateMachine();
void publishDiscovery();
void SendMQTT();
void reconnectMQTT();
void HandleTouch();
void UpdateTime();
void MeasureSgp();
void MeasureSht();
void UpdateWeather();
void UpdateDisplay();
void DrawWeatherIcon(int x, int y, int size, String condition);
void DisplayManager();
void InitUITheme();
void LayoutTiles();
void RenderUIStatic();
void RenderUIDynamic();
void DrawHeader();
void DrawStatusDot(int x, int y, bool ok);
void DrawTileFrame(const UiTile& tile);
void DrawTileValue(const UiTile& tile, const String& value, const char* unit);
void DrawSensorButton(int x, int y, int cx, int cy, String label, String value, uint16_t color);
uint16_t ColorMeter(int n, int low, int high, int extreme);
void DrawButton(int x, int y, int cx, int cy, String text, uint16_t color);
void PrintCentreString(const String& t, int x, int y);
void SendMQTTMessages(const char* topic, String parameter);
float Difrentiator(float a, float b);
void Progressor(String in);
String WeatherRequest();
//--------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  InitializeDisplay();
  InitializeWifi();
  isSgp = InitializeSgp();
  ArduinoOTAInitializer();
  mqttClient.setServer(mqtt_server, mqtt_port);
  publishDiscovery();
  delay(1000);
  StartApp();
  delay(1000);
}

void loop() {
  ArduinoOTA.handle();

  if (ts.touched()) HandleTouch();

  StateMachine();
}


//--------------------------------------------------------------------------------
void StateMachine() {
  unsigned long currentMillis = millis();
  switch (state) {
    case 0:
      {
        if (isSgp) MeasureSgp();
        state++;
        break;
      }
    case 1:
      {
        if (currentMillis - previousMillis > updateTimeInterval) {
          previousMillis = currentMillis;
          UpdateTime();
          DisplayManager();
        }
        state++;
        break;
      }
    case 2:
      {
        MeasureSht();
        state++;
        break;
      }
    case 3:
      {
        if (currentMillis - lastConnectionTime > postInterval) {
          lastConnectionTime = currentMillis;
          UpdateWeather();
        }
        state++;
        break;
      }
    case 4:
      {
        if (currentMillis - lastTimeSentMQTT > intervalMQTT) {
          lastTimeSentMQTT = currentMillis;
          if (!mqttClient.connected()) {
            reconnectMQTT();
          }
          mqttClient.loop();
          SendMQTT();
        }
        state++;
        break;
      }
    case 5:
      {
        UpdateDisplay();
        state = 0;
        break;
      }
  }
}

// -------------------- Home Assistant Discovery --------------------
void publishDiscovery() {
  mqttClient.publish(
    "homeassistant/sensor/envirosense_touch_temperature/config",
    R"({"
    "name":"ENVT_Temperature",
    "state_topic":"EnviroSenseTouch/ENVT_temperature",
    "unit_of_measurement":"C",
    "device_class":"temperature"
    })",
    true);

  mqttClient.publish(
    "homeassistant/sensor/envirosense_touch_humidity/config",
    R"({
      "name":"ENVT_Humidity",
      "state_topic":"EnviroSenseTouch/ENVT_humidity",
      "unit_of_measurement":"%",
      "device_class":"humidity"
      })",
    true);

  mqttClient.publish(
    "homeassistant/sensor/envirosense_touch_tvoc/config",
    R"({
      "name":"ENVT_tVOC",
      "state_topic":"EnviroSenseTouch/ENVT_tVoc",
      "unit_of_measurement":"ppb"
      })",
    true);

  mqttClient.publish(
    "homeassistant/sensor/envirosense_touch_eco2/config",
    R"({
      "name":"ENVT_eCO2",
      "state_topic":"EnviroSenseTouch/ENVT_equivalentCO2",
      "unit_of_measurement":"ppm"
      })",
    true);

  Serial.println("Home Assistant discovery published.");
}

// -------------------- Send Sensor Data --------------------
void SendMQTT() {
  if (!mqttClient.connected()) return;

  // mqttClient.publish(temperature_topic, String(temp, 1).c_str(), true);
  // mqttClient.publish(humidity_topic, String(humid, 1).c_str(), true);
  // mqttClient.publish(tVoc_topic, String(tVoc, 0).c_str(), true);
  // mqttClient.publish(eCO2_topic, String(eCo2, 0).c_str(), true);

  char payload[32];

  // Temperature
  snprintf(payload, sizeof(payload), "%.2f", temp);
  mqttClient.publish(temperature_topic, payload, true);

  // Humidity
  snprintf(payload, sizeof(payload), "%.2f", humid);
  mqttClient.publish(humidity_topic, payload, true);

  // tVOC
  snprintf(payload, sizeof(payload), "%.0f", tVoc);
  mqttClient.publish(tVoc_topic, payload, true);

  // eCO2
  snprintf(payload, sizeof(payload), "%.0f", eCo2);
  mqttClient.publish(eCO2_topic, payload, true);

  Serial.println("MQTT data sent:");
  Serial.printf("Temp: %.1f °C, Humidity: %.1f%%, tVOC: %.0f ppb, eCO2: %.0f ppm\n", temp, humid, tVoc, eCo2);
}

// -------------------- MQTT Reconnect --------------------
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(clientID, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Optionally re-publish discovery after reconnect
      publishDiscovery();
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void HandleTouch() {
  unsigned long now = millis();
  if (now - lastTouchMs < touchDebounceMs) {
    return;
  }
  lastTouchMs = now;
  touchOverrideTime = now;

  // Read the touch point to clear the controller state and keep display awake.
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    (void)p;
  }

  // Force a refresh when waking the screen.
  DisplayManager();

  // tft.fillScreen(ILI9341_BLACK);
  // tft.setCursor(0, 0);

  // tft.print("Pressure = ");
  // tft.println(p.z);
  // tft.print("X = ");
  // tft.println(p.x);
  // tft.print("Y = ");
  // tft.println(p.y);
}

void UpdateTime() {
  timeClient.update();
  timeToDisplay = timeClient.getFormattedTime();
}

void MeasureSgp() {
  if (!sgp.IAQmeasure()) {
    return;
  }

  lasttVoc = tVoc;
  lasteCo2 = eCo2;
  tVoc = sgp.TVOC;
  eCo2 = sgp.eCO2;
}

void MeasureSht() {
  if (sht30.get() == 0) {
    lastTemp = temp;
    lastHumid = humid;
    temp = sht30.cTemp + tempOffset;
    humid = sht30.humidity;

  } else {
  }
}

void UpdateWeather() {
  weatherUpdated = false;
  weatherForcast = WeatherRequest();
}

void UpdateDisplay() {
  float t = Difrentiator(temp, lastTemp);
  float h = Difrentiator(humid, lastHumid);
  float v = Difrentiator(tVoc, lasttVoc);
  float c = Difrentiator(eCo2, lasteCo2);

  if (t >= displayThereshold || h >= displayThereshold || c >= displayThereshold) {
    DisplayManager();
  }
}

// Draw weather icon
void DrawWeatherIcon(int x, int y, int size, String condition) {
  // simple icons using filled shapes
  if (condition == "CLEAR") {
    tft.fillCircle(x, y, size / 2, ILI9341_YELLOW);  // sun
  } else if (condition == "CLOUDY") {
    tft.fillRoundRect(x - size / 2, y - size / 4, size, size / 2, 5, ILI9341_LIGHTGREY);  // cloud
  } else if (condition == "RAIN") {
    tft.fillRoundRect(x - size / 2, y - size / 4, size, size / 2, 5, ILI9341_LIGHTGREY);
    tft.drawLine(x - size / 4, y + size / 4, x - size / 4, y + size / 2, ILI9341_BLUE);
    tft.drawLine(x, y + size / 4, x, y + size / 2, ILI9341_BLUE);
    tft.drawLine(x + size / 4, y + size / 4, x + size / 4, y + size / 2, ILI9341_BLUE);
  } else if (condition == "SNOW") {
    tft.fillRoundRect(x - size / 2, y - size / 4, size, size / 2, 5, ILI9341_WHITE);
    tft.drawLine(x - 5, y, x + 5, y, ILI9341_CYAN);
    tft.drawLine(x, y - 5, x, y + 5, ILI9341_CYAN);
  } else if (condition == "THUNDERSTORM") {
    tft.fillRoundRect(x - size / 2, y - size / 4, size, size / 2, 5, ILI9341_LIGHTGREY);
    tft.fillTriangle(x, y, x - 5, y + 15, x + 5, y + 15, ILI9341_YELLOW);
  } else {
    // default cloud
    tft.fillRoundRect(x - size / 2, y - size / 4, size, size / 2, 5, ILI9341_LIGHTGREY);
  }
}

void DisplayManager() {
  int hourNow = timeClient.getHours();
  // Check if we are within the 5-second override window
  bool forceOn = (millis() - touchOverrideTime) < touchOverrideDuration;

  // If between 7 PM and 6 AM, blank the screen, unless forced
  if (!forceOn && (hourNow >= 19 || hourNow < 6)) {
    if (!displayBlanked) {
      tft.fillScreen(ILI9341_BLACK);
      displayBlanked = true;
      uiInitialized = false;
    }
    return; // skip drawing
  }

  displayBlanked = false;
  if (!uiInitialized) {
    RenderUIStatic();
    uiInitialized = true;
  }
  RenderUIDynamic();
}

void InitUITheme() {
  COLOR_BG = tft.color565(12, 16, 28);
  COLOR_PANEL = tft.color565(20, 26, 40);
  COLOR_TEXT = ILI9341_WHITE;
  COLOR_SUBTEXT = tft.color565(170, 180, 200);
  COLOR_ACCENT_T = tft.color565(255, 145, 56);
  COLOR_ACCENT_H = tft.color565(80, 200, 210);
  COLOR_ACCENT_CO2 = tft.color565(255, 90, 90);
  COLOR_ACCENT_TVOC = tft.color565(170, 120, 255);
  COLOR_STATUS_OK = tft.color565(60, 200, 120);
  COLOR_STATUS_BAD = tft.color565(230, 80, 80);
}

void LayoutTiles() {
  int width = tft.width();
  int height = tft.height();
  int tileW = (width - (TILE_GAP * 3)) / 2;
  int tileHgt = (height - HEADER_H - (TILE_GAP * 3)) / 2;

  tileT = {TILE_GAP, HEADER_H + TILE_GAP, tileW, tileHgt, "TEMP", COLOR_ACCENT_T};
  tileH = {TILE_GAP * 2 + tileW, HEADER_H + TILE_GAP, tileW, tileHgt, "HUM", COLOR_ACCENT_H};
  tileCO2 = {TILE_GAP, HEADER_H + TILE_GAP * 2 + tileHgt, tileW, tileHgt, "eCO2", COLOR_ACCENT_CO2};
  tileTVOC = {TILE_GAP * 2 + tileW, HEADER_H + TILE_GAP * 2 + tileHgt, tileW, tileHgt, "TVOC", COLOR_ACCENT_TVOC};
}

void RenderUIStatic() {
  tft.fillScreen(COLOR_BG);
  tft.fillRect(0, 0, tft.width(), HEADER_H, COLOR_PANEL);
  tft.drawFastHLine(0, HEADER_H - 1, tft.width(), tft.color565(40, 50, 70));

  DrawTileFrame(tileT);
  DrawTileFrame(tileH);
  DrawTileFrame(tileCO2);
  DrawTileFrame(tileTVOC);
}

void RenderUIDynamic() {
  DrawHeader();
  DrawTileValue(tileT, String(temp, 1), "C");
  DrawTileValue(tileH, String(humid, 1), "%");
  DrawTileValue(tileCO2, String(eCo2, 0), "ppm");
  DrawTileValue(tileTVOC, String(tVoc, 0), "ppb");
}

void DrawHeader() {
  int width = tft.width();
  int y = 4;

  // Clear header content area
  tft.fillRect(0, 1, width, HEADER_H - 2, COLOR_PANEL);

  // Time
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(6, y);
  tft.print(timeToDisplay);

  // Weather summary
  String weatherText = weatherCondition + " " + String(weatherTemp, 1) + "C";
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(1);
  tft.getTextBounds(weatherText, 0, 0, &x1, &y1, &w, &h);
  int wx = width - w - 24;
  if (wx < 120) {
    weatherText = weatherCondition;
    tft.getTextBounds(weatherText, 0, 0, &x1, &y1, &w, &h);
    wx = width - w - 24;
  }
  tft.setCursor(wx, y + 6);
  tft.setTextColor(COLOR_SUBTEXT);
  tft.print(weatherText);

  // Status dots
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool mqttOk = mqttClient.connected();
  DrawStatusDot(width - 16, y + 8, wifiOk);
  DrawStatusDot(width - 8, y + 8, mqttOk);
}

void DrawStatusDot(int x, int y, bool ok) {
  uint16_t c = ok ? COLOR_STATUS_OK : COLOR_STATUS_BAD;
  tft.fillCircle(x, y, 3, c);
}

void DrawTileFrame(const UiTile& tile) {
  int r = 8;
  tft.fillRoundRect(tile.x + 2, tile.y + 2, tile.w, tile.h, r, tft.color565(10, 12, 20));
  tft.fillRoundRect(tile.x, tile.y, tile.w, tile.h, r, COLOR_PANEL);
  tft.drawRoundRect(tile.x, tile.y, tile.w, tile.h, r, tile.accent);

  tft.setTextColor(COLOR_SUBTEXT);
  tft.setTextSize(1);
  tft.setCursor(tile.x + 8, tile.y + 6);
  tft.print(tile.label);
}

void DrawTileValue(const UiTile& tile, const String& value, const char* unit) {
  int valueAreaX = tile.x + 6;
  int valueAreaY = tile.y + 18;
  int valueAreaW = tile.w - 12;
  int valueAreaH = tile.h - 24;

  tft.fillRect(valueAreaX, valueAreaY, valueAreaW, valueAreaH, COLOR_PANEL);

  // Value
  tft.setTextColor(tile.accent);
  tft.setTextSize(3);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(value, 0, 0, &x1, &y1, &w, &h);
  int vx = tile.x + (tile.w - w) / 2;
  int vy = tile.y + (tile.h / 2) - 10;
  tft.setCursor(vx, vy);
  tft.print(value);

  // Unit
  tft.setTextColor(COLOR_SUBTEXT);
  tft.setTextSize(1);
  tft.getTextBounds(unit, 0, 0, &x1, &y1, &w, &h);
  int ux = tile.x + (tile.w - w) / 2;
  int uy = vy + 28;
  tft.setCursor(ux, uy);
  tft.print(unit);
}

void DrawSensorButton(int x, int y, int cx, int cy, String label, String value, uint16_t color) {
  int r = 8;  // rounded corners
  int t = 4;  // inner padding

  // Shadow
  tft.fillRoundRect(x + 3, y + 3, cx, cy, r, ILI9341_DARKGREY);

  // Button background
  tft.fillRoundRect(x, y, cx, cy, r, color);

  // Inner rectangle
  tft.fillRoundRect(x + t, y + t, cx - 2 * t, cy - 2 * t, r, ILI9341_BLACK);

  // Draw value centered
  tft.setTextColor(color);
  tft.setTextSize(3);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(value, x + cx / 2, y + cy / 2, &x1, &y1, &w, &h);
  tft.setCursor(x + cx / 2 - w / 2, y + cy / 2 - h / 2 - 10);
  tft.print(value);

  // Draw label centered below value
  tft.setTextSize(2);
  tft.getTextBounds(label, x + cx / 2, y + cy / 2, &x1, &y1, &w, &h);
  tft.setCursor(x + cx / 2 - w / 2, y + cy / 2 - h / 2 + 20);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(label);
}


// void DisplayManager() {
//   int cx = tft.width() / 3 - 1;
//   int cy = tft.height() / 4 - 1;

//   tft.fillScreen(ILI9341_BLACK);
//   tft.setCursor(0, 0);

//   DrawButton(cx / 3, cy / 2, cx, cy, String(temp, 1), ColorMeter(temp, 20, 25, 27));
//   DrawButton(5 * cx / 3, cy / 2, cx, cy, String(humid, 1), ColorMeter(humid, 40, 70, 80));
//   DrawButton(cx / 3, 5 * cy / 2, cx, cy, String(eCo2, 0), ColorMeter(eCo2, 400, 800, 1000));
//   DrawButton(5 * cx / 3, 5 * cy / 2, cx, cy, String(tVoc, 0), ColorMeter(tVoc, 50, 500, 1000));
//   tft.setTextColor(ILI9341_WHITE);
//   PrintCentreString(timeToDisplay, tft.width() / 2, cy / 4);
//   tft.setCursor(0, 0);
//   PrintCentreString(weatherForcast, tft.width() / 2, tft.height() - cy / 10);
// }

uint16_t ColorMeter(int n, int low, int high, int extreme) {
  if (n < low) {
    return ILI9341_BLUE;
  } else if (n < high) {
    return ILI9341_GREEN;
  } else if (n < extreme) {
    return ILI9341_ORANGE;
  } else {
    return ILI9341_RED;
  }
}

void DrawButton(int x, int y, int cx, int cy, String text, uint16_t color) {
  int r = 4;
  int t = 3;
  uint16_t bc = ILI9341_BLACK;
  int textSize = 3;

  tft.fillRoundRect(x, y, cx, cy, r, color);
  tft.fillRoundRect(x + t, y + t, cx - t - t, cy - t - t, r, bc);
  tft.setTextColor(color);
  tft.setTextSize(textSize);
  PrintCentreString(text, x + cx / 2, y + cy / 2);
}

void PrintCentreString(const String& t, int x, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(t, x, y, &x1, &y1, &w, &h);  //calc width of new string
  tft.setCursor(x - w / 2, y - h / 2);
  tft.print(t);
}

// void SendMQTT() {
//   if (mqttClient.connect(clientID)) {
//     SendMQTTMessages(temperature_topic, String(temp));
//     SendMQTTMessages(humidity_topic, String(humid));
//     SendMQTTMessages(tVoc_topic, String(tVoc));
//     SendMQTTMessages(eCO2_topic, String(eCo2));

//     mqttClient.disconnect();  // disconnect from the MQTT broker
//   } else {
//     //Serial.println("Connection to MQTT Broker failed...");
//   }
// }

void SendMQTTMessages(const char* topic, String parameter) {
  if (!mqttClient.publish(topic, parameter.c_str())) {
    mqttClient.connect(clientID);
    delay(10);  // This delay ensures that client.publish doesn't clash with the client.connect call
    mqttClient.publish(topic, parameter.c_str());
  }
}

float Difrentiator(float a, float b) {
  if (a == 0) {
    return 0;
  }
  float r = ((a - b) / a) * 100;
  return r;
}

void InitializeDisplay() {
  ts.begin();
  ts.setRotation(1);

  tft.begin();
  tft.setRotation(3);
  InitUITheme();
  LayoutTiles();
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.println("EnviroSense Touch");
  tft.println("A. Yaghini");
}

void InitializeWifi() {
  tft.println("");
  Progressor("WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  const unsigned long startMs = millis();
  const unsigned long timeoutMs = 20000;

  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    tft.println(" ok");
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    tft.setTextColor(ILI9341_RED);
    tft.println(" fail");
    tft.setTextColor(ILI9341_WHITE);
    Serial.println("\nWiFi connect timeout.");
    Serial.print("Status: ");
    Serial.println(WiFi.status());
    // Continue running in offline mode; MQTT/Weather will retry later.
  }
}

bool InitializeSgp() {
  Progressor("SGP30");

  if (!sgp.begin()) {
    tft.setTextColor(ILI9341_RED);
    tft.println("  X");
    tft.setTextColor(ILI9341_WHITE);
    return false;
  } else {
    tft.setTextColor(ILI9341_GREEN);
    tft.println("  ok");
    tft.setTextColor(ILI9341_WHITE);
    return true;
  }
}

void Progressor(String in) {
  tft.print(in);
  delay(200);
  tft.setTextColor(ILI9341_RED);
  tft.print(".");
  delay(200);
  tft.print("..");
  delay(200);
  tft.print("...");
  tft.setTextColor(ILI9341_WHITE);
}

void StartApp() {
  Progressor("Time");
  timeClient.begin();
  tft.setTextColor(ILI9341_GREEN);
  tft.println("  ok");
  tft.setTextColor(ILI9341_WHITE);
  delay(1000);
  Progressor("Weather");
  tft.setTextColor(ILI9341_GREEN);
  tft.println("  ok");
  tft.setTextColor(ILI9341_WHITE);
  delay(3000);
  tft.fillScreen(ILI9341_GREEN);
}

void ArduinoOTAInitializer() {
  ArduinoOTA.setHostname("Enviro_Touch");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

// to request data from OWM
String WeatherRequest() {
  client.stop();
  weatherUpdated = false;

  if (!client.connect(server, 80)) {
    Serial.println("Failed to connect to server");
    return "NAS";
  }

  // Send HTTP request
  client.print("GET /data/2.5/forecast?q=");
  client.print(nameOfCity);
  client.print("&APPID=");
  client.print(apiKey);
  client.println("&mode=json&units=metric&cnt=2 HTTP/1.1");
  client.println("Host: api.openweathermap.org");
  client.println("User-Agent: ArduinoWiFi/1.1");
  client.println("Connection: close");
  client.println();

  // Wait for response
  unsigned long timeout = millis();
  while (!client.available()) {
    if (millis() - timeout > 5000) {
      client.stop();
      Serial.println("Client timeout");
      return "CONNECTION!";
    }
  }

  // Skip headers
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) break;
  }

  // Read JSON
  String jsonBody = client.readString();
  StaticJsonDocument<JSON_BUFF_DIMENSION> doc;
  DeserializationError error = deserializeJson(doc, jsonBody);
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    client.stop();
    return "JSON ERR";
  }

  client.stop();

  // Extract values
  int id = doc["list"][0]["weather"][0]["id"];
  weatherTemp = doc["list"][0]["main"]["feels_like"];  // store temperature

  if (id >= 200 && id < 300) weatherCondition = "THUNDERSTORM";
  else if (id >= 300 && id < 400) weatherCondition = "DRIZZLE";
  else if (id >= 500 && id < 600) weatherCondition = "RAIN";
  else if (id >= 600 && id < 700) weatherCondition = "SNOW";
  else if (id >= 700 && id < 800) weatherCondition = "ATMOSPHERE";
  else if (id == 800) weatherCondition = "CLEAR";
  else if (id > 800) weatherCondition = "CLOUDY";
  else weatherCondition = "UPDATING";

  weatherUpdated = (id > 100);

  // Return simple status for debug
  return weatherCondition;
}
