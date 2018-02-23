/**
 * Example for sending AC current data
 * to the cloud using the ncd.io ac current monit and ESP8266
 *
 * Copyright (c) 2016 Losant IoT. All rights reserved.
 * https://www.losant.com
 * hardware 
 * https://store.ncd.io/product/esp8266-iot-communication-module-with-integrated-usb/
 * https://store.ncd.io/product/sht25-humidity-and-temperature-sensor-%C2%B11-8rh-%C2%B10-2c-i2c-mini-module/
 * https://store.ncd.io/product/i2c-shield-for-particle-electron-or-particle-photon-with-outward-facing-5v-i2c-port/
 */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Losant.h>
#include <Wire.h>

#define Addr 0x2A /// i2c address of Current Monit
float current_ch1;
float current_ch2;
float current1;
float current2;

const char* ssid = "XX";
const char* password = "XX";

const char* LOSANT_DEVICE_ID = "XX";
const char* LOSANT_ACCESS_KEY = "XX";
const char* LOSANT_ACCESS_SECRET = "XX";
WiFiClientSecure wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  delay(500); 
  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(password);
      Serial.println();
    }

    delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if(millis() - wifiConnectStart > 5000) {
      Serial.println("Failed to connect to WiFi");
      Serial.println("Please attempt to send updated configuration parameters.");
      return;
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.print("Authenticating Device...");
  HTTPClient http;
  http.begin("http://api.losant.com/auth/device");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = LOSANT_DEVICE_ID;
  root["key"] = LOSANT_ACCESS_KEY;
  root["secret"] = LOSANT_ACCESS_SECRET;
  String buffer;
  root.printTo(buffer);

  int httpCode = http.POST(buffer);

  if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
          Serial.println("This device is authorized!");
      } else {
        Serial.println("Failed to authorize device to Losant.");
        if(httpCode == 400) {
          Serial.println("Validation error: The device ID, access key, or access secret is not in the proper format.");
        } else if(httpCode == 401) {
          Serial.println("Invalid credentials to Losant: Please double-check the device ID, access key, and access secret.");
        } else {
           Serial.println("Unknown response from API");
        }
      }
    } else {
        Serial.println("Failed to connect to Losant API.");

   }

  http.end();

  // Connect to Losant.
  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
  Serial.println();
  Serial.println("This device is now ready for use!");
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);
  Wire.begin(12, 14);
  // Wait for serial to initialize.
  while(!Serial) { }

  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("AC PUMP HEALTH MONITOR");
  Serial.println("-------------------------------------");

  connect();
}
void report(double current_ch1, double current_ch2) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["current_ch1"] = current_ch1;
  Serial.println("     ");
  root["current_ch2"] = current_ch2;
  device.sendState(root);
  Serial.println("Reported!");
}

int timeSinceLastRead = 0;
void loop() {
   bool toReconnect = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  device.loop();

//////////////i2c starts over here /////////////////
  // Report every 2 seconds.
    // Start I2C Transmission
     // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Command header byte-1
    Wire.write(0x92);
    // Command header byte-2
    Wire.write(0x6A);
    // Command 1
    Wire.write(0x01);
    // Start Channel No.
    Wire.write(0x01);
    // End Channel No.
    Wire.write(0x01);
    // Reserved
    Wire.write(0x00);
    // Reserved
    Wire.write(0x00);
    // CheckSum
    Wire.write((0x92 + 0x6A + 0x01 + 0x01 + 0x01 + 0x00 + 0x00) & 0xFF);
    // Stop I2C Transmission
    Wire.endTransmission();
    delay(50);

    // Request 3 bytes of data
    Wire.requestFrom(Addr, 3);

    // Read 3 bytes of data
    // msb1, msb, lsb
    int msb1 = Wire.read();
    int msb = Wire.read();
    int lsb = Wire.read();
    double current_ch1 = (msb1 * 65536) + (msb * 256) + lsb;
   // Serial.print(current_ch1);
    // Convert the data to ampere
    current_ch1 = current_ch1 / 1000;
    delay(100);

     Wire.beginTransmission(Addr);
    // Command header byte-1
    Wire.write(0x92);
    // Command header byte-2
    Wire.write(0x6A);
    // Command 1
    Wire.write(0x01);
    // Start Channel No.
    Wire.write(0x02);
    // End Channel No.
    Wire.write(0x02);
    // Reserved
    Wire.write(0x00);
    // Reserved
    Wire.write(0x00);
    // CheckSum
    Wire.write((0x92 + 0x6A + 0x01 + 0x02 + 0x02 + 0x00 + 0x00) & 0xFF);
    // Stop I2C Transmission
    Wire.endTransmission();
    delay(50);

    // Request 3 bytes of data
    Wire.requestFrom(Addr, 3);

    // Read 3 bytes of data
    // msb1, msb, lsb
    int msb1_1 = Wire.read();
    int msb_1 = Wire.read();
    int lsb_1 = Wire.read();
    double current_ch2 = (msb1_1 * 65536) + (msb_1 * 256) + lsb_1;
    //Serial.print(current_ch2);
    // Convert the data to ampere
    current_ch2 = current_ch2 / 1000;
    delay(100);
//////////////i2c ends over here /////////////////

  if(timeSinceLastRead > 2000) {
    Serial.print("Current On Channel One ");
    Serial.print(current1);
    Serial.print(" %\t");
    Serial.print("Current On Channel Two ");
    Serial.print(current2);
    report(current_ch1, current_ch2);

    timeSinceLastRead = 0;
  }
  delay(100);
  timeSinceLastRead += 100;
}
