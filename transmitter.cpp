#include <esp_now.h>
#include <WiFi.h>
#include "Adafruit_APDS9960.h" // Ensure this library is installed in your Arduino IDE

// Explicitly define gesture constants if the library isn't making them globally available
// These values are standard for the APDS-9960 sensor's gesture output
#ifndef APDS9960_GESTURE_NONE
#define APDS9960_GESTURE_NONE 0
#endif
#ifndef APDS9960_GESTURE_UP
#define APDS9960_GESTURE_UP 0x01
#endif
#ifndef APDS9960_GESTURE_DOWN
#define APDS9960_GESTURE_DOWN 0x02
#endif
#ifndef APDS9960_GESTURE_LEFT
#define APDS9960_GESTURE_LEFT 0x03
#endif
#ifndef APDS9960_GESTURE_RIGHT
#define APDS9960_GESTURE_RIGHT 0x04
#endif
#ifndef APDS9960_GESTURE_NEAR
#define APDS9960_GESTURE_NEAR 0x05
#endif
#ifndef APDS9960_GESTURE_FAR
#define APDS9960_GESTURE_FAR 0x06
#endif


// REPLACE WITH THE MAC ADDRESS OF YOUR RECEIVER ESP32
// You can find the receiver's MAC address by running a sketch that prints it (e.g., WiFi.macAddress())
uint8_t broadcastAddress[] = {0x00, 0x4b, 0x12, 0x34, 0x8c, 0x00}; // <<< IMPORTANT: Change this to your receiver's MAC

// Create an instance of the APDS9960 sensor
Adafruit_APDS9960 apds;

// Define a structure to send data
// This must match the receiver's structure exactly
typedef struct struct_message {
  int gesture;
} struct_message;

// Create a variable of the structure
struct_message myData;

// Create a peer info structure for ESP-NOW
esp_now_peer_info_t peerInfo;

// Callback function that will be executed when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  // Initialize Serial Monitor for debugging output
  Serial.begin(115200);

  // Initialize the APDS-9960 sensor
  // Check if the sensor is connected and communicating properly
  if (!apds.begin()) {
    Serial.println("Failed to initialize APDS-9960! Please check your wiring.");
    while (1); // Halt execution if sensor initialization fails
  }
  Serial.println("APDS-9960 Initialized successfully.");
  
  // Enable gesture sensing mode on the APDS-9960
  apds.enableGesture(true);
  Serial.println("Gesture sensing enabled.");

  // Set the ESP32 device as a Wi-Fi Station
  // This mode allows it to send and receive ESP-NOW packets
  WiFi.mode(WIFI_STA);
  Serial.println("WiFi in Station mode.");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW. Retrying...");
    // You might want to add a retry mechanism or delay here in a real application
    return; // Exit setup if ESP-NOW fails to initialize
  }
  Serial.println("ESP-NOW initialized.");

  // Register the callback function that gets called after data is sent
  esp_now_register_send_cb(OnDataSent);
  Serial.println("ESP-NOW send callback registered.");

  // Register peer (the receiver ESP32)
  // Copy the broadcastAddress (receiver's MAC) into the peerInfo struct
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0; // ESP-NOW uses channel 0 by default
  peerInfo.encrypt = false; // Set to true if you are using encryption

  // Add peer to the ESP-NOW peer list
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer. Ensure MAC address is correct and peer is reachable.");
    return; // Exit setup if adding peer fails
  }
  Serial.println("Peer added successfully.");
}

void loop() {
  // Read the gesture from the APDS-9960 sensor
  uint8_t gesture = apds.readGesture();

  // Check if a valid gesture was detected (not APDS9960_GESTURE_NONE)
  if (gesture != APDS9960_GESTURE_NONE) {
    // Store the detected gesture in our data structure
    myData.gesture = gesture; 

    // Print the detected gesture to the Serial Monitor for debugging
    Serial.print("Detected Gesture: ");
    switch (gesture) {
      case APDS9960_GESTURE_UP: Serial.println("UP"); break;
      case APDS9960_GESTURE_DOWN: Serial.println("DOWN"); break;
      case APDS9960_GESTURE_LEFT: Serial.println("LEFT"); break;
      case APDS9960_GESTURE_RIGHT: Serial.println("RIGHT"); break;
      case APDS9960_GESTURE_NEAR: Serial.println("NEAR"); break;
      case APDS9960_GESTURE_FAR: Serial.println("FAR"); break;
      default: Serial.println("UNKNOWN"); break;
    }

    // Send the gesture data via ESP-NOW to the registered peer
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    // Check the result of the ESP-NOW send operation
    if (result == ESP_OK) {
      Serial.println("Gesture data sent successfully via ESP-NOW.");
    } else {
      Serial.println("Error sending gesture data via ESP-NOW.");
    }
    
    // Add a small delay to prevent overwhelming the receiver with too many messages
    // and to give time for the gesture sensor to settle.
    delay(1000); 
  }
}