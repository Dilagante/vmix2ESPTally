//Variables for you to change!

const char* ssid = "MyCoolAP"; // Replace with your WiFi SSID
const char* password = "MySecurePassword"; // Replace with your WiFi password
const char* vmix_ip = "192.168.1.128:8088"; // Replace with your vMix IP address WITH Port
const String input = "b681a619-1468-452c-89ea-feebec33c0db"; // You can use either the Input Number or the GUID. So cool!

const int redPin = 5; //Pin Number for the Red Pin
const int greenPin = 4; //Pin Number for the Green Pin
const int bluePin = 0; //Pin Number for the Blue Pin

//Change the below boolean to "true" to use a StaticIP

bool useStaticIP = false;

IPAddress staticIP(192, 168, 1, 127); // Replace with your desired static IP
IPAddress gateway(192, 168, 1, 1); // Replace with your network gateway
IPAddress subnet(255, 255, 255, 0); // Replace with your network subnet mask