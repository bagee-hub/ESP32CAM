#!/usr/bin/env python3
"""
Dual UDP Listener for ESP32 devices
- Port 12345: RD-03E Sensor data (distance + IP)
- Port 12346: ESP32-CAM IP announcements
"""

import socket
import json
import threading
from datetime import datetime

def listen_on_port(port, device_name):
    """Listen for UDP packets on specified port"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', port))
    
    print(f"[{device_name}] Listening on port {port}...")
    
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            
            try:
                json_data = json.loads(data.decode('utf-8'))
                print(f"\n[{timestamp}] [{device_name}] From {addr[0]}:{addr[1]}")
                print(f"  Data: {json.dumps(json_data, indent=2)}")
            except json.JSONDecodeError:
                print(f"\n[{timestamp}] [{device_name}] Raw data from {addr[0]}:{addr[1]}")
                print(f"  {data.decode('utf-8', errors='ignore')}")
                
        except Exception as e:
            print(f"[{device_name}] Error: {e}")

def main():
    print("=" * 60)
    print("Dual UDP Listener for ESP32 Devices")
    print("=" * 60)
    print("Port 12345: RD-03E Sensor (distance + IP)")
    print("Port 12346: ESP32-CAM (IP announcements)")
    print("=" * 60)
    print("\nPress Ctrl+C to stop\n")
    
    # Create threads for both ports
    sensor_thread = threading.Thread(target=listen_on_port, args=(12345, "SENSOR"), daemon=True)
    camera_thread = threading.Thread(target=listen_on_port, args=(12346, "CAMERA"), daemon=True)
    
    # Start both listeners
    sensor_thread.start()
    camera_thread.start()
    
    try:
        # Keep main thread alive
        sensor_thread.join()
        camera_thread.join()
    except KeyboardInterrupt:
        print("\n\nStopping UDP listeners...")
        print("Goodbye!")

if __name__ == "__main__":
    main()
