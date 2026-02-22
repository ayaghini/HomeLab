#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_BME680.h>

// -------------------- WiFi + MQTT --------------------
#include "secrets.h"
// const char* ssid = "";
// const char* password = "";

// const char* mqtt_server = "";
// const int mqtt_port = 1883;
// const char* mqtt_user = "";
// const char* mqtt_pass = "";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
ESP8266WebServer webServer(80);
DNSServer dnsServer;

// -------------------- Device Identity --------------------
const char* DEVICE_NAME = "EnviroSensePico";
const char* DEVICE_MODEL = "EnviroSense Pico";
const char* DEVICE_MFR = "ayaghini";
const char* SW_VERSION = "rev01";

const char* TOPIC_BASE = "EnviroSense";
const char* TOPIC_AVAIL = "EnviroSense/status";

// -------------------- Display --------------------
//TFT LCD
#define TFT_CS         D3
#define TFT_RST        RX
#define TFT_DC         D4
#define TFT_MOSI D7  // Data out
#define TFT_SCLK D5  // Clock out
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// UI colors
uint16_t UI_BG;
uint16_t UI_PANEL;
uint16_t UI_TEXT;
uint16_t UI_SUBTEXT;
uint16_t UI_ACCENT_T;
uint16_t UI_ACCENT_H;
uint16_t UI_ACCENT_P;
uint16_t UI_ACCENT_G;
uint16_t UI_ACCENT_A;

// Last displayed values
float lastTemperature = NAN;
float lastHumidity = NAN;
float lastGas = NAN;
String lastAqiText = "";
float lastPressure = NAN;
bool lastWifi = false;
bool lastMqtt = false;

// Thresholds
const float tempThreshold = 1.0;      // degrees Celsius
const float humidityThreshold = 5.0;  // percent
const float gasThreshold = 100000.0;      // ohms (or adjust based on scale)
const float pressureThreshold = 1.0;      // hPa

// -------------------- BME680 --------------------
Adafruit_BME680 bme; // I2C

// Global sensor values
float temperature = NAN;
float humidity = NAN;
float pressure = NAN;
float gas_resistance = NAN;

float tempOffset = -3.06;

const unsigned long sensorInterval = 2000; // read every 2 seconds
unsigned long lastSensorRead = 0;

// -------------------- Timers --------------------
unsigned long lastDisplayUpdate = 0;
unsigned long lastMqttUpdate = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastWiFiAttempt = 0;
const unsigned long displayInterval = 2000;
const unsigned long mqttInterval = 60000;
const unsigned long mqttReconnectInterval = 5000;
const unsigned long wifiReconnectInterval = 15000;
const unsigned long wifiConnectTimeout = 30000;
const char* CONFIG_FILE = "/config.txt";

// -------------------- Config --------------------
String cfgSsid;
String cfgPassword;
String cfgMqttServer;
String cfgMqttUser;
String cfgMqttPass;
int cfgMqttPort = 1883;
bool portalActive = false;
unsigned long wifiConnectStart = 0;
bool fsMounted = false;

String chipIdString() {
  return String(ESP.getChipId(), HEX);
}

void applyFallbackFromSecrets() {
  if (cfgSsid.length() == 0) cfgSsid = String(ssid);
  if (cfgPassword.length() == 0) cfgPassword = String(password);
  if (cfgMqttServer.length() == 0) cfgMqttServer = String(mqtt_server);
  if (cfgMqttUser.length() == 0) cfgMqttUser = String(mqtt_user);
  if (cfgMqttPass.length() == 0) cfgMqttPass = String(mqtt_pass);
  if (cfgMqttPort <= 0) cfgMqttPort = mqtt_port;
}

void parseConfigLine(const String& line) {
  int eq = line.indexOf('=');
  if (eq <= 0) return;
  String key = line.substring(0, eq);
  String val = line.substring(eq + 1);
  key.trim();
  val.trim();

  if (key == "ssid") cfgSsid = val;
  else if (key == "password") cfgPassword = val;
  else if (key == "mqtt_server") cfgMqttServer = val;
  else if (key == "mqtt_port") cfgMqttPort = val.toInt();
  else if (key == "mqtt_user") cfgMqttUser = val;
  else if (key == "mqtt_pass") cfgMqttPass = val;
}

bool loadConfigFromFS() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return false;
  }
  fsMounted = true;
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("No config file in LittleFS");
    return false;
  }

  File f = LittleFS.open(CONFIG_FILE, "r");
  if (!f) {
    Serial.println("Failed to open config file");
    return false;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (line.startsWith("#")) continue;
    parseConfigLine(line);
  }
  f.close();
  return true;
}

