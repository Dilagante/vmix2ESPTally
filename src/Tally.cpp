#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// WiFi Credentials
const char* ssid = "";
const char* password = "";


// Static IP Configuration - Comment out for DHCP with Line 48
IPAddress staticIP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


// Web server
ESP8266WebServer server(80);

// LED Pins
const int redPin   = 5;
const int greenPin = 4;
const int bluePin  = 0;

String vmix_ip;
String guid;

//CHANGE IF YOU DON'T USE DEFAULT VMIX COLOURS
const char* PrvColor = "#ff8c00";
const char* PgmColor = "#ff0000";

// EEPROM Addresses
#define EEPROM_SIZE 100
#define VMIX_IP_ADDR 0
#define GUID_ADDR 40

unsigned long lastRequestTime = 0;
const unsigned long requestInterval = 300; // Check vMix tally every 300ms

void setup() {
  Serial.begin(115200);

  // Start EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();

  // WiFi Setup with Static IP
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());

  // LED Setup
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("HTTP Server Started");
}

void loop() {
  server.handleClient();
  
  // If we have a valid GUID, check tally status periodically
  if (guid.length() > 0 && WiFi.status() == WL_CONNECTED && (millis() - lastRequestTime >= requestInterval)) {
    lastRequestTime = millis();
    checkTallyStatus();
  }
}

// Load stored settings from EEPROM
void loadConfig() {
  vmix_ip = readStringFromEEPROM(VMIX_IP_ADDR);
  guid = readStringFromEEPROM(GUID_ADDR);

  if (vmix_ip.length() == 0) vmix_ip = "127.0.0.1:8088"; // Default
  if (guid.length() > 0) Serial.println("Loaded GUID: " + guid);
}

// Save updated values to EEPROM
void saveConfig(String newIp, String newGuid) {
  writeStringToEEPROM(VMIX_IP_ADDR, newIp);
  writeStringToEEPROM(GUID_ADDR, newGuid);
  EEPROM.commit();
  guid = newGuid;
  Serial.println("Config saved!");
}

// Web Configuration Page
void handleRoot() {
String html = "<html>\
<head>\
  <title>vMix Config</title>\
  <style>\
    * { text-align: center; font-family: sans-serif; color: white; }\
    h1 { text-align: center; }\
    #container { border: solid 2px black; border-radius: 20px; border-color: rgb(139, 31, 189); width: 35%; padding: 10px; display: flex; flex-direction: column; gap: 5px; }\
    body { display: flex; align-items: center; justify-content: center; height: 100vh; background-color: rgb(48, 48, 48); }\
    #save { width: 25%; padding: 5px; background-color: blueviolet; border-radius: 5px; border: 1px; transition: 0.4s; }\
    #save:hover { background-color: rgb(112, 23, 196); transition: 0.4s; }\
    .input { margin: 5px; color: black; width: 80%; }\
    .form { display: flex; flex-direction: column; gap: 10px; align-items: center; justify-content: center; }\
  </style>\
</head>\
<body>\
  <div id=\"container\">\
    <h1>ESP8266 vMix Configuration</h1>\
    <form action=\"/save\" class=\"form\" method=\"GET\">\
      <label for=\"vmix_ip\">vMix IP:</label>\
      <input class=\"input\" type=\"text\" id=\"vmix_ip\" name=\"vmix_ip\" value=\"" + vmix_ip + "\">\
      <label for=\"guid\">GUID:</label>\
      <input class=\"input\" type=\"text\" id=\"guid\" name=\"guid\" value=\"" + guid + "\">\
      <input type=\"submit\" value=\"Save\" id=\"save\" />\
    </form>\
  </div>\
</body>\
</html>";
  server.send(200, "text/html", html);
}

// Handle Save Request
void handleSave() {
  if (server.hasArg("vmix_ip") && server.hasArg("guid")) {
    vmix_ip = server.arg("vmix_ip");
    guid = server.arg("guid");
    saveConfig(vmix_ip, guid); // Save to EEPROM
    server.send(200, "text/html", "<html>\
  <head>\
    <style>\
      * { text-align: center; font-family: sans-serif; color: white; }\
      body { background-color: rgb(48, 48, 48); height: 100vh; display: flex; align-items: center; justify-content: center; }\
    </style>\
  </head>\
  <body>\
    <h1>Configuration Updated! :)</h1><br>\
  </body>\
</html>");
  } else {
    server.send(400, "text/html", "Missing parameters!");
  }
}

// Query vMix for tally status
void checkTallyStatus() {
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + vmix_ip + "/tallyupdate/?key=" + guid;

  http.begin(client, url);
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    parseTallyStatus(payload);
    Serial.println("Tally Status: " + payload);
  } else {
    Serial.println("HTTP Error: " + http.errorToString(httpCode));
  }
  http.end();
}

// Analyze vMix tally response
void parseTallyStatus(String payload) {
  bool inPreview = (payload.indexOf(PrvColor) != -1);
  bool inProgram = (payload.indexOf(PgmColor) != -1);

  if (inProgram) {
    setColor(255, 0, 0);   // Red for Program
  } else if (inPreview) {
    setColor(255, 127, 0); // Yellow for Preview
  } else {
    setColor(0, 0, 127);   // Blue for Off
  }
}

// Set LED color
void setColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

// EEPROM Helper Functions
String readStringFromEEPROM(int address) {
  String value = "";
  for (int i = address; i < address + 40; i++) {
    char c = EEPROM.read(i);
    if (c == '\0') break;
    value += c;
  }
  return value;
}

void writeStringToEEPROM(int address, String value) {
  for (int i = address; i < address + 40; i++) {
    EEPROM.write(i, (i - address < value.length()) ? value[i - address] : '\0');
  }
}
