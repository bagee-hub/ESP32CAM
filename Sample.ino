// Define the onboard LED pin
int onboard_led = 5;

// Radar data buffer
uint8_t RX_BUF[64] = {0};  
uint8_t RX_count = 0;      
uint8_t RX_temp;           
uint16_t range;

// Timing variables
unsigned long lastDetectionTime = 0;
unsigned long detectionStartTime = 0; // Tracks when target is first detected
const unsigned long timerDelay = 5000; // 5 seconds delay
const unsigned long confirmDelay = 3000; // 3 seconds delay
bool targetInRange = false;
bool ledActivated = false;

void setup() {
  // Initialize LED pin as output
  pinMode(onboard_led, OUTPUT);
  digitalWrite(onboard_led, HIGH);
  // Initialize serial communications
  Serial.begin(115200);
  
  // Start Serial1 on pins 16 (RX) and 17 (TX) at 256000 baud rate
  Serial1.begin(256000, SERIAL_8N1, 16, 17);
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

        // Output distance in meters and gesture for debugging
        Serial.printf("Distance: %.2f m, Gesture: 0x%02X\n", distanceInMeters, gesture);
        if (gesture == 0x01 )
        {
          Serial.println("Gesture detected");
          delay(2000);
        }
        // Check if target is within the specified range
        if (distanceInMeters >= .7 && distanceInMeters <= 3) {
          if (!targetInRange) {
            detectionStartTime = millis(); // Start the 3-second confirmation timer
          }
          targetInRange = true; // Set target detected to true
        } else {
          targetInRange = false; // Target out of range
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

  // Control LED based on detection confirmation and timer
  if (targetInRange && (millis() - detectionStartTime >= confirmDelay)) {
    digitalWrite(onboard_led, LOW); // Turn on LED if target is confirmed for 3 second
    lastDetectionTime = millis(); // Reset the 5-second timer
    ledActivated = true;
  } else if (millis() - lastDetectionTime >= timerDelay && ledActivated) {
    digitalWrite(onboard_led, HIGH); // Turn off LED if target is out of range for 5 seconds
    ledActivated = false;
  }

  // Optional: Add a small delay to avoid flooding the Serial Monitor
  //delay(100);
}