bool saveConfigToFS() {
  if (!fsMounted && !LittleFS.begin()) {
    Serial.println("LittleFS mount failed (save)");
    return false;
  }
  fsMounted = true;
  File f = LittleFS.open(CONFIG_FILE, "w");
  if (!f) return false;
  f.printf("ssid=%s\n", cfgSsid.c_str());
  f.printf("password=%s\n", cfgPassword.c_str());
  f.printf("mqtt_server=%s\n", cfgMqttServer.c_str());
  f.printf("mqtt_port=%d\n", cfgMqttPort);
  f.printf("mqtt_user=%s\n", cfgMqttUser.c_str());
  f.printf("mqtt_pass=%s\n", cfgMqttPass.c_str());
  f.close();
  return true;
}

String buildConfigPage() {
  String page;
  page += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>EnviroSense Pico Setup</title></head><body style='font-family:Arial,Helvetica,sans-serif;'>");
  page += F("<h2>EnviroSense Pico WiFi Setup</h2>");
  page += F("<p>Enter WiFi and MQTT settings, then Save.</p>");
  page += F("<form method='POST' action='/save'>");
  page += F("WiFi SSID<br><input name='ssid' required value='");
  page += cfgSsid;
  page += F("'><br><br>");
  page += F("WiFi Password<br><input name='password' type='password' value='");
  page += cfgPassword;
  page += F("'><br><br>");
  page += F("MQTT Server<br><input name='mqtt_server' required value='");
  page += cfgMqttServer;
  page += F("'><br><br>");
  page += F("MQTT Port<br><input name='mqtt_port' type='number' value='");
  page += String(cfgMqttPort);
  page += F("'><br><br>");
  page += F("MQTT User<br><input name='mqtt_user' value='");
  page += cfgMqttUser;
  page += F("'><br><br>");
  page += F("MQTT Pass<br><input name='mqtt_pass' type='password' value='");
  page += cfgMqttPass;
  page += F("'><br><br>");
  page += F("<button type='submit'>Save</button></form>");
  page += F("<p>Device will reboot after saving.</p>");
  page += F("</body></html>");
  return page;
}

void showConfigPortalScreen(const String& apSsid, const IPAddress& ip) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.print("AP MODE");
  tft.setCursor(0, 12);
  tft.print("SSID:");
  tft.setCursor(0, 22);
  tft.print(apSsid);
  tft.setCursor(0, 36);
  tft.print("Open:");
  tft.setCursor(0, 46);
  tft.print(ip.toString());
}

void handleSave() {
  cfgSsid = webServer.arg("ssid");
  cfgPassword = webServer.arg("password");
  cfgMqttServer = webServer.arg("mqtt_server");
  String portStr = webServer.arg("mqtt_port");
  cfgMqttUser = webServer.arg("mqtt_user");
  cfgMqttPass = webServer.arg("mqtt_pass");

  if (portStr.length() > 0) {
    int p = portStr.toInt();
    if (p > 0) cfgMqttPort = p;
  }

  if (cfgSsid.length() == 0 || cfgMqttServer.length() == 0) {
    webServer.send(400, "text/plain", "SSID and MQTT server are required.");
    return;
  }

  if (!saveConfigToFS()) {
    webServer.send(500, "text/plain", "Failed to save config.");
    return;
  }

  webServer.send(200, "text/html", "<html><body><h3>Saved. Rebooting...</h3></body></html>");
  delay(1500);
  ESP.restart();
}

void startConfigPortal(const char* reason) {
  if (portalActive) return;
  portalActive = true;

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  String apSsid = String(DEVICE_NAME) + "-" + chipIdString();
  WiFi.softAP(apSsid.c_str());
  IPAddress ip = WiFi.softAPIP();

  dnsServer.start(53, "*", ip);

  Serial.println();
  Serial.println("=== WiFi Config Portal ===");
  Serial.print("Reason: ");
  Serial.println(reason);
  Serial.print("Connect to AP: ");
  Serial.println(apSsid);
  Serial.print("Open: http://");
  Serial.println(ip.toString());

  showConfigPortalScreen(apSsid, ip);

  webServer.on("/", HTTP_GET, []() {
    webServer.send(200, "text/html", buildConfigPage());
  });
  webServer.on("/save", HTTP_POST, []() {
    handleSave();
  });
  webServer.onNotFound([]() {
    webServer.sendHeader("Location", "/");
    webServer.send(302, "text/plain", "");
  });

  webServer.begin();
}

