#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>
#include <IRremote.hpp>

// IR Remote Codes
#define IR_RECEIVE_PIN 4  // IR receiver pin

#define BUTTON_RED 0x936C0707
#define BUTTON_GREEN 0xEB140707
#define BUTTON_YELLOW 0xEA150707
#define BUTTON_BLUE 0xE9160707
#define OFF 0x19E60707

//---ESP-NOW--------
#define MAX_RETRIES 25
#define TOUCH_THRESHOLD 40
#define CHANNEL 1

// Constant used by WLED from wled00/remote.cpp 
#define ON 1
#define OFF_CMD 2
#define NIGHT 3
#define PRESET_ONE 16
#define PRESET_TWO 17
#define PRESET_THREE 18
#define PRESET_FOUR 19
#define BRIGHT_UP 9
#define BRIGHT_DOWN 8

// State of the LED
RTC_DATA_ATTR bool is_lightOn = false;
RTC_DATA_ATTR uint32_t seq = 1;  // Sequence number

// Modify with the WLED mac address 
const uint8_t macAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};    // 

int retriesCount = 0;
esp_now_peer_info_t peerInfo;

// Message structure from wled00/remote.cpp
typedef struct message_structure {
  uint8_t program = 0x81;  // 0x91 for ON button, 0x81 for all others
  uint8_t seq[4];  // Incremental sequence number 32-bit unsigned integer LSB first
  uint8_t byte5 = 32;  // Unknown
  uint8_t button;  // Identifies which button is being pressed
  uint8_t byte8 = 1;  // Unknown, but always 0x01
  uint8_t byte9 = 100;  // Unknown, but always 0x64

  uint8_t byte10;  // Unknown, maybe checksum
  uint8_t byte11;  // Unknown, maybe checksum
  uint8_t byte12;  // Unknown, maybe checksum
  uint8_t byte13;  // Unknown, maybe checksum
} message_structure;

message_structure message;

//================================================================

// Callback on data sent. If the message delivery fails, retry until success or MAX_RETRIES
void sentStatusAndRetries(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Delivery Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Success");
  } else {
    Serial.println("Fail. Retrying");

    if (retriesCount < MAX_RETRIES) {
      esp_now_send(macAddress, (uint8_t *)&message, sizeof(message));
      retriesCount += 1;
    }
    retriesCount = 0;
  }
}

void sendMessage(int button) {
  // Increase seq number
  seq += 1;
  Serial.println(button);

  // Format seq number
  message.seq[0] = seq;
  message.seq[1] = seq >> 8;
  message.seq[2] = seq >> 16;
  message.seq[3] = seq >> 24;

  Serial.print("SEQ:");
  Serial.println(seq);

  message.button = button;

  esp_err_t result = esp_now_send(macAddress, (uint8_t *)&message, sizeof(message));

  // Display the error
  switch (result) {
    case ESP_OK:
      break;

    case ESP_ERR_ESPNOW_NOT_INIT:
      Serial.println("ESP-NOW not initialized");
      break;

    case ESP_ERR_ESPNOW_ARG:
      Serial.println("Invalid argument");
      break;

    case ESP_ERR_ESPNOW_INTERNAL:
      Serial.println("Internal error");
      break;

    case ESP_ERR_ESPNOW_NO_MEM:
      Serial.println("Out of memory");
      break;

    case ESP_ERR_ESPNOW_NOT_FOUND:
      Serial.println("Peer not found");
      break;

    case ESP_ERR_ESPNOW_IF:
      Serial.println("Interface error");
      break;

    default:
      Serial.print("Unknown error code: ");
      Serial.println(result);
      break;
  }
}

//============================================================================

void setup() {
  Serial.begin(115200);

  // Wifi configuration
  WiFi.mode(WIFI_STA);
  WiFi.begin(); 
  WiFi.channel(CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Message sending callback function
  esp_now_register_send_cb(sentStatusAndRetries);
  
  memcpy(peerInfo.peer_addr, macAddress, 6);
  peerInfo.channel = CHANNEL;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer fail");
    return;
  }

  Serial.println(is_lightOn);

  // IR_Receiver Init
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  delay(200);
}

void loop() {
  if (IrReceiver.decode()) {
    long receivedCode = IrReceiver.decodedIRData.decodedRawData; // Get the raw data from the IR signal

    Serial.print("Received Code: ");
    Serial.println(receivedCode, HEX); // Print the received code in hexadecimal format

    
    switch (receivedCode) {
      case OFF:
        if (is_lightOn) {
          sendMessage(OFF_CMD);
          is_lightOn = false;
          Serial.println("Light Off");
        } else {
          sendMessage(ON);
          is_lightOn = true;
          Serial.println("Light On");
        }
        break;

      case BUTTON_RED:
        Serial.println("Red Button Pressed!");
        sendMessage(PRESET_ONE);
        break;

      case BUTTON_GREEN:
        Serial.println("Green Button Pressed!");
        sendMessage(PRESET_TWO);
        break;

      case BUTTON_YELLOW:
        Serial.println("Yellow Button Pressed!");
        sendMessage(PRESET_THREE);
        break;

      case BUTTON_BLUE:
        Serial.println("Blue Button Pressed!");
        sendMessage(PRESET_FOUR);
        break;

      default:
        Serial.println("Unknown Button Pressed");
        break;
    }

    IrReceiver.resume(); // Ready to receive the next IR signal
  }
  delay(500);
}
