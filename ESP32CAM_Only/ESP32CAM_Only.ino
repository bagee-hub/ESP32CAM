/*
 * ESP32-CAM Only
 * - Provides camera frames via HTTP on request
 * - No sensor integration
 * 
 * Endpoints:
 * - GET / or GET /capture - Single camera frame
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// ===== WiFi Configuration =====
const char* ssid = "HD";        // Replace with your WiFi network name
const char* password = "12345678"; // Replace with your WiFi password

// ===== UDP Configuration =====
WiFiUDP udp;
IPAddress broadcastIP(255, 255, 255, 255);  // Broadcast to all devices on network
const int udpPort = 12346;  // Different port than sensor (12345)
unsigned long lastCaptureRequestTime = 0;
unsigned long lastUdpSendTime = 0;
const unsigned long captureTimeout = 5000;  // 5 seconds
const unsigned long udpSendInterval = 2000; // 2 seconds

// ===== Camera Configuration (AI-Thinker ESP32-CAM) =====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===== HTTP Server =====
WiFiServer server(80);

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000); // Wait for serial to stabilize
  Serial.setDebugOutput(true);
  Serial.println("\n\n========================================");
  Serial.println("ESP32-CAM Starting...");
  Serial.println("========================================");

  // Initialize camera
  Serial.println("Initializing camera...");
  if (!initCamera()) {
    Serial.println("ERROR: Camera initialization failed!");
    Serial.println("Check: 1) Camera connected 2) Power supply 3) GPIO pins");
    delay(5000); // Wait before restart so you can see the error
    ESP.restart();
  }
  Serial.println("SUCCESS: Camera initialized");

  // Connect to WiFi
  connectToWiFi();

  // Start HTTP server for camera frames
  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Camera URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  // Handle HTTP requests for camera frames
  handleCameraRequests();
  
  // Send UDP IP broadcast if no capture requests for 5 seconds
  unsigned long timeSinceLastCapture = millis() - lastCaptureRequestTime;
  if (timeSinceLastCapture >= captureTimeout) {
    if (millis() - lastUdpSendTime >= udpSendInterval) {
      sendIPviaUDP();
      lastUdpSendTime = millis();
    }
  }
}

// ===== Camera Initialization =====
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Init with high specs for quality
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;  // 1600x1200
    config.jpeg_quality = 10;             // 0-63 lower means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;   // 800x600
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  // Adjust sensor settings
  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_framesize(s, FRAMESIZE_VGA); // 640x480 for better performance
  }

  return true;
}

// ===== WiFi Connection =====
void connectToWiFi() {
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
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
    ESP.restart();
  }
}

// ===== Handle Camera HTTP Requests =====
void handleCameraRequests() {
  WiFiClient client = server.available();
  
  if (client) {
    String currentLine = "";
    String request = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Parse request
            if (request.indexOf("GET / ") >= 0 || request.indexOf("GET /capture") >= 0) {
              // Capture and send single frame
              lastCaptureRequestTime = millis(); // Update last capture time
              sendCameraFrame(client);
            } else {
              // 404 Not Found
              client.println("HTTP/1.1 404 Not Found");
              client.println("Content-Type: text/html");
              client.println("Connection: close");
              client.println();
              client.println("<!DOCTYPE HTML><html><body><h1>404 Not Found</h1></body></html>");
            }
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    
    delay(1);
    client.stop();
  }
}

// ===== Send Single Camera Frame =====
void sendCameraFrame(WiFiClient &client) {
  camera_fb_t * fb = esp_camera_fb_get();
  
  if (!fb) {
    client.println("HTTP/1.1 500 Internal Server Error");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Camera capture failed");
    return;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.println("Content-Disposition: inline; filename=capture.jpg");
  client.printf("Content-Length: %d\r\n", fb->len);
  client.println("Connection: close");
  client.println();
  
  client.write(fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
}

// ===== Send IP via UDP =====
void sendIPviaUDP() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  char udpBuffer[128];
  snprintf(udpBuffer, sizeof(udpBuffer),
           "{\"device\":\"ESP32-CAM\",\"ip\":\"%s\"}",
           WiFi.localIP().toString().c_str());
  
  udp.beginPacket(broadcastIP, udpPort);
  udp.write((uint8_t*)udpBuffer, strlen(udpBuffer));
  int result = udp.endPacket();
  
  if (result == 1) {
    Serial.printf("[UDP] Sent IP: %s\n", udpBuffer);
  } else {
    Serial.printf("[UDP] Send failed (code: %d)\n", result);
  }
}