// -------------------- Display Layout --------------------
const int SCREEN_W = 160;
const int SCREEN_H = 80;
const int HEADER_H = 14;
const int CELL_W = 80;
const int CELL_H = 22;

struct Cell {
  int x;
  int y;
  const char* label;
};

const Cell CELL_TEMP = {0, HEADER_H, "TEMP"};
const Cell CELL_HUM = {CELL_W, HEADER_H, "HUM"};
const Cell CELL_PRES = {0, HEADER_H + CELL_H, "PRES"};
const Cell CELL_GAS = {CELL_W, HEADER_H + CELL_H, "GAS"};
const Cell CELL_AQI = {0, HEADER_H + CELL_H * 2, "AQI"};
const Cell CELL_STAT = {CELL_W, HEADER_H + CELL_H * 2, "STAT"};

// -------------------- MQTT Discovery --------------------
void publishDiscovery() {
  String unique = String(DEVICE_NAME) + "_" + String(ESP.getChipId(), HEX);
  String dev = String(R"({"ids":[")") + unique + R"("],"name":")" + DEVICE_MODEL +
               R"(","mdl":")" + DEVICE_MODEL + R"(","mf":")" + DEVICE_MFR +
               R"(","sw":")" + SW_VERSION + R"("})";

  // Temperature
  String tempCfg = String(R"({")")
    + R"("name":"EnviroSense Temperature",)"
    + R"("unique_id":")" + unique + R"(_temperature",)"
    + R"("state_topic":"EnviroSense/temperature",)"
    + R"("unit_of_measurement":"°C",)"
    + R"("device_class":"temperature",)"
    + R"("state_class":"measurement",)"
    + R"("availability_topic":"EnviroSense/status",)"
    + R"("payload_available":"online",)"
    + R"("payload_not_available":"offline",)"
    + R"("device":)" + dev
    + R"(})";
  mqttClient.publish("homeassistant/sensor/enviroSense_temperature/config", tempCfg.c_str(), true);

  // Humidity
  String humCfg = String(R"({")")
    + R"("name":"EnviroSense Humidity",)"
    + R"("unique_id":")" + unique + R"(_humidity",)"
    + R"("state_topic":"EnviroSense/humidity",)"
    + R"("unit_of_measurement":"%",)"
    + R"("device_class":"humidity",)"
    + R"("state_class":"measurement",)"
    + R"("availability_topic":"EnviroSense/status",)"
    + R"("payload_available":"online",)"
    + R"("payload_not_available":"offline",)"
    + R"("device":)" + dev
    + R"(})";
  mqttClient.publish("homeassistant/sensor/enviroSense_humidity/config", humCfg.c_str(), true);

  // Pressure
  String presCfg = String(R"({")")
    + R"("name":"EnviroSense Pressure",)"
    + R"("unique_id":")" + unique + R"(_pressure",)"
    + R"("state_topic":"EnviroSense/pressure",)"
    + R"("unit_of_measurement":"hPa",)"
    + R"("device_class":"pressure",)"
    + R"("state_class":"measurement",)"
    + R"("availability_topic":"EnviroSense/status",)"
    + R"("payload_available":"online",)"
    + R"("payload_not_available":"offline",)"
    + R"("device":)" + dev
    + R"(})";
  mqttClient.publish("homeassistant/sensor/enviroSense_pressure/config", presCfg.c_str(), true);

  // Gas
  String gasCfg = String(R"({")")
    + R"("name":"EnviroSense Gas",)"
    + R"("unique_id":")" + unique + R"(_gas",)"
    + R"("state_topic":"EnviroSense/gas",)"
    + R"("unit_of_measurement":"Ohm",)"
    + R"("availability_topic":"EnviroSense/status",)"
    + R"("payload_available":"online",)"
    + R"("payload_not_available":"offline",)"
    + R"("device":)" + dev
    + R"(})";
  mqttClient.publish("homeassistant/sensor/enviroSense_gas/config", gasCfg.c_str(), true);

  // AQI
  String aqiCfg = String(R"({")")
    + R"("name":"EnviroSense AQI",)"
    + R"("unique_id":")" + unique + R"(_aqi",)"
    + R"("state_topic":"EnviroSense/aqi",)"
    + R"("availability_topic":"EnviroSense/status",)"
    + R"("payload_available":"online",)"
    + R"("payload_not_available":"offline",)"
    + R"("device":)" + dev
    + R"(})";
  mqttClient.publish("homeassistant/sensor/enviroSense_aqi/config", aqiCfg.c_str(), true);
}




