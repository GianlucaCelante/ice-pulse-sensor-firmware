# device-tools/flash.sh - Automated Flashing Script
#!/bin/bash
# Flash Ice Pulse Sensor Firmware

set -e

# Configuration
FIRMWARE_DIR="../firmware"
BUILD_DIR="${FIRMWARE_DIR}/build"
DEVICE_PORT=${1:-"/dev/ttyUSB0"}
BAUD_RATE=921600

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🔌 Ice Pulse Sensor Flashing Tool${NC}"
echo "=================================="

# Check if device port is provided
if [ $# -eq 0 ]; then
    echo -e "${YELLOW}No device port specified, using default: ${DEVICE_PORT}${NC}"
fi

# Check if port exists
if [ ! -e "$DEVICE_PORT" ]; then
    echo -e "${RED}Error: Device port ${DEVICE_PORT} not found${NC}"
    echo "Available ports:"
    ls /dev/tty* 2>/dev/null | grep -E "(USB|ACM)" || echo "No USB devices found"
    exit 1
fi

# Check if firmware exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Firmware not built. Run 'idf.py build' first${NC}"
    exit 1
fi

if [ ! -f "$BUILD_DIR/ice-pulse-sensor.bin" ]; then
    echo -e "${RED}Error: Firmware binary not found${NC}"
    exit 1
fi

# Get firmware info
FIRMWARE_SIZE=$(stat -f%z "$BUILD_DIR/ice-pulse-sensor.bin" 2>/dev/null || stat -c%s "$BUILD_DIR/ice-pulse-sensor.bin")
echo -e "${GREEN}Firmware size: ${FIRMWARE_SIZE} bytes${NC}"

# Check ESP-IDF environment
if [ -z "$IDF_PATH" ]; then
    echo -e "${RED}Error: ESP-IDF environment not set. Run '. \$IDF_PATH/export.sh'${NC}"
    exit 1
fi

echo -e "${YELLOW}Flashing to device: ${DEVICE_PORT}${NC}"
echo -e "${YELLOW}Baud rate: ${BAUD_RATE}${NC}"

# Flash firmware
cd "$FIRMWARE_DIR"

echo -e "${BLUE}Starting flash process...${NC}"
idf.py -p "$DEVICE_PORT" -b "$BAUD_RATE" flash

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Firmware flashed successfully!${NC}"
    echo ""
    echo -e "${BLUE}Starting monitor (Ctrl+] to exit)...${NC}"
    sleep 2
    idf.py -p "$DEVICE_PORT" monitor
else
    echo -e "${RED}❌ Flash failed!${NC}"
    exit 1
fi
