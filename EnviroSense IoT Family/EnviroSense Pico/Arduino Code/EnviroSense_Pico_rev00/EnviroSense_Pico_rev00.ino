#include <Wire.h>
#include <ESP8266WiFi.h>
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

// -------------------- Device Identity --------------------
const char* DEVICE_NAME = "EnviroSensePico";
const char* DEVICE_MODEL = "EnviroSense Pico";
const char* DEVICE_MFR = "ayaghini";
const char* SW_VERSION = "rev00";

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
const unsigned long displayInterval = 2000;
const unsigned long mqttInterval = 10000;
const unsigned long mqttReconnectInterval = 5000;

// -------------------- Display Layout --------------------
const int SCREEN_W = 160;
const int SCREEN_H = 80;
const int HEADER_H = 12;
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
  mqttClient.publish("homeassistant/sensor/enviroSense_temperature/config",
    R"({
      "name": "Temperature",
      "unique_id": "enviroSense_temperature",
      "state_topic": "EnviroSense/temperature",
      "unit_of_measurement": "°C",
      "device_class": "temperature",
      "availability_topic": "EnviroSense/status"
    })", true);

  // Humidity
  mqttClient.publish("homeassistant/sensor/enviroSense_humidity/config",
    R"({
      "name": "Humidity",
      "unique_id": "enviroSense_humidity",
      "state_topic": "EnviroSense/humidity",
      "unit_of_measurement": "%",
      "device_class": "humidity",
      "availability_topic": "EnviroSense/status"
    })", true);

  // Pressure
  mqttClient.publish("homeassistant/sensor/enviroSense_pressure/config",
    R"({
      "name": "Pressure",
      "unique_id": "enviroSense_pressure",
      "state_topic": "EnviroSense/pressure",
      "unit_of_measurement": "hPa",
      "device_class": "pressure",
      "availability_topic": "EnviroSense/status"
    })", true);

  // Gas
  mqttClient.publish("homeassistant/sensor/enviroSense_gas/config",
    R"({
      "name": "Gas",
      "unique_id": "enviroSense_gas",
      "state_topic": "EnviroSense/gas",
      "unit_of_measurement": "Ohm",
      "availability_topic": "EnviroSense/status"
    })", true);

  // AQI
  mqttClient.publish("homeassistant/sensor/enviroSense_aqi/config",
    R"({
      "name": "AQI",
      "unique_id": "enviroSense_aqi",
      "state_topic": "EnviroSense/aqi",
      "availability_topic": "EnviroSense/status"
    })", true);
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
void drawHeader(bool wifiOk, bool mqttOk) {
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, ST77XX_BLACK);
  tft.setCursor(2, 2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("EnviroSense");

  tft.setCursor(110, 2);
  tft.setTextColor(wifiOk ? ST77XX_GREEN : ST77XX_RED);
  tft.print("W");
  tft.setCursor(125, 2);
  tft.setTextColor(mqttOk ? ST77XX_GREEN : ST77XX_RED);
  tft.print("M");
}

void drawGrid() {
  tft.drawFastHLine(0, HEADER_H, SCREEN_W, ST77XX_DARKGREY);
  tft.drawFastHLine(0, HEADER_H + CELL_H, SCREEN_W, ST77XX_DARKGREY);
  tft.drawFastHLine(0, HEADER_H + CELL_H * 2, SCREEN_W, ST77XX_DARKGREY);
  tft.drawFastVLine(CELL_W, HEADER_H, SCREEN_H - HEADER_H, ST77XX_DARKGREY);
}

void drawCellLabel(const Cell& cell) {
  tft.setCursor(cell.x + 2, cell.y + 2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print(cell.label);
}

void drawCellValue(const Cell& cell, const String& value, uint16_t color) {
  tft.fillRect(cell.x, cell.y + 10, CELL_W, CELL_H - 10, ST77XX_BLACK);
  tft.setCursor(cell.x + 2, cell.y + 12);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.print(value);
}

void setupDisplay() {
  tft.initR(INITR_MINI160x80);   // Initialize ST7735
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);          // Adjust rotation if needed
  tft.invertDisplay(true);
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

  drawCellValue(CELL_TEMP, String(temperature, 1) + " C", ST77XX_RED);
  drawCellValue(CELL_HUM, String(humidity, 0) + " %", ST77XX_YELLOW);
  drawCellValue(CELL_PRES, String(pressure, 0) + " hPa", ST77XX_CYAN);
  drawCellValue(CELL_GAS, String((int)gas_resistance) + " ohm", ST77XX_BLUE);
  drawCellValue(CELL_AQI, aqiText, ST77XX_GREEN);
  drawCellValue(CELL_STAT, mqttClient.connected() ? "MQTT OK" : "MQTT OFF", ST77XX_WHITE);
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
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
  } else {
    Serial.println("\nWiFi connect timeout");
  }
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  setupWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setKeepAlive(30);

  setupDisplay();

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
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
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