// -------------------- MQTT Publish --------------------
void publishState() {
  char payload[32];

  // Temperature
  snprintf(payload, sizeof(payload), "%.2f", temperature);
  mqttClient.publish("EnviroSense/temperature", payload, true);

  // Humidity
  snprintf(payload, sizeof(payload), "%.2f", humidity);
  mqttClient.publish("EnviroSense/humidity", payload, true);

  // Pressure
  snprintf(payload, sizeof(payload), "%.0f", pressure);
  mqttClient.publish("EnviroSense/pressure", payload, true);

  // Gas
  snprintf(payload, sizeof(payload), "%.0f", gas_resistance);
  mqttClient.publish("EnviroSense/gas", payload, true);

  // AQI
  String aqiText;
  if (gas_resistance > 30000) aqiText = "Good";
  else if (gas_resistance > 15000) aqiText = "Moderate";
  else aqiText = "Poor";

  mqttClient.publish("EnviroSense/aqi", aqiText.c_str(), true);
}

// -------------------- Debug Print Function --------------------
void printSensorData() {
  Serial.println(F("---- BME680 Sensor Readings ----"));
  Serial.print(F("Temperature: "));
  Serial.print(temperature, 2);
  Serial.println(F(" °C"));

  Serial.print(F("Humidity: "));
  Serial.print(humidity, 2);
  Serial.println(F(" %"));

  Serial.print(F("Pressure: "));
  Serial.print(pressure, 2);
  Serial.println(F(" hPa"));

  Serial.print(F("Gas Resistance: "));
  Serial.print(gas_resistance, 0);
  Serial.println(F(" Ohms"));

  Serial.println(F("-------------------------------"));
}

// -------------------- BME680 Reading --------------------
void readBME680() {
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    yield();
    return;
  }

  temperature = bme.temperature + tempOffset;
  humidity = bme.humidity;
  pressure = bme.pressure / 100.0f; // hPa
  gas_resistance = bme.gas_resistance;

  // Print to Serial for debugging
  printSensorData();
}

// -------------------- Display --------------------
void initUiColors() {
  UI_BG = tft.color565(10, 14, 24);
  UI_PANEL = tft.color565(18, 24, 40);
  UI_TEXT = ST77XX_WHITE;
  UI_SUBTEXT = tft.color565(150, 160, 180);
  UI_ACCENT_T = tft.color565(255, 120, 60);
  UI_ACCENT_H = tft.color565(80, 200, 210);
  UI_ACCENT_P = tft.color565(120, 180, 255);
  UI_ACCENT_G = tft.color565(120, 140, 255);
  UI_ACCENT_A = tft.color565(140, 255, 120);
}

void drawHeader(bool wifiOk, bool mqttOk) {
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, UI_PANEL);
  tft.setCursor(4, 3);
  tft.setTextColor(UI_TEXT);
  tft.setTextSize(1);
  tft.print("EnviroSense Pico");

  // Status dots
  tft.fillCircle(142, 6, 3, wifiOk ? ST77XX_GREEN : ST77XX_RED);
  tft.fillCircle(152, 6, 3, mqttOk ? ST77XX_GREEN : ST77XX_RED);
}

void drawGrid() {
  tft.drawFastHLine(0, HEADER_H, SCREEN_W, tft.color565(40, 50, 70));
  tft.drawFastHLine(0, HEADER_H + CELL_H, SCREEN_W, tft.color565(40, 50, 70));
  tft.drawFastHLine(0, HEADER_H + CELL_H * 2, SCREEN_W, tft.color565(40, 50, 70));
  tft.drawFastVLine(CELL_W, HEADER_H, SCREEN_H - HEADER_H, tft.color565(40, 50, 70));
}

void drawCellLabel(const Cell& cell) {
  tft.setCursor(cell.x + 4, cell.y + 3);
  tft.setTextColor(UI_SUBTEXT);
  tft.setTextSize(1);
  tft.print(cell.label);
}

void drawCellValue(const Cell& cell, const String& value, uint16_t color) {
  tft.fillRect(cell.x + 2, cell.y + 12, CELL_W - 4, CELL_H - 12, UI_BG);
  tft.setCursor(cell.x + 4, cell.y + 13);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.print(value);
}

void setupDisplay() {
  tft.initR(INITR_MINI160x80);   // Initialize ST7735
  tft.setRotation(1);
  tft.invertDisplay(true);
  initUiColors();
  tft.fillScreen(UI_BG);
  drawHeader(false, false);
  drawGrid();
  drawCellLabel(CELL_TEMP);
  drawCellLabel(CELL_HUM);
  drawCellLabel(CELL_PRES);
  drawCellLabel(CELL_GAS);
  drawCellLabel(CELL_AQI);
  drawCellLabel(CELL_STAT);
}

