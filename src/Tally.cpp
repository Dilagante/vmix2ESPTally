#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "config.h"

void flashLED();
void flashErrorLED();
int getInputNumberFromKey(const String &data, const String &inputKey);
int extractValue(const String &payload, const String &tag);
void parseTallyStatus(String payload);
void setColor(int red, int green, int blue);


void setup() {
  Serial.begin(115200);

  if (useStaticIP) {
    WiFi.config(staticIP, gateway, subnet);
  }
  WiFi.begin(ssid, password);
  
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 20) {  // Around 10 Seconds
      Serial.println("\nFailed to connect to WiFi.");
      return;
    }
  }
  Serial.println("\nConnected to WiFi");
  flashLED();

}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String url = "http://" + String(vmix_ip) + "/api/";
    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      parseTallyStatus(payload);
    } else {
      Serial.println("Error on HTTP request: " + String(http.errorToString(httpCode)));
      flashErrorLED();
    }
    http.end();
  } else {
    Serial.println("WiFi not connected.");
    WiFi.reconnect();
  }
}

void flashLED() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

void flashErrorLED() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
  }
}

int getInputNumberFromKey(const String &data, const String &inputKey) { //If GUID is used, check and convert it to the input number
  int keyPos = data.indexOf("key=\"" + inputKey + "\"");

  int numStart = data.indexOf("number=\"", keyPos);

  numStart += 8;
  int numEnd = data.indexOf("\"", numStart);
  return data.substring(numStart, numEnd).toInt();
}

int extractValue(const String &payload, const String &tag) { //Find Preview and Active tags in VmixAPI xml

  int start = payload.indexOf("<" + tag + ">");
  start += tag.length() + 2;

  int end = payload.indexOf("</" + tag + ">", start);
  return payload.substring(start, end).toInt();
}

void parseTallyStatus(String payload) { //Main Logic for Checking Status and Changing Colors
  bool inPreview = false;
  bool inProgram = false;
  int input_id;

  int PreviewNo = extractValue(payload, "preview");
  int ProgramNo = extractValue(payload, "active");

  Serial.println("PreviewNo:" + String(PreviewNo));
  Serial.println("ProgramNo:" + String(ProgramNo));

  if (input.indexOf("-") != -1) {
    input_id = getInputNumberFromKey(payload, input);
  }
  else {
    input_id = input.toInt();
  }

  if (String(input_id) == String(PreviewNo)) {
      inPreview = true;
    }
    else if (String(input_id) == String(ProgramNo)) {
      inProgram = true;
  }


  if (inProgram) {
    setColor(255, 0, 0);
  } else if (inPreview) {
    setColor(255, 127, 0);
  } else {
    setColor(0, 0, 127);
  }
}

void setColor(int red, int green, int blue) { //Set color to pins
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}
