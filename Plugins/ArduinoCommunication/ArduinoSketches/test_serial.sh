#!/bin/bash
# Quick test script to verify ESP32 serial communication
# Usage: ./test_serial.sh [port]
# Default port: /dev/ttyUSB0

PORT="${1:-/dev/ttyUSB0}"
BAUD=115200

echo "Testing connection to $PORT at $BAUD baud..."

# Check if port exists
if [ ! -e "$PORT" ]; then
    echo "Error: Port $PORT does not exist!"
    echo "Available ports:"
    ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  No USB serial ports found"
    exit 1
fi

# Check permissions
if [ ! -r "$PORT" ] || [ ! -w "$PORT" ]; then
    echo "Error: No permission to access $PORT"
    echo "Try: sudo usermod -a -G dialout $USER"
    echo "Then log out and back in"
    exit 1
fi

# Configure serial port
stty -F "$PORT" $BAUD cs8 -cstopb -parenb -echo raw

echo "Port configured successfully!"
echo ""

# Function to send command and read response
send_command() {
    local cmd="$1"
    echo "Sending: $cmd"
    echo "$cmd" > "$PORT"
    sleep 0.5
    # Read response with timeout
    local response=""
    response=$(timeout 2 head -n 1 "$PORT" 2>/dev/null)
    echo "Response: $response"
    echo ""
}

# Wait for device reset
echo "Waiting for device..."
sleep 2

# Read any startup messages
echo "--- Startup messages ---"
timeout 1 cat "$PORT" 2>/dev/null &
PID=$!
sleep 1
kill $PID 2>/dev/null
echo ""

# Test PING
echo "--- Testing PING ---"
send_command "PING"

# Test ECHO
echo "--- Testing ECHO ---"
send_command "ECHO:Hello"

# Test LED
echo "--- Testing LED_TOGGLE ---"
send_command "LED_TOGGLE"

# Test STATUS
echo "--- Testing STATUS ---"
send_command "STATUS"

echo "=================================="
echo "Test completed!"
echo "=================================="
