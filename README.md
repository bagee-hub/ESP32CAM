# ESP32-CAM with RD-03E Radar Sensor

This project integrates an ESP32-CAM with an RD-03E radar sensor to:
- Read radar sensor data via UART
- Send sensor data to mobile app via UDP
- Provide camera frames via HTTP requests
- Stream live video feed

## Hardware Setup

### Pin Connections
```
RD-03E Sensor -> ESP32-CAM
├─ OT1 (TX)    -> GPIO3 (U0R/RX)
└─ RX          -> GPIO1 (U0T/TX)
```

### Power Requirements
- ESP32-CAM: 5V via USB or external power supply
- RD-03E: 5V (can share power with ESP32-CAM)
- Ensure common ground connection

## Software Setup

### 1. Arduino IDE Configuration

#### Install ESP32 Board Support
1. Open Arduino IDE
2. Go to **File > Preferences**
3. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools > Board > Boards Manager**
5. Search for "ESP32" and install "esp32 by Espressif Systems"

#### Install Required Libraries
1. Go to **Sketch > Include Library > Manage Libraries**
2. Install:
   - **ESP32** (should be installed with board support)

#### Board Selection
1. **Tools > Board** → "AI Thinker ESP32-CAM"
2. **Tools > Port** → Select your ESP32-CAM port
3. **Tools > Partition Scheme** → "Huge APP (3MB No OTA/1MB SPIFFS)"

### 2. Configure WiFi Credentials

Edit `ESP32CAM_RD03E_UDP.ino`:
```cpp
const char* ssid = "YOUR_HOTSPOT_SSID";        // Your mobile hotspot name
const char* password = "YOUR_HOTSPOT_PASSWORD"; // Your mobile hotspot password
```

### 3. Configure UDP Settings

```cpp
IPAddress mobileAppIP;                      // Mobile phone IP (gateway)
const int udpPort = 12345;                  // UDP port for sensor data
```

**How Mobile IP Detection Works:**
- When ESP32-CAM connects to your phone's hotspot, the phone acts as the **gateway**
- The code automatically gets the mobile phone's IP using `WiFi.gatewayIP()`
- UDP packets are sent directly to the gateway (your phone) instead of broadcasting
- This is more efficient and works regardless of the hotspot's subnet configuration

### 4. Upload Code

1. Connect ESP32-CAM to computer using FTDI programmer or USB-to-Serial adapter
2. Put ESP32-CAM in flash mode:
   - Connect GPIO0 to GND
   - Press RESET button
3. Click **Upload** in Arduino IDE
4. After upload completes:
   - Disconnect GPIO0 from GND
   - Press RESET button

## Mobile App Setup

### Option 1: HTML Web App (Recommended for Testing)

1. Open `MobileApp_Example.html` in your mobile browser
2. Connect your phone to the same hotspot as ESP32-CAM
3. Enter ESP32-CAM IP address (shown in Serial Monitor after boot)
4. Click "Connect to ESP32-CAM"

### Option 2: Native Mobile App (UDP Receiver)

For receiving UDP data in a native app, you'll need to:

#### Android (Java/Kotlin)
```java
// Add permission in AndroidManifest.xml
<uses-permission android:name="android.permission.INTERNET" />

// UDP Receiver code
DatagramSocket socket = new DatagramSocket(12345);
byte[] buffer = new byte[1024];
DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
socket.receive(packet);
String data = new String(packet.getData(), 0, packet.getLength());
```

#### iOS (Swift)
```swift
// Use Network framework or CocoaAsyncSocket library
import Network

let connection = NWConnection(
    host: "192.168.43.255",
    port: 12345,
    using: .udp
)
```

## API Endpoints

Once ESP32-CAM is connected to WiFi, it provides these HTTP endpoints:

### GET /
- Returns single camera frame (JPEG)
- **Example:** `http://192.168.43.100/`

### GET /capture
- Captures and returns single frame
- **Example:** `http://192.168.43.100/capture`

### GET /stream
- Returns MJPEG video stream
- **Example:** `http://192.168.43.100/stream`

### GET /sensor
- Returns current sensor data as JSON
- **Example:** `http://192.168.43.100/sensor`
- **Response:**
  ```json
  {
    "distance": 1.25,
    "gesture": "0x00",
    "range": 125,
    "ip": "192.168.43.100"
  }
  ```

