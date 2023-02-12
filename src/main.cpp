#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPDateTime.h>
#include <HX711.h>
#include <queue>

#include "secrets.h"

StaticJsonDocument<200> doc;
WiFiClientSecure client;
HX711 loadcell1;
HX711 loadcell2;
HX711 loadcell3;
HX711 loadcell4;

int status = WL_IDLE_STATUS;

#define IDLE_THRESOLD 0.01
#define CHANGING_THRESOLD 1.00
#define MAX_HISTORY 8
std::deque<float> history;

void updateJsonDoc(float weight, float r1, float r2, float r3, float r4) {
  doc.clear();
  JsonObject root = doc.to<JsonObject>();
  JsonObject fields = root.createNestedObject("fields");
  fields["Datetime"] = DateTime.toISOString();
  fields["Weight"] = weight;
  fields["Raw 1"] = r1;
  fields["Raw 2"] = r2;
  fields["Raw 3"] = r3;
  fields["Raw 4"] = r4;
}

void sendWeightData(float weight, float r1, float r2, float r3, float r4) {
  updateJsonDoc(weight, r1, r2, r3, r4);
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

  history = std::deque<float>();

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

  loadcell1.begin(D3, D4);
  loadcell1.set_offset(177988);
  loadcell1.set_scale(207413.581);
  loadcell2.begin(D5, D6);
  loadcell2.set_offset(551183);
  loadcell2.set_scale(212627.8286);
  loadcell3.begin(D7, D8);
  loadcell3.set_offset(323836);
  loadcell3.set_scale(215526.6095);
  loadcell4.begin(D9, D10);
  loadcell4.set_offset(136473);
  loadcell4.set_scale(223576.2286);
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

#define AVERAGE 4

enum ScaleState {
  Idle = 0,
  Changing = 1
};

ScaleState state = Idle;

void loop() {
  float w1, w2, w3, w4;
  float r1, r2, r3, r4;

  for (int i = 0; i < AVERAGE; i++) {
    w1 += loadcell1.get_units();
    w2 += loadcell2.get_units();
    w3 += loadcell3.get_units();
    w4 += loadcell4.get_units();

    r1 += loadcell1.read();
    r2 += loadcell2.read();
    r3 += loadcell3.read();
    r4 += loadcell4.read();
  }

  w1 /= AVERAGE;
  w2 /= AVERAGE;
  w3 /= AVERAGE;
  w4 /= AVERAGE;

  r1 /= AVERAGE;
  r2 /= AVERAGE;
  r3 /= AVERAGE;
  r4 /= AVERAGE;

  float total = w1 + w2 + w3 + w4;

  // see how read value compares to history
  float avg_diff = 0;
  float avg_perc = 0;
  if (history.size() == MAX_HISTORY) {
    for (int i = 0; i < MAX_HISTORY; i++) {
      avg_diff += (total - history[i]) / MAX_HISTORY;
      avg_perc += (total / history[i]) / MAX_HISTORY;
    }
  }

  if (state == Changing && abs(avg_diff) < IDLE_THRESOLD) {
    sendWeightData(total, r1, r2, r3, r4);
    Serial.println("IDLE STATE");
    state = Idle;
  } else if (state == Idle && abs(avg_diff) > CHANGING_THRESOLD) {
    Serial.println("CHANGING STATE");
    state = Changing;
  }

  // add value to history
  history.emplace_back(total);
  while (history.size() > MAX_HISTORY) {
    history.pop_front();
  }

  Serial.print(total, 3);
  Serial.print(" : ");
  Serial.print(avg_diff, 6);
  Serial.print(" : ");
  Serial.println(avg_perc, 6);
}