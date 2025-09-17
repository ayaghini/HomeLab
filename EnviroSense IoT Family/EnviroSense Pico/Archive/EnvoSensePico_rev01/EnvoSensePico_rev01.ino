//#include <UTFT.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <Credentials.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include "bsec.h"
#include "PubSubClient.h" // Connect and publish to the MQTT broker

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif
const char* ssid = mySSID;
const char* password = myPASSWORD;

// BME680 Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);
// Create an object of the class Bsec
Bsec iaqSensor;
String BMEoutput;
String accuracyState;
String insideTemp;
String insideH;
String aqiText;
String saqi;
String eCO2;
int aqIndex;
unsigned long previousAirMillis = 10;
const unsigned long airInterval = 1 * 60 * 1000;
int i = 0;
//int j = 0;

//TFT LCD
#define TFT_CS         D3
#define TFT_RST        RX
#define TFT_DC         D4
#define TFT_MOSI D7  // Data out
#define TFT_SCLK D5  // Clock out
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Define NTP Client to get time
const long utcOffsetInSeconds = -25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
WiFiClient client;
String timeToDisplay = "waiting to receive the time from server";

// MQTT
const char* mqtt_server = "192.168.1.92";  // IP of the MQTT broker
const char* humidity_topic = "home/EnviroSensePico/humidity";
const char* temperature_topic = "home/EnviroSensePico/temperature";
const char* aqi_topic = "home/EnviroSensePico/aqi";
const char* aqiS_topic = "home/EnviroSensePico/staticaqi";
const char* eCO2_topic = "home/EnviroSensePico/equvalentCO2";
const char* accuracy_topic = "home/EnviroSensePico/accuracy";
//const char* mqtt_username = "cdavid"; // MQTT username
//const char* mqtt_password = "cdavid"; // MQTT password
const char* clientID = "client_EnviroSensePico"; // MQTT client ID
PubSubClient mqttClient(mqtt_server, 1883, client);

//Time interval Settings
long interval = 30000;           // interval at which to send (milliseconds)
long previousMillis = interval;        // will store last time data was sent
long dispInterval = 7000;           // interval at which to change display (milliseconds)
long previousDispMillis = interval;        // will store last time data was sent
int state = 0;
int displayState = 0;




void setup() {

  tft.initR(INITR_MINI160x80);  // Init ST7735S mini display

  TextWriter("Enviro  Sense   Pico v1", ST77XX_GREEN, 3);
  delay(1000);
  TextWriter("        ALI     YAGHINI", ST77XX_BLUE, 3);
  delay(1000);

  TextWriter("             Wi-Fi ...", ST77XX_WHITE, 2);
  delay(1000);
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    //Serial.print ( "." );
  }

  TextWriter("             Updating Time", ST77XX_WHITE, 2);
  delay(1000);
  timeClient.begin();


  //BME680
  TextWriter("             BME-680      Setup..", ST77XX_WHITE, 2);
  delay(1000);
  Wire.begin();
  iaqSensor.begin(0x77, Wire);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  ArduinoOTA.begin();

}

void loop() {
  ArduinoOTA.handle();
  StateMachine();
}

void StateMachine() {
  unsigned long currentMillis = millis();
  switch (state) {
    case 0: {
        if (currentMillis - previousMillis > interval) {
          previousMillis = currentMillis;
          UpdateTime();
        }
        state++;
        break;
      }
    case 1: {
        if (iaqSensor.run()) {
          updateBME();
        }
        else {
          checkIaqSensorStatus();
        }
        state++;
        break;
      }
    case 2: {
        if (currentMillis - previousDispMillis > dispInterval) {
          previousDispMillis = currentMillis;
          DisplayManager();
        }
        state = 0;
        break;
      }
  }
}

void UpdateTime() {
  timeToDisplay = GetTime();
  //TextWriter(timeToDisplay.c_str(), ST77XX_WHITE, 2);
  //UpdateDisplayText();
}


String GetTime() {
  timeClient.update();
  //  int hour = timeClient.getHours();
  //  int minute =  timeClient.getMinutes();
  //  String sep = ":";
  //  String currentTime = hour + sep + minute;

  String currentTime = timeClient.getFormattedTime();
  currentTime.remove(5, 3);
  return currentTime;
}

