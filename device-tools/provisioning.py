# device-tools/provision.py - Device Provisioning Tool
#!/usr/bin/env python3
"""
Ice Pulse Sensor Provisioning Tool
Configures WiFi and device settings via serial connection
"""

import serial
import json
import time
import argparse
from typing import Dict, Any

class IcePulseProvisioner:
    def __init__(self, port: str, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        
    def connect(self) -> bool:
        """Connect to device via serial"""
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=2)
            time.sleep(2)  # Wait for connection
            print(f"✅ Connected to {self.port}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from device"""
        if self.serial:
            self.serial.close()
            print("🔌 Disconnected")
    
    def send_command(self, command: str) -> str:
        """Send command to device and get response"""
        if not self.serial:
            raise Exception("Not connected to device")
        
        self.serial.write(f"{command}\n".encode())
        time.sleep(0.5)
        
        response = ""
        while self.serial.in_waiting:
            response += self.serial.read(self.serial.in_waiting).decode()
            time.sleep(0.1)
        
        return response.strip()
    
    def configure_wifi(self, ssid: str, password: str) -> bool:
        """Configure WiFi credentials"""
        print(f"📶 Configuring WiFi: {ssid}")
        
        try:
            # Send WiFi configuration command
            config_cmd = f"wifi_config {ssid} {password}"
            response = self.send_command(config_cmd)
            
            if "OK" in response:
                print("✅ WiFi configured successfully")
                return True
            else:
                print(f"❌ WiFi configuration failed: {response}")
                return False
                
        except Exception as e:
            print(f"❌ Error configuring WiFi: {e}")
            return False
    
    def configure_device(self, config: Dict[str, Any]) -> bool:
        """Configure device settings"""
        print("⚙️ Configuring device settings")
        
        try:
            config_json = json.dumps(config)
            config_cmd = f"device_config {config_json}"
            response = self.send_command(config_cmd)
            
            if "OK" in response:
                print("✅ Device configured successfully")
                return True
            else:
                print(f"❌ Device configuration failed: {response}")
                return False
                
        except Exception as e:
            print(f"❌ Error configuring device: {e}")
            return False
    
    def get_device_info(self) -> Dict[str, Any]:
        """Get device information"""
        try:
            response = self.send_command("device_info")
            # Parse device info from response
            info = {
                "firmware_version": "Unknown",
                "device_id": "Unknown",
                "chip_id": "Unknown",
                "free_heap": "Unknown"
            }
            
            # Parse response (device should return JSON)
            lines = response.split('\n')
            for line in lines:
                if "firmware_version" in line:
                    try:
                        info.update(json.loads(line))
                        break
                    except:
                        pass
            
            return info
            
        except Exception as e:
            print(f"❌ Error getting device info: {e}")
            return {}
    
    def test_connection(self) -> bool:
        """Test device connectivity"""
        print("🔍 Testing device connection")
        
        try:
            response = self.send_command("ping")
            if "pong" in response.lower():
                print("✅ Device responding")
                return True
            else:
                print("❌ Device not responding")
                return False
                
        except Exception as e:
            print(f"❌ Connection test failed: {e}")
            return False

def main():
    parser = argparse.ArgumentParser(description='Ice Pulse Sensor Provisioning Tool')
    parser.add_argument('--port', '-p', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baudrate', '-b', type=int, default=115200, help='Baud rate')
    parser.add_argument('--wifi-ssid', help='WiFi SSID')
    parser.add_argument('--wifi-password', help='WiFi password')
    parser.add_argument('--device-id', help='Device ID')
    parser.add_argument('--api-endpoint', help='API endpoint URL')
    parser.add_argument('--config-file', help='JSON configuration file')
    
    args = parser.parse_args()
    
    # Create provisioner
    provisioner = IcePulseProvisioner(args.port, args.baudrate)
    
    if not provisioner.connect():
        return 1
    
    try:
        # Test connection
        if not provisioner.test_connection():
            print("❌ Device not responding to commands")
            return 1
        
        # Get device info
        info = provisioner.get_device_info()
        if info:
            print(f"📱 Device Info:")
            for key, value in info.items():
                print(f"  {key}: {value}")
        
        # Configure WiFi if provided
        if args.wifi_ssid and args.wifi_password:
            if not provisioner.configure_wifi(args.wifi_ssid, args.wifi_password):
                return 1
        
        # Configure device settings
        device_config = {}
        
        if args.device_id:
            device_config['device_id'] = args.device_id
        
        if args.api_endpoint:
            device_config['api_endpoint'] = args.api_endpoint
        
        # Load config from file if provided
        if args.config_file:
            try:
                with open(args.config_file, 'r') as f:
                    file_config = json.load(f)
                    device_config.update(file_config)
            except Exception as e:
                print(f"❌ Error loading config file: {e}")
                return 1
        
        # Apply device configuration
        if device_config:
            if not provisioner.configure_device(device_config):
                return 1
        
        print("🎉 Provisioning completed successfully!")
        return 0
        
    finally:
        provisioner.disconnect()

if __name__ == "__main__":
    exit(main())
