# scripts/build.sh - Build Script
#!/bin/bash
# Ice Pulse Sensor Build Script

set -e

ENVIRONMENT=${1:-"dev"}
CLEAN=${2:-"false"}

echo "🔨 Building Ice Pulse Sensor Firmware"
echo "Environment: $ENVIRONMENT"
echo "======================================"

cd firmware

# Clean if requested
if [ "$CLEAN" = "true" ]; then
    echo "🧹 Cleaning build directory..."
    idf.py fullclean
fi

# Set environment-specific configurations
case $ENVIRONMENT in
    "dev")
        echo "📱 Building for DEVELOPMENT"
        export CONFIG_ENV=development
        ;;
    "staging")
        echo "🧪 Building for STAGING"
        export CONFIG_ENV=staging
        ;;
    "prod")
        echo "🏭 Building for PRODUCTION"
        export CONFIG_ENV=production
        ;;
    *)
        echo "❌ Unknown environment: $ENVIRONMENT"
        exit 1
        ;;
esac

# Build firmware
echo "🔨 Building firmware..."
idf.py build

# Show build results
echo ""
echo "✅ Build completed successfully!"
echo "📊 Build Statistics:"
echo "  Firmware size: $(stat -f%z build/ice-pulse-sensor.bin 2>/dev/null || stat -c%s build/ice-pulse-sensor.bin) bytes"
echo "  Bootloader size: $(stat -f%z build/bootloader/bootloader.bin 2>/dev/null || stat -c%s build/bootloader/bootloader.bin) bytes"
echo "  Environment: $ENVIRONMENT"
echo "  Build directory: $(pwd)/build"