void updateBME() {
  i++;
  UpdateBMEText();
  if (i >= 500) {
    //DisplayManager();
    SendMQTT();
    i = 0;
    // j = 0;
  }
}

void UpdateBMEText() {
  aqIndex = iaqSensor.iaq;
  aqiText = String(aqIndex);
  accuracyState = String(iaqSensor.iaqAccuracy);
  insideTemp = String(iaqSensor.temperature);
  insideH =  String(iaqSensor.humidity);
  saqi =  String(iaqSensor.staticIaq);
  eCO2 = String(iaqSensor.co2Equivalent);
}

void DisplayManager() {
  AlertManager();
  switch (displayState) {
    case 0: {
        DisplayTime();
        displayState++;
        break;
      }
    case 1: {
        DisplayAqi();
        displayState ++;
        break;
      }
    case 2: {
        DisplayTemp();
        displayState = 0;
        break;
      }
  }
}

void DisplayAqi() {
  //tft.setCursor(0, 30);
  tft.setCursor(1, 1);
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(2);
  tft.println(eCO2 + "ppm");
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(4);
  tft.println("AQI" + aqiText);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println(insideH + "%");
}

void DisplayTime() {
  tft.setCursor(1, 1);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.println("AQI: " +aqiText);
  //tft.setCursor(0, 30);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(4);
  tft.println(" " + timeToDisplay);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.setTextSize(2);
  tft.println(insideTemp + "c");
}

void DisplayTemp() {
  tft.setCursor(1, 1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println(timeToDisplay);
  //tft.setCursor(0, 30);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.setTextSize(4);
  tft.println(insideTemp + "c");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println(insideH + "%");
}

void AlertManager() {
  if (aqIndex <= 200) {
    tft.fillScreen(ST77XX_BLACK);
  } else {
    tft.fillScreen(ST77XX_RED);
  }
}

void SendMQTT() {
  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (mqttClient.connect(clientID)) {
    SendMQTTMessages(humidity_topic, String(insideH));
    SendMQTTMessages(aqi_topic, String(aqIndex));
    SendMQTTMessages(accuracy_topic, String(accuracyState));
    SendMQTTMessages(aqiS_topic, String(saqi));
    //SendMQTTMessages(aqiS_topic, saqi));
    SendMQTTMessages(eCO2_topic, String(eCO2));
    SendMQTTMessages(temperature_topic, String(insideTemp));

    mqttClient.disconnect();  // disconnect from the MQTT broker
    //errLeds();
  }
  else {
    //Serial.println("Connection to MQTT Broker failed...");
  }
}

void SendMQTTMessages(const char* topic, String parameter) {
  if (!mqttClient.publish(topic, parameter.c_str())) {

    // Again, client.publish will return a boolean value depending on whether it succeeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.

    //Serial.println("Temperature failed to send. Reconnecting to MQTT Broker and trying again");
    mqttClient.connect(clientID);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    mqttClient.publish(topic, parameter.c_str());
  }
}


void TextWriter(String text, uint16_t color, int size) {
  tft.fillScreen(ST77XX_BLACK);
  tft.invertDisplay(true);
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);

}

void rotateText() {
  for (uint8_t i = 0; i < 4; i++) {
    tft.fillScreen(ST77XX_BLACK);
    Serial.println(tft.getRotation(), DEC);

    tft.setCursor(0, 30);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.println("Hello World!");
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(2);
    tft.println("Hello World!");
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(3);
    tft.println("Hello World!");
    tft.setTextColor(ST77XX_BLUE);
    tft.setTextSize(4);
    tft.print(1234.567);
    delay(1000);

    tft.setRotation(tft.getRotation() + 1);
  }
}

// BME680 Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      //output = "BSEC error code : " + String(iaqSensor.status);
      //Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      //output = "BSEC warning code : " + String(iaqSensor.status);
      //Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      //output = "BME680 error code : " + String(iaqSensor.bme680Status);
      //Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      //output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      //Serial.println(output);
    }
  }
}

//BME680 Reated
void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}
