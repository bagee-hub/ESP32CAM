# ESP32 UDP Broadcast System

This system allows the ESP32 to broadcast sensor data to all devices on the network without hardcoding IP addresses.

## Architecture

```
ESP32 → UDP Broadcast (255.255.255.255:12345) → UDP Bridge Server → WebSocket → Web Browser
```

## Setup Instructions

### 1. ESP32 Setup
The ESP32 code (`Rd03e.ino`) is already configured to broadcast UDP packets to `255.255.255.255:12345`.

Upload the code to your ESP32 and it will automatically start broadcasting distance data every second.

### 2. Install Node.js Dependencies
```bash
cd /Users/b0n00pr/au/ai/ESP32CAM
npm install ws
```

### 3. Start the UDP Bridge Server
```bash
node udp_bridge.js
```

You should see:
```
🎯 UDP Bridge Server Started
📡 Listening for UDP broadcasts on port 12345
🌐 WebSocket server running on ws://localhost:8080
Waiting for ESP32 broadcasts...
```

### 4. Open the Web Interface
Open `MobileApp_Example.html` in your web browser and click "🎯 Listen for Broadcasts"

## How It Works

### ESP32 Side
- Broadcasts UDP packets to `255.255.255.255:12345`
- All devices on the network receive these packets
- No need to configure target IP addresses

### Bridge Server (udp_bridge.js)
- Listens for UDP broadcasts on port 12345
- Forwards received data to connected web clients via WebSocket
- Runs on port 8080 by default

### Web Interface (MobileApp_Example.html)
- Connects to the bridge server via WebSocket
- Displays real-time distance data
- Shows ESP32 IP address, packet count, and timestamps
- Auto-reconnects if connection is lost

## Features

✅ **No Hardcoded IPs** - ESP32 broadcasts to entire network
✅ **Multiple Listeners** - Any device can receive the broadcasts
✅ **Real-time Updates** - Data updates every second
✅ **Auto-reconnect** - WebSocket reconnects automatically
✅ **Packet Counter** - Track number of received packets
✅ **ESP32 IP Detection** - Automatically shows which ESP32 is broadcasting

## Troubleshooting

### ESP32 not broadcasting?
- Check WiFi connection (ESP32 should print its IP on Serial Monitor)
- Verify ESP32 is on the same network
- Check firewall settings

### Bridge server not receiving?
- Ensure UDP port 12345 is not blocked by firewall
- Try running: `sudo node udp_bridge.js` (may need elevated permissions)
- Check if another application is using port 12345

### Web interface not connecting?
- Make sure bridge server is running first
- Check WebSocket URL is correct (default: `ws://localhost:8080`)
- Open browser console (F12) to see connection errors

## Ports Used

- **UDP 12345**: ESP32 broadcasts sensor data
- **WebSocket 8080**: Bridge server communicates with web clients

## Network Requirements

- ESP32 and computer must be on the same local network
- Router must allow UDP broadcasts (most do by default)
- Firewall must allow incoming UDP on port 12345
