#!/usr/bin/env python3
"""
Quick test script to verify ESP32 serial communication.
Run this to test the connection before using in Unreal Engine.

Usage: python3 test_serial_connection.py [port]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import time

def test_connection(port="/dev/ttyUSB0", baudrate=115200):
    print(f"Testing connection to {port} at {baudrate} baud...")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        print(f"Port opened successfully!")

        # Wait for device to reset (ESP32 resets when serial connects)
        time.sleep(2)

        # Clear any startup messages
        while ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"Startup message: {line}")

        # Test PING command
        print("\n--- Testing PING ---")
        ser.write(b"PING\n")
        response = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"Sent: PING")
        print(f"Response: {response}")

        if response == "PONG":
            print("PING test PASSED!")
        else:
            print(f"PING test FAILED - expected 'PONG', got '{response}'")

        # Test ECHO command
        print("\n--- Testing ECHO ---")
        ser.write(b"ECHO:Hello from Unreal!\n")
        response = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"Sent: ECHO:Hello from Unreal!")
        print(f"Response: {response}")

        # Test LED toggle
        print("\n--- Testing LED_TOGGLE ---")
        ser.write(b"LED_TOGGLE\n")
        response = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"Sent: LED_TOGGLE")
        print(f"Response: {response}")

        # Test STATUS
        print("\n--- Testing STATUS ---")
        ser.write(b"STATUS\n")
        response = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"Sent: STATUS")
        print(f"Response: {response}")

        # Test INFO
        print("\n--- Testing INFO ---")
        ser.write(b"INFO\n")
        time.sleep(0.5)
        print(f"Sent: INFO")
        while ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"Response: {line}")

        print("\n" + "="*50)
        print("Connection test completed!")
        print("="*50)

        ser.close()
        return True

    except serial.SerialException as e:
        print(f"Error: {e}")
        print("\nTroubleshooting tips:")
        print("1. Make sure the ESP32 is connected via USB")
        print("2. Check if the port exists: ls -la /dev/ttyUSB* /dev/ttyACM*")
        print("3. Make sure you have permission: sudo usermod -a -G dialout $USER")
        print("   (Then log out and back in)")
        print("4. Make sure no other program is using the port")
        return False

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    test_connection(port)
