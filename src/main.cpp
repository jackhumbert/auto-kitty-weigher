#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPDateTime.h>
#include <HX711.h>

#include "secrets.h"

StaticJsonDocument<200> doc;
WiFiClientSecure client;
HX711 loadcell1;
HX711 loadcell2;

int status = WL_IDLE_STATUS;

void updateJsonDoc(float weight) {
  doc.clear();
  JsonObject root = doc.to<JsonObject>();
  JsonObject fields = root.createNestedObject("fields");
  fields["Datetime"] = DateTime.toISOString();
  fields["Weight"] = weight;
}

void sendWeightData(float weight) {
  updateJsonDoc(weight);
  if (WiFi.status() == WL_CONNECTED) {
    String json;
    serializeJson(doc, json);
    serializeJson(doc, Serial);
    Serial.println();

    HTTPClient http;
    http.begin("https://api.airtable.com/v0/" AIRTABLE_BASE, root_ca);
    http.addHeader("Authorization", "Bearer " AIRTABLE_AUTH);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json);
    Serial.print("HTTP Response code: ");
    Serial.println(code);
    if (code > 0) {
      String payload = http.getString();
      Serial.println(payload);
    }
    http.end();

  } else {
    Serial.println("WiFi not connected");
  }
}

void setup() { 
  // delay(1000);
  Serial.begin(9600);
  delay(5000);
  // while (!Serial) {
  // ;
  // }

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // setup datetime
  DateTime.setTimeZone("CST-5");
  // this method config ntp and wait for time sync
  // default timeout is 10 seconds
  DateTime.begin(/* timeout param */);
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  }

  loadcell1.begin(D1, D0);
  loadcell2.begin(D3, D2);
  // loadcell.set_scale();
  // loadcell.set_offset(50682624);
  // loadcell.tare();

  // if (WiFi.status()== WL_CONNECTED) {
  // Serial.println("connected to wifi");
  // sendWeightData(10.5);
  // } else {
  // Serial.println("not connected to wifi");
  // }

  Serial.println("Setup done");
}

void loop() {
  Serial.print("Weight: ");
  Serial.print(loadcell1.read_average(20));
  Serial.print(" & ");
  Serial.println(loadcell2.read_average(20));
  delay(1000);
}