#!/usr/bin/env python3
# device-tools/calibration_test.py

import serial
import time

class SensorCalibrator:
    def __init__(self, port: str):
        self.serial = serial.Serial(port, 115200)
        
    def temperature_calibration(self, reference_temp: float):
        """Calibrate temperature sensor against reference"""
        print(f"🌡️ Calibrating against reference: {reference_temp}°C")
        
        readings = []
        for i in range(10):
            self.serial.write(b"get_temp\n")
            response = self.serial.readline().decode().strip()
            
            if "Temperature:" in response:
                temp = float(response.split(":")[1].split("°C")[0])
                readings.append(temp)
                print(f"Reading {i+1}: {temp}°C")
            
            time.sleep(2)
        
        avg_reading = sum(readings) / len(readings)
        offset = reference_temp - avg_reading
        
        print(f"📊 Average reading: {avg_reading:.2f}°C")
        print(f"🎯 Required offset: {offset:.2f}°C")
        
        # Apply calibration offset
        calibration_cmd = f"calibrate_temp {offset:.2f}\n"
        self.serial.write(calibration_cmd.encode())
        
        return offset

# Reference measurement setup
# Use certified thermometer alongside ESP32