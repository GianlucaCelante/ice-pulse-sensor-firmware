# README.md - ice-pulse-sensor-firmware
# 🔌 Ice Pulse Sensor Firmware

ESP32-based temperature and humidity sensor for HACCP compliance monitoring.

## Features

- 🌡️ **Temperature Monitoring** (DS18B20, DHT22)
- 💧 **Humidity Monitoring** (DHT22, SHT30)
- 📶 **WiFi Connectivity** with auto-reconnect
- 🔄 **OTA Updates** over-the-air
- 💾 **Local Data Storage** (NVS, SPIFFS)
- ⏰ **Real-time Clock** with NTP sync
- 🚨 **Alert System** with thresholds
- 🔋 **Power Management** with deep sleep
- 📊 **Data Transmission** to API server

## Hardware Requirements

- **ESP32** development board
- **DHT22** or **SHT30** humidity/temperature sensor
- **DS18B20** temperature sensor (optional)
- **LED** for status indication
- **Power supply** 3.3V-5V

## Quick Start

### 1. Setup Development Environment

```bash
# Install ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh all
. ./export.sh

# Clone firmware repository
git clone https://github.com/YourOrg/ice-pulse-sensor-firmware.git
cd ice-pulse-sensor-firmware
```

### 2. Configure and Build

```bash
# Configure project
cd firmware
idf.py menuconfig

# Build firmware
idf.py build
```

### 3. Flash Device

```bash
# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Or use convenience script
../device-tools/flash.sh /dev/ttyUSB0
```

## Configuration

### WiFi Setup
Edit `firmware/main/config.h` or use WiFi provisioning:

```c
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"
```

### API Endpoint
```c
#define API_ENDPOINT "http://your-api.com/api/v1/sensors/data"
```

### OTA Server
```c
#define OTA_SERVER_URL "http://your-ota-server.com:8092"
```

## Development

### File Structure
```
firmware/
├── main/           # Main application
├── components/     # Custom components
└── build/          # Build artifacts

device-tools/       # Development tools
ota-server/         # OTA update server
config/             # Environment configs
```

### Building for Different Environments

```bash
# Development build
idf.py build

# Staging build
idf.py -DCONFIG_ENV=staging build

# Production build
idf.py -DCONFIG_ENV=production build
```

### Testing

```bash
# Run unit tests
cd tests
idf.py build flash monitor

# Hardware-in-loop tests
python ../device-tools/test_connection.py
```

## OTA Updates

### Manual Update
```bash
python device-tools/firmware_updater.py --device 192.168.1.100 --firmware build/ice-pulse-sensor.bin
```

### Automatic Updates
Devices check for updates every hour automatically when connected to WiFi.

### OTA Server Dashboard
Access the OTA management dashboard at: `http://localhost:8092`

## Deployment

### CI/CD Pipeline
GitHub Actions automatically:
1. **Build** firmware for target environment
2. **Test** with hardware simulation
3. **Package** OTA server Docker image
4. **Deploy** to infrastructure
5. **Update** device fleet

### Production Deployment
```bash
# Build production firmware
idf.py -DCONFIG_ENV=production build

# Deploy OTA server
docker-compose -f devops/prod/docker-compose-ota-prod.yml up -d

# Flash initial devices
./scripts/production_flash.sh /dev/ttyUSB0
```

## Monitoring

### Device Logs
```bash
# Real-time monitoring
idf.py monitor

# Or use monitoring script
./device-tools/monitor.sh /dev/ttyUSB0
```

### Health Checks
- **LED Status**: Green=OK, Red=Error, Blue=OTA
- **Serial Output**: Detailed logging available
- **API Heartbeat**: Every 10 minutes
- **OTA Check**: Every hour

## Troubleshooting

### Common Issues

**Device not connecting to WiFi:**
```bash
# Check WiFi credentials
idf.py monitor
# Look for WiFi connection logs
```

**OTA update fails:**
```bash
# Check OTA server accessibility
curl http://your-ota-server:8092/health

# Verify firmware compatibility
curl http://your-ota-server:8092/firmware/manifest
```

**Sensor readings incorrect:**
```bash
# Check sensor wiring
# Verify power supply (3.3V/5V)
# Test sensors individually
```

### Debug Mode
Enable debug logging in `config.h`:
```c
#define DEBUG_LEVEL ESP_LOG_DEBUG
#define ENABLE_SERIAL_OUTPUT true
```

## API Protocol

### Data Format
```json
{
  "device_id": "ice-pulse-001",
  "firmware_version": "0.1.0",
  "timestamp": 1706694000,
  "temperature": -18.5,
  "humidity": 65.0,
  "reading_count": 1234,
  "uptime": 86400
}
```

### Endpoints
- `POST /api/v1/sensors/data` - Send sensor data
- `GET /api/v1/sensors/{id}/config` - Get device config
- `POST /api/v1/sensors/{id}/command` - Send commands

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/new-sensor`
3. Commit changes: `git commit -am 'Add new sensor support'`
4. Push branch: `git push origin feature/new-sensor`
5. Submit pull request

## License

MIT License - see LICENSE file for details.
