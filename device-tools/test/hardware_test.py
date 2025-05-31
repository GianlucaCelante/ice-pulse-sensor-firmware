#!/usr/bin/env python3
# device-tools/hardware_test.py

import serial
import time
import json
import requests
from typing import Dict, List

class HardwareValidator:
    def __init__(self, port: str = "/dev/ttyUSB0"):
        self.port = port
        self.serial = None
        self.test_results = []
    
    def connect_device(self) -> bool:
        """Connect to ESP32 via serial"""
        try:
            self.serial = serial.Serial(self.port, 115200, timeout=5)
            time.sleep(2)
            print("✅ Connected to ESP32")
            return True
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            return False
    
    def test_power_on(self) -> bool:
        """Test if device powers on correctly"""
        print("🔋 Testing power on...")
        
        # Look for boot messages
        boot_msgs = []
        start_time = time.time()
        
        while time.time() - start_time < 10:
            if self.serial.in_waiting:
                line = self.serial.readline().decode().strip()
                boot_msgs.append(line)
                
                if "ice_pulse_main: Ice Pulse Sensor started" in line:
                    print("✅ Device boot successful")
                    return True
                    
        print("❌ Device boot failed")
        print("Boot messages:", boot_msgs[-5:])  # Last 5 messages
        return False
    
    def test_sensors(self) -> Dict[str, bool]:
        """Test sensor functionality"""
        print("🌡️ Testing sensors...")
        
        results = {
            "temperature": False,
            "humidity": False,
            "sensor_power": False
        }
        
        # Send sensor test command
        self.send_command("test_sensors")
        
        # Wait for sensor readings
        start_time = time.time()
        while time.time() - start_time < 30:
            if self.serial.in_waiting:
                line = self.serial.readline().decode().strip()
                
                if "Temperature:" in line and "°C" in line:
                    temp = float(line.split("Temperature:")[1].split("°C")[0])
                    if -50 < temp < 100:  # Reasonable range
                        results["temperature"] = True
                        print(f"✅ Temperature sensor: {temp}°C")
                
                if "Humidity:" in line and "%" in line:
                    humidity = float(line.split("Humidity:")[1].split("%")[0])
                    if 0 < humidity < 100:
                        results["humidity"] = True
                        print(f"✅ Humidity sensor: {humidity}%")
        
        return results
    
    def test_wifi_connectivity(self) -> bool:
        """Test WiFi connection"""
        print("📶 Testing WiFi connectivity...")
        
        # Send WiFi test command
        self.send_command("test_wifi")
        
        start_time = time.time()
        while time.time() - start_time < 30:
            if self.serial.in_waiting:
                line = self.serial.readline().decode().strip()
                
                if "WiFi connected" in line:
                    print("✅ WiFi connection successful")
                    return True
                elif "WiFi failed" in line:
                    print("❌ WiFi connection failed")
                    return False
        
        print("❌ WiFi test timeout")
        return False
    
    def test_api_communication(self) -> bool:
        """Test API communication"""
        print("🌐 Testing API communication...")
        
        self.send_command("test_api")
        
        start_time = time.time()
        while time.time() - start_time < 20:
            if self.serial.in_waiting:
                line = self.serial.readline().decode().strip()
                
                if "API response: 200" in line:
                    print("✅ API communication successful")
                    return True
                elif "API failed" in line:
                    print("❌ API communication failed")
                    return False
        
        return False
    
    def test_ota_functionality(self) -> bool:
        """Test OTA update mechanism"""
        print("🔄 Testing OTA functionality...")
        
        self.send_command("test_ota")
        
        start_time = time.time()
        while time.time() - start_time < 30:
            if self.serial.in_waiting:
                line = self.serial.readline().decode().strip()
                
                if "OTA check completed" in line:
                    print("✅ OTA system functional")
                    return True
                elif "OTA failed" in line:
                    print("❌ OTA system failed")
                    return False
        
        return False
    
    def send_command(self, command: str):
        """Send command to device"""
        if self.serial:
            self.serial.write(f"{command}\n".encode())
    
    def run_full_test_suite(self) -> Dict[str, bool]:
        """Run complete hardware validation"""
        print("🧪 Starting Hardware Validation Suite")
        print("=" * 50)
        
        if not self.connect_device():
            return {"connection": False}
        
        results = {}
        
        # Test sequence
        results["power_on"] = self.test_power_on()
        results["sensors"] = self.test_sensors()
        results["wifi"] = self.test_wifi_connectivity()
        results["api"] = self.test_api_communication()
        results["ota"] = self.test_ota_functionality()
        
        # Summary
        print("\n📊 TEST RESULTS SUMMARY")
        print("=" * 30)
        for test, result in results.items():
            status = "✅ PASS" if result else "❌ FAIL"
            print(f"{test:15}: {status}")
        
        total_tests = len(results)
        passed_tests = sum(1 for r in results.values() if r)
        print(f"\nOverall: {passed_tests}/{total_tests} tests passed")
        
        return results

if __name__ == "__main__":
    import sys
    
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    validator = HardwareValidator(port)
    results = validator.run_full_test_suite()
    
    # Exit with error code if any test failed
    if not all(results.values()):
        sys.exit(1)