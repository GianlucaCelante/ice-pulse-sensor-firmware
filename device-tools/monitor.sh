# device-tools/monitor.sh - Serial Monitor Script
#!/bin/bash
# Ice Pulse Sensor Serial Monitor

DEVICE_PORT=${1:-"/dev/ttyUSB0"}
BAUD_RATE=115200

echo "🔍 Starting Ice Pulse Sensor Monitor"
echo "Device: $DEVICE_PORT"
echo "Baud Rate: $BAUD_RATE"
echo "Press Ctrl+] to exit"
echo "=================================="

cd ../firmware
idf.py -p "$DEVICE_PORT" monitor
