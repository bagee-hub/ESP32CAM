#include <WiFi.h>
#include <WiFiUdp.h>

// ===== WiFi Configuration =====
const char* ssid = "HD";
const char* password = "12345678";

// ===== UDP Configuration =====
WiFiUDP udp;
IPAddress broadcastIP(255, 255, 255, 255);  // Broadcast to all devices on network
const int udpPort = 12345;
unsigned long lastUdpSendTime = 0;
const unsigned long udpSendInterval = 1000; // Send every 1 second

// Define the onboard LED pin
int onboard_led = 2;
 
// Radar data buffer
uint8_t RX_BUF[64] = {0};  
uint8_t RX_count = 0;      
uint8_t RX_temp;           
uint16_t range;
float currentDistance = 0.0; // Track current distance
 
// Timing variables - removed delays for instant response
bool targetInRange = false;
 
void setup() {
  // Initialize LED pin as output
  pinMode(onboard_led, OUTPUT);
  digitalWrite(onboard_led, HIGH);
  
  // Initialize serial communications
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP32 with RD-03E Sensor Starting...");
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Broadcasting UDP to: ");
    Serial.print(broadcastIP);
    Serial.print(":");
    Serial.println(udpPort);
  } else {
    Serial.println("\nWiFi connection failed! Restarting...");
    ESP.restart();
  }
  
  // Start Serial1 on pins 16 (RX) and 17 (TX) at 256000 baud rate
  Serial1.begin(256000, SERIAL_8N1, 16, 17);
  Serial.println("RD-03E sensor initialized\n");
}
 
void loop() {
  // Check for data from Serial1
  while (Serial1.available()) {
    RX_temp = Serial1.read(); // Read a byte from Serial1
    RX_BUF[RX_count++] = RX_temp; // Store it in the buffer
 
    // Check if we have a valid packet (minimum 5 bytes needed)
    if (RX_count >= 5) { // Ensure we have enough bytes to check for a complete frame
      // Check for valid frame: AA + distance (2 bytes) + gesture (1 byte) + 55
      if (RX_BUF[0] == 0xAA && RX_BUF[RX_count - 1] == 0x55) {
        // Extract distance and gesture
        range = (RX_BUF[2] << 8) | RX_BUF[1]; // Combine distance bytes in little-end format
        uint8_t gesture = RX_BUF[3]; // Gesture information (1 byte)
 
        // Convert distance to meters
        float distanceInMeters = range / 100.0; // Convert cm to meters
        currentDistance = distanceInMeters; // Update current distance
 
        // Output distance in meters and gesture for debugging
        Serial.printf("Distance: %.2f m, Gesture: 0x%02X\n", distanceInMeters, gesture);
        if (gesture == 0x01 )
        {
          Serial.println("Gesture detected");
          delay(2000);
        }
        // Check if target distance is greater than 2 meters
        if (distanceInMeters > 2.0) {
          targetInRange = true; // Human detected beyond 2m
        } else {
          targetInRange = false; // Human at 2m or closer
        }
 
        // Reset buffer and count after processing a packet
        memset(RX_BUF, 0x00, sizeof(RX_BUF));
        RX_count = 0;
      } else {
        // If we receive an unexpected byte, we can reset the buffer
        if (RX_count >= sizeof(RX_BUF)) {
          memset(RX_BUF, 0x00, sizeof(RX_BUF));
          RX_count = 0;
        }
      }
    }
  }
 
  // Control LED instantly based on distance > 2m
  if (targetInRange) {
    digitalWrite(onboard_led, HIGH); // Turn on LED - human detected beyond 2m
  } else {
    digitalWrite(onboard_led, LOW); // Turn off LED - no human or human closer than 2m
  }
  
  // Send UDP data periodically
  if (millis() - lastUdpSendTime >= udpSendInterval) {
    sendUDPData();
    lastUdpSendTime = millis();
  }
 
  // Optional: Add a small delay to avoid flooding the Serial Monitor
  delay(100);
}

// ===== Send UDP Data =====
void sendUDPData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[UDP] WiFi not connected!");
    return;
  }
  
  char udpBuffer[128];
  snprintf(udpBuffer, sizeof(udpBuffer),
           "{\"distance\":%.2f}",
           currentDistance);
  
  udp.beginPacket(broadcastIP, udpPort);
  udp.write((uint8_t*)udpBuffer, strlen(udpBuffer));
  int result = udp.endPacket();
  
  if (result == 1) {
    Serial.printf("[UDP] Sent: %s\n", udpBuffer);
  } else {
    Serial.printf("[UDP] Send failed (code: %d)\n", result);
  }
}