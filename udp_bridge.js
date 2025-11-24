#!/usr/bin/env node

/**
 * UDP Broadcast Bridge Server
 * Receives UDP broadcasts from ESP32 and forwards them to web clients via WebSocket
 */

const dgram = require('dgram');
const WebSocket = require('ws');

const UDP_PORT = 12345;
const WS_PORT = 8080;

// Create UDP socket
const udpSocket = dgram.createSocket('udp4');

// Create WebSocket server
const wss = new WebSocket.Server({ port: WS_PORT });

let connectedClients = new Set();
let latestData = null;

// WebSocket server handlers
wss.on('connection', (ws) => {
    console.log('✅ New WebSocket client connected');
    connectedClients.add(ws);
    
    // Send latest data immediately if available
    if (latestData) {
        ws.send(JSON.stringify(latestData));
    }
    
    ws.on('close', () => {
        console.log('❌ WebSocket client disconnected');
        connectedClients.delete(ws);
    });
    
    ws.on('error', (error) => {
        console.error('WebSocket error:', error);
        connectedClients.delete(ws);
    });
});

// UDP socket handlers
udpSocket.on('error', (err) => {
    console.error('UDP socket error:', err);
    udpSocket.close();
});

udpSocket.on('message', (msg, rinfo) => {
    try {
        const data = JSON.parse(msg.toString());
        data.timestamp = new Date().toISOString();
        data.esp32_ip = rinfo.address;
        
        latestData = data;
        
        console.log(`📡 Received from ${rinfo.address}:${rinfo.port} - Distance: ${data.distance}m`);
        
        // Broadcast to all connected WebSocket clients
        const message = JSON.stringify(data);
        connectedClients.forEach((client) => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(message);
            }
        });
    } catch (error) {
        console.error('Error parsing UDP message:', error);
    }
});

udpSocket.on('listening', () => {
    const address = udpSocket.address();
    console.log('🎯 UDP Bridge Server Started');
    console.log(`📡 Listening for UDP broadcasts on port ${address.port}`);
    console.log(`🌐 WebSocket server running on ws://localhost:${WS_PORT}`);
    console.log('Waiting for ESP32 broadcasts...\n');
});

// Bind UDP socket to listen for broadcasts
udpSocket.bind(UDP_PORT, () => {
    udpSocket.setBroadcast(true);
});

// Handle graceful shutdown
process.on('SIGINT', () => {
    console.log('\n🛑 Shutting down...');
    udpSocket.close();
    wss.close();
    process.exit(0);
});
