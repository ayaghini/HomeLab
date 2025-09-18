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

// Thresholds
const float tempThreshold = 1.0;      // degrees Celsius
const float humidityThreshold = 5.0;  // percent
const float gasThreshold = 100000.0;      // ohms (or adjust based on scale)

// -------------------- BME680 --------------------
Adafruit_BME680 bme; // I2C

// Global sensor values
float temperature = NAN;
float humidity = NAN;
float pressure = NAN;
float gas_resistance = NAN;

float tempOffset = -3.06;

const unsigned long sensorInterval = 10000; // read every 2 seconds
unsigned long lastSensorRead = 0;

// -------------------- Timers --------------------
unsigned long lastDisplayUpdate = 0;
unsigned long lastMqttUpdate = 0;
const unsigned long displayInterval = 2000;
const unsigned long mqttInterval = 10000;

// -------------------- MQTT Discovery --------------------
void publishDiscovery() {
  // Temperature
  mqttClient.publish("homeassistant/sensor/enviroSense_temperature/config",
    R"({
      "name": "Temperature",
      "state_topic": "EnviroSense/temperature",
      "unit_of_measurement": "°C",
      "device_class": "temperature"
    })", true);

  // Humidity
  mqttClient.publish("homeassistant/sensor/enviroSense_humidity/config",
    R"({
      "name": "Humidity",
      "state_topic": "EnviroSense/humidity",
      "unit_of_measurement": "%",
      "device_class": "humidity"
    })", true);

  // Pressure
  mqttClient.publish("homeassistant/sensor/enviroSense_pressure/config",
    R"({
      "name": "Pressure",
      "state_topic": "EnviroSense/pressure",
      "unit_of_measurement": "hPa",
      "device_class": "pressure"
    })", true);

  // Gas
  mqttClient.publish("homeassistant/sensor/enviroSense_gas/config",
    R"({
      "name": "Gas",
      "state_topic": "EnviroSense/gas",
      "unit_of_measurement": "Ohm"
    })", true);

  // AQI
  mqttClient.publish("homeassistant/sensor/enviroSense_aqi/config",
    R"({
      "name": "AQI",
      "state_topic": "EnviroSense/aqi"
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
void setupDisplay() {
  tft.initR(INITR_MINI160x80);   // Initialize ST7735
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);          // Adjust rotation if needed
  tft.invertDisplay(true);
  //tft.setTextColor(ST77XX_WHITE);
  //tft.setTextSize(1);
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
  if (lastAqiText != aqiText) updateRequired = true;

  if (!updateRequired) return; // nothing significant changed

  // Save current values as last displayed
  lastTemperature = temperature;
  lastHumidity = humidity;
  lastGas = gas_resistance;
  lastAqiText = aqiText;

  // ---- Redraw TFT ----
  tft.fillScreen(ST77XX_BLACK);  // clear screen
  int y = 5;

  // Gas
  tft.setCursor(1, y);
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(1);
  tft.print("Gas: ");
  tft.println((int)gas_resistance);
  yield();

  // AQI
  //tft.setCursor(1, y + 16);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.print("AQI: ");
  tft.println(aqiText);
  yield();

  // Temperature
  //tft.setCursor(1, y + 48);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.print("T:   ");
  tft.println((int)temperature);
  yield();

  // Humidity
  //tft.setCursor(1, y + 72);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.print("H:   ");
  tft.println((int)humidity);
  yield();

  // Pressure
  //tft.setCursor(1, y + 96);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.print("P:   ");
  tft.println((int)pressure);
  yield();
}






// -------------------- WiFi + MQTT --------------------
void reconnectMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("EnviroSensePico", mqtt_user, mqtt_pass)) {
  publishDiscovery();
} else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  setupWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);

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
  if (!mqttClient.connected()) {
    reconnectMqtt();
  }
  mqttClient.loop();

  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();
    readBME680();
    updateDisplay();
  }

  

  if (millis() - lastMqttUpdate >= mqttInterval) {
    lastMqttUpdate = millis();
    publishState();
  }
}