void updateDisplay() {
  // Determine AQI text
  String aqiText;
  if (gas_resistance > 30000) aqiText = "Good";
  else if (gas_resistance > 15000) aqiText = "Moderate";
  else aqiText = "Poor";

  // Check if any value changed significantly
  bool updateRequired = false;

  if (isnan(lastTemperature) || abs(temperature - lastTemperature) >= tempThreshold) updateRequired = true;
  if (isnan(lastHumidity) || abs(humidity - lastHumidity) >= humidityThreshold) updateRequired = true;
  if (isnan(lastGas) || abs(gas_resistance - lastGas) >= gasThreshold) updateRequired = true;
  if (isnan(lastPressure) || abs(pressure - lastPressure) >= pressureThreshold) updateRequired = true;
  if (lastAqiText != aqiText) updateRequired = true;

  if (!updateRequired && (millis() - lastDisplayUpdate) < displayInterval) return;
  lastDisplayUpdate = millis();

  // Save current values as last displayed
  lastTemperature = temperature;
  lastHumidity = humidity;
  lastGas = gas_resistance;
  lastAqiText = aqiText;
  lastPressure = pressure;

  bool wifiOk = WiFi.status() == WL_CONNECTED;
  bool mqttOk = mqttClient.connected();
  if (wifiOk != lastWifi || mqttOk != lastMqtt) {
    drawHeader(wifiOk, mqttOk);
    lastWifi = wifiOk;
    lastMqtt = mqttOk;
  }

  drawCellValue(CELL_TEMP, String(temperature, 1) + " C", UI_ACCENT_T);
  drawCellValue(CELL_HUM, String(humidity, 0) + " %", UI_ACCENT_H);
  drawCellValue(CELL_PRES, String(pressure, 0) + " hPa", UI_ACCENT_P);
  drawCellValue(CELL_GAS, String((int)gas_resistance) + " ohm", UI_ACCENT_G);
  drawCellValue(CELL_AQI, aqiText, UI_ACCENT_A);
  drawCellValue(CELL_STAT, mqttClient.connected() ? "MQTT OK" : "MQTT OFF", UI_TEXT);
}






// -------------------- WiFi + MQTT --------------------
void reconnectMqtt() {
  if (mqttClient.connected()) return;
  if (millis() - lastMqttAttempt < mqttReconnectInterval) return;
  lastMqttAttempt = millis();

  Serial.print("Connecting to MQTT...");
  String clientId = String(DEVICE_NAME) + "_" + String(ESP.getChipId(), HEX);
  if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass, TOPIC_AVAIL, 0, true, "offline")) {
    mqttClient.publish(TOPIC_AVAIL, "online", true);
    publishDiscovery();
    Serial.println("connected");
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
  }
}

void setupWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(cfgSsid);
  if (cfgSsid.length() == 0) {
    Serial.println("No SSID configured");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfgSsid.c_str(), cfgPassword.c_str());
  lastWiFiAttempt = millis();
  wifiConnectStart = millis();
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  bool loadedFromFS = loadConfigFromFS();
  applyFallbackFromSecrets();
  if (!loadedFromFS && cfgSsid.length() > 0 && cfgMqttServer.length() > 0) {
    saveConfigToFS();
  }

  setupWiFi();
  mqttClient.setServer(cfgMqttServer.c_str(), cfgMqttPort);
  mqttClient.setKeepAlive(30);

  setupDisplay();

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < wifiConnectTimeout) {
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    startConfigPortal("connect timeout");
  }
  if (portalActive) return;

  if (!bme.begin()) {
    Serial.println("BME680 init failed!");
    for (;;);
  }

  // Set up oversampling and filter for faster reading
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms
}

// -------------------- Loop --------------------
void loop() {
  if (portalActive) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    return;
  }

  if (WiFi.status() != WL_CONNECTED && (millis() - lastWiFiAttempt) > wifiReconnectInterval) {
    WiFi.disconnect();
    setupWiFi();
  }
  if (WiFi.status() != WL_CONNECTED && wifiConnectStart > 0 &&
      (millis() - wifiConnectStart) > wifiConnectTimeout) {
    startConfigPortal("reconnect timeout");
    return;
  }

  reconnectMqtt();
  mqttClient.loop();

  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();
    readBME680();
    updateDisplay();
  }

  

  if (mqttClient.connected() && (millis() - lastMqttUpdate >= mqttInterval)) {
    lastMqttUpdate = millis();
    publishState();
  }
}
