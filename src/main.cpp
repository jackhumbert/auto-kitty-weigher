#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPDateTime.h>
#include <DateTimeTZ.h>
#include <HX711.h>
#include <queue>
#include <ResponsiveAnalogRead.h>
#include <ESP_Google_Sheet_Client.h>

#include "secrets.h"

#define IDLE_THRESHOLD 4000
#define CHANGING_THRESHOLD 64000
#define ACTIVITY_THRESHOLD_1 8000
#define ACTIVITY_THRESHOLD_2 8000
#define ACTIVITY_THRESHOLD_3 8000
#define ACTIVITY_THRESHOLD_4 8000
#define MAX_HISTORY 16

StaticJsonDocument<200> doc;
WiFiClientSecure client;
HX711 loadcell1;
HX711 loadcell2;
HX711 loadcell3;
HX711 loadcell4;

ResponsiveAnalogRead analog1(0, false);
ResponsiveAnalogRead analog2(0, false);
ResponsiveAnalogRead analog3(0, false);
ResponsiveAnalogRead analog4(0, false);

int status = WL_IDLE_STATUS;

std::deque<float> history;
std::deque<float> history1;
std::deque<float> history2;
std::deque<float> history3;
std::deque<float> history4;

void updateJsonDoc(float r1, float r2, float r3, float r4) {
  doc.clear();
  JsonObject root = doc.to<JsonObject>();
  JsonObject fields = root.createNestedObject("fields");
  JsonArray values = root.createNestedArray("values");
  JsonArray values_array = values.createNestedArray();
  values_array.add(DateTime.toISOString());
  values_array.add(r1);
  values_array.add(r2);
  values_array.add(r3);
  values_array.add(r4);
  // fields["Datetime"] = DateTime.toISOString();
  // fields["Weight"] = weight;
  // fields["Raw 1"] = r1;
  // fields["Raw 2"] = r2;
  // fields["Raw 3"] = r3;
  // fields["Raw 4"] = r4;
}

void sendWeightData(float r1, float r2, float r3, float r4) {
  // updateJsonDoc(r1, r2, r3, r4);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Readying GSheets");
    while (!GSheet.ready()) {
      Serial.print(".");
    }
    Serial.println("");
    FirebaseJson response;
    FirebaseJson data;
    data.add("values");
    data.set("values/[0]/[0]", DateTime.format("%Y-%m-%d %H:%M:%S"));
    char buffer[10];
    sprintf(buffer, "%.2f", r1);
    data.set("values/[0]/[1]", buffer);
    sprintf(buffer, "%.2f", r2);
    data.set("values/[0]/[2]", buffer);
    sprintf(buffer, "%.2f", r3);
    data.set("values/[0]/[3]", buffer);
    sprintf(buffer, "%.2f", r4);
    data.set("values/[0]/[4]", buffer);
    data.toString(Serial, true);
    // bool success = GSheet.values.get(&response, "1o4j6W-qBNH-4BgIbn9iTBa7n-K6YHjyUozrD8Kfawug", "Data!B1:B1");
    GSheet.values.append(&response, "1o4j6W-qBNH-4BgIbn9iTBa7n-K6YHjyUozrD8Kfawug", "Data!A2:E2", &data, "USER_ENTERED", "OVERWRITE", "true", "FORMATTED_VALUE", "SERIAL_NUMBER");
    response.toString(Serial, true);
    
    // String json;
    // serializeJson(doc, json);
    // serializeJson(doc, Serial);
    // Serial.println();

    // HTTPClient http;
    // http.begin("https://api.airtable.com/v0/" AIRTABLE_BASE, root_ca);
    // http.addHeader("Authorization", "Bearer " AIRTABLE_AUTH);
    // http.addHeader("Content-Type", "application/json");
    // int code = http.POST(json);
    // Serial.print("HTTP Response code: ");
    // Serial.println(code);
    // if (code > 0) {
    //   String payload = http.getString();
    //   Serial.println(payload);
    // }
    // http.end();

  } else {
    Serial.println("WiFi not connected");
  }
}

