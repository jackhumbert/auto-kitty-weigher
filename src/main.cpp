#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "secrets.h"

StaticJsonDocument<200> doc;
WiFiClient client;

int status = WL_IDLE_STATUS;

void updateJsonDoc(float weight) {
  doc.clear();
  JsonObject root = doc.to<JsonObject>();
  JsonArray records = root.createNestedArray("records");
  JsonObject record = records.createNestedObject();
  JsonObject fields = record.createNestedObject("fields");
  fields["Datetime"] = "2023-02-03T02:08:00.000Z";
  fields["Weight"] = weight;
}

void sendWeightData(float weight) {
  updateJsonDoc(weight);
  if (WiFi.status() == WL_CONNECTED) {
    if (client.connect("api.airtable.com", 443)) {
      client.println("POST /v0/" AIRTABLE_BASE " HTTP/1.1");
      client.println("Host: api.airtable.com");
      client.println("Authorization: Bearer " AIRTABLE_AUTH);
      client.print("Content-Length: ");
      client.println(measureJson(doc));
      client.println("Content-Type: application/json");
      client.println();
      serializeJson(doc, client);
      // client.println(postData.length());
      // client.println();
      // client.print(postData);

      // print json
      // serializeJson(doc, Serial);
      // Serial.println();

      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          Serial.write(c);
        }
      }
      // if (!client.connected()) {
      Serial.println();
      Serial.println("Disconnecting from server...");
      client.stop();
      // }

    } else {
      Serial.println("Could not connect to server");
    }
    // if (client.connected()) {
    // client.stop();
    // }
  } else {
    Serial.println("WiFi not connected");
  }
}

void setup() { 
  // delay(1000);
  Serial.begin(115200);
  // delay(10);
  while (!Serial) {
  ;
  }

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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // if (WiFi.status()== WL_CONNECTED) {
  // Serial.println("connected to wifi");
  sendWeightData(10.5);
  // } else {
  // Serial.println("not connected to wifi");
  // }
  Serial.println("Setup done");
}
void loop() {
  Serial.println("loop start");
  delay(5000);
}