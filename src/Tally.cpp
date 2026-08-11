#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include "config.h"

// Runtime Variables (Loaded from EEPROM)
String wifi_ssid = "";
String wifi_pass = "";
String vmix_ip = "";
String inputID = "";

String static_ip = "";
String static_gw = "";
String static_sn = "";
bool use_dhcp = true;

String mdns_hostname = "tally"; // Default mDNS hostname for the device, can be changed in WebUI

// Web server
ESP8266WebServer server(80);
WiFiClient vmixClient;

unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 3000;
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
    currentState = STATE_NO_VMIX;
    if (MDNS.begin(mdns_hostname.c_str()))
    {
      Serial.println("mDNS responder started: http://" + mdns_hostname + ".local");
    }
  }

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void loop()
{
  if (!isAPMode)
    MDNS.update();
  server.handleClient();

  // Constantly update LEDs (handles animations without blocking)
  updateLEDs();

  if (!isAPMode && inputID.length() > 0 && vmix_ip.length() > 0)
  {
    handleTallyTCP();
  }
}

void handleTallyTCP() {
  // 1. Maintain Connection
  if (!vmixClient.connected()) {
    if (currentState == STATE_OFF_AIR || currentState == STATE_PREVIEW || currentState == STATE_PROGRAM) {
      currentState = STATE_NO_VMIX; // Set to error blink if we lose connection
    }
    
    if (millis() - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = millis();
      
      // Strip port if the user accidentally included ":8088" in the web UI
      String clean_ip = vmix_ip;
      int colonIdx = clean_ip.indexOf(':');
      if (colonIdx > 0) clean_ip = clean_ip.substring(0, colonIdx);
      
      Serial.print("Connecting to vMix TCP at ");
      Serial.println(clean_ip);
      
      if (vmixClient.connect(clean_ip.c_str(), 8099)) {
        Serial.println("Connected to vMix TCP!");
        vmixClient.println("SUBSCRIBE TALLY");
        currentState = STATE_OFF_AIR; // Default state until vMix pushes data
      }
    }
    return; // Stop here if not connected
  }
  
  // 2. Read Incoming Push Data
  while (vmixClient.available()) {
    String line = vmixClient.readStringUntil('\n');
    line.trim(); // Remove carriage return
    
    // Check if the message is a tally update
    if (line.startsWith("TALLY OK ")) {
      String tallyData = line.substring(9);
      
      // Convert user input (e.g., "1") to an array index (0-based)
      int inputIdx = inputID.toInt() - 1; 
      
      // Ensure the requested input exists in the tally string
      if (inputIdx >= 0 && inputIdx < tallyData.length()) {
        char status = tallyData.charAt(inputIdx);
        
        if (status == '1') {
          currentState = STATE_PROGRAM;
        } else if (status == '2') {
          currentState = STATE_PREVIEW;
        } else {
          currentState = STATE_OFF_AIR;
        }
      }
    }
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

// --- EEPROM Management ---

void loadConfig()
{
  wifi_ssid = readEEPROMString(EEPROM_SSID_ADDR, EEPROM_SSID_LEN);
  wifi_pass = readEEPROMString(EEPROM_PASS_ADDR, EEPROM_PASS_LEN);
  vmix_ip = readEEPROMString(EEPROM_VMIX_IP_ADDR, EEPROM_VMIX_IP_LEN);
  inputID = readEEPROMString(EEPROM_INPUTID_ADDR, EEPROM_INPUTID_LEN);
  static_ip = readEEPROMString(EEPROM_STATIC_IP_ADDR, EEPROM_STATIC_IP_LEN);
  static_gw = readEEPROMString(EEPROM_STATIC_GW_ADDR, EEPROM_STATIC_GW_LEN);
  static_sn = readEEPROMString(EEPROM_STATIC_SN_ADDR, EEPROM_STATIC_SN_LEN);
  String dhcpFlag = readEEPROMString(EEPROM_USE_DHCP_ADDR, EEPROM_USE_DHCP_LEN);
  use_dhcp = (dhcpFlag == "0") ? false : true; // Default to true if empty
  mdns_hostname = readEEPROMString(EEPROM_HOSTNAME_ADDR, EEPROM_HOSTNAME_LEN);
  if (mdns_hostname.length() == 0)
    mdns_hostname = "tally"; // Set default if empty
}

void saveConfig()
{
  writeEEPROMString(EEPROM_SSID_ADDR, EEPROM_SSID_LEN, wifi_ssid);
  writeEEPROMString(EEPROM_PASS_ADDR, EEPROM_PASS_LEN, wifi_pass);
  writeEEPROMString(EEPROM_VMIX_IP_ADDR, EEPROM_VMIX_IP_LEN, vmix_ip);
  writeEEPROMString(EEPROM_INPUTID_ADDR, EEPROM_INPUTID_LEN, inputID);
  writeEEPROMString(EEPROM_STATIC_IP_ADDR, EEPROM_STATIC_IP_LEN, static_ip);
  writeEEPROMString(EEPROM_STATIC_GW_ADDR, EEPROM_STATIC_GW_LEN, static_gw);
  writeEEPROMString(EEPROM_STATIC_SN_ADDR, EEPROM_STATIC_SN_LEN, static_sn);
  writeEEPROMString(EEPROM_USE_DHCP_ADDR, EEPROM_USE_DHCP_LEN, use_dhcp ? "1" : "0");
  writeEEPROMString(EEPROM_HOSTNAME_ADDR, EEPROM_HOSTNAME_LEN, mdns_hostname);
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

void handleRoot()
{
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

  if (isAPMode)
    html += "<p style='color: yellow;'>Currently in AP Setup Mode</p>";

  html += "<form action='/save' method='GET'>";
  html += "<label>WiFi SSID:</label><br><input class='input' type='text' name='ssid' value='" + wifi_ssid + "'><br>";
  html += "<label>WiFi Password:</label><br><input class='input' type='password' name='pass' value='" + wifi_pass + "'><br>";
  html += "<label>Device Name (mDNS):</label><br><input class='input' type='text' name='mdns_hostname' value='" + mdns_hostname + "'><br>";
  html += "<p style='font-size: 12px; margin-top: -10px;'>Access at: http://" + mdns_hostname + ".local</p>";
  html += "<label>vMix IP (e.g. 192.168.1.50:8088):</label><br><input class='input' type='text' name='vmix_ip' value='" + vmix_ip + "'><br>";
  html += "<label>Input Number:</label><br><input class='input' type='text' name='inputID' value='" + inputID + "'><br><hr>";

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

void handleSave()
{
  if (server.hasArg("ssid"))
    wifi_ssid = server.arg("ssid");
  if (server.hasArg("pass"))
    wifi_pass = server.arg("pass");
  if (server.hasArg("vmix_ip"))
    vmix_ip = server.arg("vmix_ip");
  if (server.hasArg("inputID"))
    inputID = server.arg("inputID");

  // A checkbox only sends a value if it is checked
  use_dhcp = server.hasArg("use_dhcp");

  if (server.hasArg("static_ip"))
    static_ip = server.arg("static_ip");
  if (server.hasArg("static_gw"))
    static_gw = server.arg("static_gw");
  if (server.hasArg("static_sn"))
    static_sn = server.arg("static_sn");

  if (server.hasArg("mdns_hostname"))
    mdns_hostname = server.arg("mdns_hostname");

  saveConfig();

  server.send(200, "text/html", "<html><body style='background-color:#303030; color:white; text-align:center; font-family:sans-serif;'><h1>Saved! Rebooting...</h1></body></html>");

  delay(1000);
  ESP.restart();
}