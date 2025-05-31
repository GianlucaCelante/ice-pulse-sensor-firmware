#!/usr/bin/env python3
# device-tools/environmental_test.py

import time
import serial
import matplotlib.pyplot as plt
from datetime import datetime
import json

class EnvironmentalTester:
    def __init__(self, port: str):
        self.port = port
        self.serial = serial.Serial(port, 115200, timeout=2)
        self.data_log = []
    
    def log_readings_over_time(self, duration_minutes: int = 60):
        """Log sensor readings over specified time"""
        print(f"📊 Logging readings for {duration_minutes} minutes...")
        
        start_time = time.time()
        end_time = start_time + (duration_minutes * 60)
        
        while time.time() < end_time:
            try:
                # Request reading
                self.serial.write(b"get_reading\n")
                
                # Parse response
                line = self.serial.readline().decode().strip()
                if line:
                    timestamp = datetime.now()
                    
                    # Extract temperature and humidity
                    if "Temperature:" in line and "Humidity:" in line:
                        temp_str = line.split("Temperature:")[1].split("°C")[0].strip()
                        humidity_str = line.split("Humidity:")[1].split("%")[0].strip()
                        
                        temp = float(temp_str)
                        humidity = float(humidity_str)
                        
                        self.data_log.append({
                            "timestamp": timestamp,
                            "temperature": temp,
                            "humidity": humidity
                        })
                        
                        print(f"{timestamp}: T={temp}°C, H={humidity}%")
                
                time.sleep(30)  # Reading every 30 seconds
                
            except Exception as e:
                print(f"Error reading data: {e}")
                time.sleep(5)
        
        self.generate_report()
    
    def generate_report(self):
        """Generate environmental test report"""
        if not self.data_log:
            print("No data to report")
            return
        
        # Extract data for plotting
        timestamps = [d["timestamp"] for d in self.data_log]
        temperatures = [d["temperature"] for d in self.data_log]
        humidities = [d["humidity"] for d in self.data_log]
        
        # Create plots
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
        
        # Temperature plot
        ax1.plot(timestamps, temperatures, 'r-', label='Temperature')
        ax1.set_ylabel('Temperature (°C)')
        ax1.set_title('Environmental Test Results')
        ax1.grid(True)
        ax1.legend()
        
        # Humidity plot
        ax2.plot(timestamps, humidities, 'b-', label='Humidity')
        ax2.set_ylabel('Humidity (%)')
        ax2.set_xlabel('Time')
        ax2.grid(True)
        ax2.legend()
        
        plt.tight_layout()
        plt.savefig('environmental_test_results.png')
        print("📈 Report saved as environmental_test_results.png")
        
        # Statistics
        print("\n📊 STATISTICS:")
        print(f"Temperature - Min: {min(temperatures):.1f}°C, Max: {max(temperatures):.1f}°C, Avg: {sum(temperatures)/len(temperatures):.1f}°C")
        print(f"Humidity - Min: {min(humidities):.1f}%, Max: {max(humidities):.1f}%, Avg: {sum(humidities)/len(humidities):.1f}%")