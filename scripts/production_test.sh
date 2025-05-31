#!/bin/bash
# scripts/production_test.sh

echo "🏭 Production Testing Sequence"
echo "=============================="

DEVICE_PORT=$1
DEVICE_ID=$2

if [ -z "$DEVICE_PORT" ] || [ -z "$DEVICE_ID" ]; then
    echo "Usage: $0 <port> <device_id>"
    exit 1
fi

# 1. Flash firmware
echo "📀 Flashing production firmware..."
./device-tools/flash.sh $DEVICE_PORT

# 2. Hardware validation
echo "🔧 Hardware validation..."
python3 device-tools/hardware_test.py $DEVICE_PORT

# 3. Calibration
echo "📐 Sensor calibration..."
python3 device-tools/calibration_test.py $DEVICE_PORT

# 4. Provisioning
echo "⚙️ Device provisioning..."
python3 device-tools/provision.py --port $DEVICE_PORT --device-id $DEVICE_ID --config-file config/prod/device_config.json

# 5. Final validation
echo "✅ Final validation..."
python3 device-tools/environmental_test.py $DEVICE_PORT 5  # 5 minute test

echo "🎉 Production testing completed for device: $DEVICE_ID"