## Mobile Phone IP Detection

The ESP32-CAM automatically detects your mobile phone's IP address using the **gateway IP method**:

```cpp
mobileAppIP = WiFi.gatewayIP();  // Gateway = Mobile Phone
```

**Why this works:**
- When your phone creates a hotspot, it becomes the network gateway
- All devices connected to the hotspot route traffic through the phone
- `WiFi.gatewayIP()` returns the phone's IP address
- No need for broadcast addresses or manual IP configuration

**Benefits:**
- ✅ Works with any hotspot subnet (Android, iPhone, etc.)
- ✅ Direct UDP communication (no broadcast overhead)
- ✅ Automatic detection on connection
- ✅ No manual IP configuration needed

## UDP Data Format

Sensor data is sent directly to the mobile phone via UDP in JSON format:
```json
{
  "distance": 1.25,
  "gesture": "0x01",
  "range": 125
}
```

- **distance**: Distance in meters (float)
- **gesture**: Gesture code in hex format (string)
- **range**: Raw distance in centimeters (integer)

## RD-03E Sensor Protocol

The RD-03E uses UART communication at 256000 baud:

### Data Frame Format
```
[0xAA] [Distance_L] [Distance_H] [Gesture] [0x55]
```

- **Header:** 0xAA
- **Distance_L:** Lower byte of distance (cm)
- **Distance_H:** Upper byte of distance (cm)
- **Gesture:** Gesture detection code
- **Footer:** 0x55

### Gesture Codes
- `0x00`: No gesture
- `0x01`: Gesture detected
- Other codes may vary by firmware version

## Troubleshooting

### ESP32-CAM won't upload
- Ensure GPIO0 is connected to GND during upload
- Check FTDI programmer connections (TX→RX, RX→TX)
- Try lower upload speed: **Tools > Upload Speed** → 115200

### Camera initialization failed
- Check camera ribbon cable connection
- Ensure proper power supply (5V, sufficient current)
- Try different partition scheme

### WiFi connection failed
- Verify SSID and password
- Ensure hotspot is 2.4GHz (ESP32 doesn't support 5GHz)
- Check signal strength

### No sensor data
- Verify UART connections (TX→RX, RX→TX)
- Check baud rate (256000)
- Ensure RD-03E is powered properly
- Test sensor with Sample.ino first

### Can't access camera from mobile
- Verify ESP32-CAM and phone are on same network
- Check firewall settings on phone
- Try accessing from browser: `http://[ESP32_IP]/`
- Ping ESP32-CAM IP from phone

### UDP data not received
- Verify UDP port (12345) is not blocked
- Check broadcast address matches your subnet
- Use a UDP testing app to verify reception
- Ensure phone allows UDP broadcasts

## Performance Notes

- **Camera Resolution:** VGA (640x480) for optimal performance
- **Frame Rate:** ~30 FPS for streaming
- **UDP Send Rate:** 100ms interval (10 Hz)
- **UART Baud Rate:** 256000 for RD-03E

## Serial Monitor Note

⚠️ **Important:** Once the code is uploaded, the default Serial (UART0) is used for RD-03E communication on GPIO3/GPIO1. This means you'll lose USB Serial Monitor access for debugging.

### Solutions:
1. **Use Serial2 for debugging** (if available pins)
2. **Comment out RD-03E serial initialization** during debugging
3. **Use WiFi-based logging** (send debug info via UDP or HTTP)

## Power Consumption

- ESP32-CAM idle: ~160mA
- ESP32-CAM with camera active: ~300-400mA
- RD-03E sensor: ~100mA
- **Total:** ~500mA peak

Use a quality 5V power supply with at least 1A capacity.

## Future Enhancements

- [ ] Add authentication for camera access
- [ ] Implement motion detection alerts
- [ ] Add SD card recording capability
- [ ] Create native Android/iOS apps
- [ ] Add OTA (Over-The-Air) firmware updates
- [ ] Implement MQTT for IoT integration
- [ ] Add battery power support with deep sleep

## License

This project is open source. Feel free to modify and distribute.

## Support

For issues or questions:
1. Check Serial Monitor output for error messages
2. Verify all connections and configurations
3. Test components individually (camera, sensor, WiFi)
4. Review ESP32-CAM and RD-03E datasheets
