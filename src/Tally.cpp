#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "config.h"

// Runtime Variables (Loaded from EEPROM)
String wifi_ssid = "";
String wifi_pass = "";
String vmix_ip = "";
String guid = "";

String static_ip = "";
String static_gw = "";
String static_sn = "";
bool use_dhcp = true;

// Web server
ESP8266WebServer server(80);

unsigned long lastRequestTime = 0;
const unsigned long requestInterval = 300;
bool isAPMode = false;

// --- Device States ---
enum TallyState
{
  STATE_BOOTING,
  STATE_AP_MODE,
  STATE_NO_VMIX,
  STATE_OFF_AIR,
  STATE_PREVIEW,
  STATE_PROGRAM
};

TallyState currentState = STATE_BOOTING;

void setup()
{
  Serial.begin(115200);

  // Set standard PWM range for ESP8266 (0-255)
  analogWriteRange(255);

  // Initialize EEPROM and Load Config
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();

  // Setup LED Pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  currentState = STATE_BOOTING;

  // Attempt to connect to WiFi
  WiFi.mode(WIFI_STA);
  if (wifi_ssid.length() > 0)
  {
    // Apply Static IP if DHCP is disabled
    if (!use_dhcp && static_ip.length() > 7)
    {
      IPAddress ip, gw, sn;
      ip.fromString(static_ip);
      gw.fromString(static_gw);
      sn.fromString(static_sn);
      WiFi.config(ip, gw, sn);
      Serial.println("Using Static IP Config");
    }
    else
    {
      Serial.println("Using DHCP");
    }
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    Serial.print("Connecting to WiFi: ");
    Serial.println(wifi_ssid);

    // Wait up to 10 seconds for connection, but keep animating LEDs
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 200)
    {
      updateLEDs(); // Keep the fade animation running
      delay(50);    // Small delay prevents hardware watchdog reset while keeping fade smooth
      if (attempts % 10 == 0)
        Serial.print(".");
      attempts++;
    }
  }

  // Fallback to AP Mode if connection failed or no SSID is set
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nWiFi Failed! Starting Access Point Mode.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("Connect to AP: ");
    Serial.println(AP_SSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    isAPMode = true;
    currentState = STATE_AP_MODE;
  }
  else
  {
    Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
    // Default to NO_VMIX until we get our first successful HTTP response
    currentState = STATE_NO_VMIX;
  }

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("HTTP Server Started");
}

void loop()
{
  server.handleClient();

  // Constantly update LEDs (handles animations without blocking)
  updateLEDs();

  // Only poll vMix if we are connected to a network and have a valid IP/GUID
  if (!isAPMode && guid.length() > 0 && vmix_ip.length() > 0 && (millis() - lastRequestTime >= requestInterval))
  {
    lastRequestTime = millis();
    checkTallyStatus();
  }
}

// --- LED State Machine & Animations ---

void updateLEDs()
{
  unsigned long t = millis();

  switch (currentState)
  {
  case STATE_BOOTING:
  {
    // Slow fade blank to yellow
    float intensity = (cos(t * 3.14159 / 1000.0) + 1.0) / 2.0;
    setColor(255 * intensity, 127 * intensity, 0);
    break;
  }
  case STATE_AP_MODE:
  {
    // 3 Blink Blue Loop (2-second total cycle)
    int cycle = t % 2000;
    if (cycle < 150 || (cycle > 300 && cycle < 450) || (cycle > 600 && cycle < 750))
    {
      setColor(0, 0, 255); // Blue
    }
    else
    {
      setColor(0, 0, 0); // Blank
    }
    break;
  }
  case STATE_NO_VMIX:
  {
    // 3 Blink Yellow Loop (2-second total cycle)
    int cycle = t % 2000;
    if (cycle < 150 || (cycle > 300 && cycle < 450) || (cycle > 600 && cycle < 750))
    {
      setColor(255, 127, 0); // Yellow
    }
    else
    {
      setColor(0, 0, 0); // Blank
    }
    break;
  }
  case STATE_OFF_AIR:
    setColor(0, 0, 127); // Solid dim blue
    break;
  case STATE_PREVIEW:
    setColor(255, 127, 0); // Solid yellow
    break;
  case STATE_PROGRAM:
    setColor(255, 0, 0); // Solid red
    break;
  }
}

void setColor(int red, int green, int blue)
{
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

// --- Tally Logic ---

void checkTallyStatus()
{
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + vmix_ip + "/tallyupdate/?key=" + guid;

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0)
  {
    String payload = http.getString();
    parseTallyStatus(payload);
  }
  else
  {
    Serial.println("HTTP Error: " + http.errorToString(httpCode));
    currentState = STATE_NO_VMIX; // Fallback to error blink if vMix drops
  }
  http.end();
}

void parseTallyStatus(String payload)
{
  // Update state rather than setting colors directly
  if (payload.indexOf(PGM_COLOR) != -1)
  {
    currentState = STATE_PROGRAM;
  }
  else if (payload.indexOf(PRV_COLOR) != -1)
  {
    currentState = STATE_PREVIEW;
  }
  else
  {
    currentState = STATE_OFF_AIR;
  }
}

// --- EEPROM Management ---

void loadConfig()
{
  wifi_ssid = readEEPROMString(EEPROM_SSID_ADDR, EEPROM_SSID_LEN);
  wifi_pass = readEEPROMString(EEPROM_PASS_ADDR, EEPROM_PASS_LEN);
  vmix_ip = readEEPROMString(EEPROM_VMIX_IP_ADDR, EEPROM_VMIX_IP_LEN);
  guid = readEEPROMString(EEPROM_GUID_ADDR, EEPROM_GUID_LEN);
  static_ip = readEEPROMString(EEPROM_STATIC_IP_ADDR, EEPROM_STATIC_IP_LEN);
  static_gw = readEEPROMString(EEPROM_STATIC_GW_ADDR, EEPROM_STATIC_GW_LEN);
  static_sn = readEEPROMString(EEPROM_STATIC_SN_ADDR, EEPROM_STATIC_SN_LEN);
  String dhcpFlag = readEEPROMString(EEPROM_USE_DHCP_ADDR, EEPROM_USE_DHCP_LEN);
  use_dhcp = (dhcpFlag == "0") ? false : true; // Default to true if empty
}

