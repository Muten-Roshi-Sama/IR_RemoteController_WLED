#include <WiFi.h>
#include <HTTPClient.h>
#include <IRremote.hpp>
#include <ESPmDNS.h>  

// This code sends a http request to a local LED device (given its hostname, needs to be setup with WLED) to change its color using a normal TV remote.



#define IR_RECEIVE_PIN 4  // IR receiver pin

// WiFi credentials
const char* ssid = "";
const char* password = "";

// WLED Configuration
const char* WLED_HOSTNAME = "tvled.local";  // Hostname
const char* WLED_IP = "192.168.129.89";     // Static IP
const bool USE_IP = true;  // Set to `true` to use IP, `false` to use hostname


// Construct the correct URL dynamically
String WLED_URL = USE_IP ? "http://" + String(WLED_IP) + "/win&" : "http://" + String(WLED_HOSTNAME) + "/win&";

// IR Remote Codes
#define BUTTON_RED 0x936C0707
#define BUTTON_GREEN 0xEB140707
#define BUTTON_YELLOW 0xEA150707
#define BUTTON_BLUE 0xE9160707
#define OFF 0x19E60707

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Booting...");

  // IR_Receiver Init
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  // Connect to Wi-Fi
  // WiFi.disconnect(true);  // Clear old Wi-Fi settings
  delay(1000);
  WiFi.begin(ssid, password);

  Serial.println("⏳ Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi!");

  // Start mDNS service if using hostname
  if (!USE_IP) {
    Serial.println("🌍 Starting mDNS...");
    if (!MDNS.begin(WLED_HOSTNAME)) {
      Serial.println("❌ Failed to start mDNS!");
    } else {
      Serial.println("✅ mDNS started!");
    }

  testMDNS();


  }
}

void loop() {
  if (IrReceiver.decode()) {
    long receivedCode = IrReceiver.decodedIRData.decodedRawData; // Get the raw data from the IR signal

    Serial.print("Received Code: ");
    Serial.println(receivedCode, HEX); // Print the received code in hexadecimal format

    if (receivedCode == OFF) {
      Serial.println("OFF Button Pressed!");
      sendWLEDRequest("T=2"); // Toggle
    }
    else if (receivedCode == BUTTON_RED) {
      Serial.println("Red Button Pressed!");
      sendWLEDRequest("CL=FF0000");  // Change WLED color to RED
    }
    else if (receivedCode == BUTTON_GREEN) {
      Serial.println("Green Button Pressed!");
      sendWLEDRequest("CL=00FF00");  // Change WLED color to GREEN
    }
    else if (receivedCode == BUTTON_YELLOW) {
      Serial.println("Yellow Button Pressed!");
      sendWLEDRequest("CL=FFFF00");  // Change WLED color to YELLOW
    }
    else if (receivedCode == BUTTON_BLUE) {
      Serial.println("Blue Button Pressed!");
      sendWLEDRequest("CL=0000FF");  // Change WLED color to BLUE
    }
    else {
      Serial.println("Unknown Button Pressed");
    }

    IrReceiver.resume(); // Ready to receive the next IR signal
  }
}

// Function to send HTTP requests to WLED
void sendWLEDRequest(String command) {
  const int maxRetries = 3;  // Maximum number of retries
  const unsigned long retryDelay = 2000;  // Delay between retries (2 seconds)
  static int tries = 0;  // Track the number of retries

  HTTPClient http;
  String url = WLED_URL + command;  // Construct URL

  Serial.print("Sending request to: ");
  Serial.println(url);

  http.begin(url);  // Start HTTP request
  int httpCode = http.GET();  // Send GET request to WLED

  if (httpCode > 0) {
    Serial.println("✅ HTTP Request Sent Successfully!");
    tries = 0;  // Reset retry counter on success
  } else {
    Serial.print("❌ Error sending HTTP Request. Code: ");
    Serial.println(httpCode);

    if (tries < maxRetries) {
      Serial.print("Retrying in ");
      Serial.print(retryDelay / 1000);
      Serial.println(" seconds...");
      delay(retryDelay);  // Wait before retrying
      tries++;  // Increment retry counter
      sendWLEDRequest(command);  // Retry the request
    } else {
      Serial.println("❌ Max retries reached. Aborting request.");
      tries = 0;  // Reset retry counter
    }
  }

  http.end();  // End HTTP request
}

// Function to test mDNS resolution
void testMDNS() {
  IPAddress wledIP;
  if (WiFi.hostByName(WLED_HOSTNAME, wledIP)) {
    Serial.print("✅ Resolved WLED IP: ");
    Serial.println(wledIP);
  } else {
    Serial.println("❌ Failed to resolve WLED hostname!");
  }
}