void tokenStatusCallback(TokenInfo info)
{
    if (info.status == esp_signer_token_status_error)
    {
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        Serial.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else
    {
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}

void setup() { 
  // delay(1000);
  Serial.begin(9600);
  // delay(5000);
  // while (!Serial) {
  // ;
  // }

  analog1.setAnalogResolution(2147483647);
  analog1.setActivityThreshold(ACTIVITY_THRESHOLD_1);
  analog2.setAnalogResolution(2147483647);
  analog2.setActivityThreshold(ACTIVITY_THRESHOLD_2);
  analog3.setAnalogResolution(2147483647);
  analog3.setActivityThreshold(ACTIVITY_THRESHOLD_3);
  analog4.setAnalogResolution(2147483647);
  analog4.setActivityThreshold(ACTIVITY_THRESHOLD_4);

  history = std::deque<float>();
  history1 = std::deque<float>();
  history2 = std::deque<float>();
  history3 = std::deque<float>();
  history4 = std::deque<float>();

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
  DateTime.setTimeZone(TZ_America_Indiana_Indianapolis);
  // this method config ntp and wait for time sync
  // default timeout is 10 seconds
  DateTime.begin(/* timeout param */);
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  }

  loadcell1.begin(D3, D4);
  loadcell1.set_offset(179246);
  loadcell1.set_scale(198126.5595);
  loadcell2.begin(D5, D6);
  loadcell2.set_offset(632649);
  loadcell2.set_scale(210156.3237);
  loadcell3.begin(D7, D8);
  loadcell3.set_offset(333111);
  loadcell3.set_scale(211612.6061);
  loadcell4.begin(D9, D10);
  loadcell4.set_offset(188310);
  loadcell4.set_scale(230208.1782);

  // if (WiFi.status()== WL_CONNECTED) {
  // Serial.println("connected to wifi");
  // sendWeightData(10.5);
  // } else {
  // Serial.println("not connected to wifi");
  // }

  // GSheet.setTokenCallback(tokenStatusCallback);

  //Begin the access token generation for Google API authentication
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

  Serial.println("Setup done");
}

#define AVERAGE 4

enum ScaleState {
  Idle = 0,
  Changing = 1
};

ScaleState state = Idle;

void loop() {
  analog1.update(loadcell1.read());
  analog2.update(loadcell2.read());
  analog3.update(loadcell3.read());
  analog4.update(loadcell4.read());

  int w1, w2, w3, w4;
  // float r1, r2, r3, r4;

  // for (int i = 0; i < AVERAGE; i++) {
  //   w1 += loadcell1.get_units();
  //   w2 += loadcell2.get_units();
  //   w3 += loadcell3.get_units();
  //   w4 += loadcell4.get_units();

  //   r1 += loadcell1.read();
  //   r2 += loadcell2.read();
  //   r3 += loadcell3.read();
  //   r4 += loadcell4.read();
  // }

  // w1 /= AVERAGE;
  // w2 /= AVERAGE;
  // w3 /= AVERAGE;
  // w4 /= AVERAGE;

  // r1 /= AVERAGE;
  // r2 /= AVERAGE;
  // r3 /= AVERAGE;
  // r4 /= AVERAGE;

  w1 = analog1.getValue();
  w2 = analog2.getValue();
  w3 = analog3.getValue();
  w4 = analog4.getValue();

  // if (analog1.hasChanged() || analog2.hasChanged() || analog3.hasChanged() || analog4.hasChanged()) {
  //   sendWeightData(w1, w2, w3, w4);
  // }

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

  // add value to history
  history.emplace_back(total);
  history1.emplace_back(w1);
  history2.emplace_back(w2);
  history3.emplace_back(w3);
  history4.emplace_back(w4);
  while (history.size() > MAX_HISTORY) {
    history.pop_front();
    history1.pop_front();
    history2.pop_front();
    history3.pop_front();
    history4.pop_front();
  }

  if (state == Changing && abs(avg_diff) < IDLE_THRESHOLD) {
    float a1, a2, a3, a4;
    for (int i = 0; i < MAX_HISTORY; i++) {
      a1 += history1[i] / MAX_HISTORY;
      a2 += history2[i] / MAX_HISTORY;
      a3 += history3[i] / MAX_HISTORY;
      a4 += history4[i] / MAX_HISTORY;
    }
    Serial.print("AVERAGE:                ");
    Serial.print(a1);
    Serial.print(" ");
    Serial.print(a2);
    Serial.print(" ");
    Serial.print(a3);
    Serial.print(" ");
    Serial.println(a4);
    Serial.println("IDLE STATE");
    sendWeightData(a1, a2, a3, a4);
    state = Idle;
  } else if (state == Idle && abs(avg_diff) > CHANGING_THRESHOLD) {
    Serial.println("CHANGING STATE");
    state = Changing;
  }

  Serial.print(w1);
  Serial.print(" ");
  Serial.print(w2);
  Serial.print(" ");
  Serial.print(w3);
  Serial.print(" ");
  Serial.println(w4);
}