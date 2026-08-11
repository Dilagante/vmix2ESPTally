#ifndef CONFIG_H
#define CONFIG_H

// --- Hardware Setup ---
const int RED_PIN   = 5;
const int GREEN_PIN = 4;
const int BLUE_PIN  = 0;

// The network the ESP will create if it cannot find a known WiFi network
const char* AP_SSID = "Tally-Setup";
const char* AP_PASS = "admin123";

// --- vMix Tally Colors ---
const char* PRV_COLOR = "#ff8c00";
const char* PGM_COLOR = "#ff0000";

// --- EEPROM Memory Map ---
// Total size allocated (increased to hold WiFi credentials)
#define EEPROM_SIZE 512 

// Starting addresses and max lengths for each stored variable
#define EEPROM_SSID_ADDR 0
#define EEPROM_SSID_LEN  32

#define EEPROM_PASS_ADDR 32
#define EEPROM_PASS_LEN  64

#define EEPROM_VMIX_IP_ADDR 100
#define EEPROM_VMIX_IP_LEN  32

#define EEPROM_GUID_ADDR 140
#define EEPROM_GUID_LEN  40

#endif