void saveConfig()
{
  writeEEPROMString(EEPROM_SSID_ADDR, EEPROM_SSID_LEN, wifi_ssid);
  writeEEPROMString(EEPROM_PASS_ADDR, EEPROM_PASS_LEN, wifi_pass);
  writeEEPROMString(EEPROM_VMIX_IP_ADDR, EEPROM_VMIX_IP_LEN, vmix_ip);
  writeEEPROMString(EEPROM_GUID_ADDR, EEPROM_GUID_LEN, guid);
  writeEEPROMString(EEPROM_STATIC_IP_ADDR, EEPROM_STATIC_IP_LEN, static_ip);
  writeEEPROMString(EEPROM_STATIC_GW_ADDR, EEPROM_STATIC_GW_LEN, static_gw);
  writeEEPROMString(EEPROM_STATIC_SN_ADDR, EEPROM_STATIC_SN_LEN, static_sn);
  writeEEPROMString(EEPROM_USE_DHCP_ADDR, EEPROM_USE_DHCP_LEN, use_dhcp ? "1" : "0");
  EEPROM.commit();
  Serial.println("Config saved to EEPROM!");
}

String readEEPROMString(int start, int maxLength)
{
  String value = "";
  for (int i = 0; i < maxLength; i++)
  {
    char c = EEPROM.read(start + i);
    if (c == '\0' || c == 255)
      break;
    value += c;
  }
  return value;
}

void writeEEPROMString(int start, int maxLength, String value)
{
  for (int i = 0; i < maxLength; i++)
  {
    if (i < value.length())
    {
      EEPROM.write(start + i, value[i]);
    }
    else
    {
      EEPROM.write(start + i, '\0');
    }
  }
}

// --- Web Interface ---

void handleRoot() {
  String html = "<html><head><title>Tally Config</title><style>";
  html += "* { text-align: center; font-family: sans-serif; color: white; }";
  html += "body { display: flex; align-items: center; justify-content: center; height: 100vh; background-color: #303030; margin: 0; }";
  html += "#container { border: solid 2px #8b1fbd; border-radius: 20px; width: 350px; padding: 20px; background: #222; }";
  html += ".input { margin-bottom: 10px; color: black; width: 90%; padding: 5px; }";
  html += "#save { width: 100%; padding: 10px; background-color: blueviolet; border-radius: 5px; border: none; cursor: pointer; margin-top: 15px;}";
  html += "#save:hover { background-color: #7017c4; }";
  html += "</style>";
  html += "<script>function toggleStatic(cb) { document.getElementById('static_settings').style.display = cb.checked ? 'none' : 'block'; }</script>";
  html += "</head><body><div id='container'><h1>Tally Configuration</h1>";
  
  if (isAPMode) html += "<p style='color: yellow;'>Currently in AP Setup Mode</p>";

  html += "<form action='/save' method='GET'>";
  html += "<label>WiFi SSID:</label><br><input class='input' type='text' name='ssid' value='" + wifi_ssid + "'><br>";
  html += "<label>WiFi Password:</label><br><input class='input' type='password' name='pass' value='" + wifi_pass + "'><br>";
  html += "<label>vMix IP (e.g. 192.168.1.50:8088):</label><br><input class='input' type='text' name='vmix_ip' value='" + vmix_ip + "'><br>";
  html += "<label>Input Name or GUID:</label><br><input class='input' type='text' name='guid' value='" + guid + "'><br><hr>";
  
  String checked = use_dhcp ? "checked" : "";
  String display = use_dhcp ? "none" : "block";
  
  html += "<div style='margin-bottom: 10px;'><label>Use DHCP:</label> <input type='checkbox' name='use_dhcp' value='1' onchange='toggleStatic(this)' " + checked + "></div>";
  html += "<div id='static_settings' style='display:" + display + ";'>";
  html += "<label>Static IP:</label><br><input class='input' type='text' name='static_ip' value='" + static_ip + "'><br>";
  html += "<label>Gateway:</label><br><input class='input' type='text' name='static_gw' value='" + static_gw + "'><br>";
  html += "<label>Subnet Mask:</label><br><input class='input' type='text' name='static_sn' value='" + static_sn + "'><br>";
  html += "</div>";

  html += "<input type='submit' value='Save & Reboot' id='save'>";
  html += "</form></div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid")) wifi_ssid = server.arg("ssid");
  if (server.hasArg("pass")) wifi_pass = server.arg("pass");
  if (server.hasArg("vmix_ip")) vmix_ip = server.arg("vmix_ip");
  if (server.hasArg("guid")) guid = server.arg("guid");
  
  // A checkbox only sends a value if it is checked
  use_dhcp = server.hasArg("use_dhcp");
  
  if (server.hasArg("static_ip")) static_ip = server.arg("static_ip");
  if (server.hasArg("static_gw")) static_gw = server.arg("static_gw");
  if (server.hasArg("static_sn")) static_sn = server.arg("static_sn");
  
  saveConfig(); 
  
  server.send(200, "text/html", "<html><body style='background-color:#303030; color:white; text-align:center; font-family:sans-serif;'><h1>Saved! Rebooting...</h1></body></html>");
  
  delay(1000);
  ESP.restart(); 
}