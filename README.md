# Unduinocpp - Unreal Engine ESP32/ESP8266 Communication Plugin

A plugin for **Unreal Engine 5.7** that enables bidirectional communication between Unreal Engine and ESP32/ESP8266 microcontrollers via USB serial or WiFi TCP connections.

## What This Project Does

This plugin bridges the gap between game development and physical hardware, allowing you to:

- **Control physical devices** from Unreal Engine (LEDs, motors, servos, relays)
- **Read sensor data** into your game (temperature, humidity, motion, light sensors)
- **Create interactive installations** where game events trigger real-world effects
- **Build hardware prototypes** with Unreal Engine as a visualization interface
- **Develop IoT applications** with a powerful 3D control hub

## Why ESP32/ESP8266?

These microcontrollers are ideal for this plugin because they offer:

- **Low cost** - Affordable for prototyping and production
- **Built-in WiFi** - Native wireless connectivity (no additional modules needed)
- **Arduino compatible** - Easy programming with familiar Arduino IDE
- **Flexible GPIO** - Multiple digital and analog pins for diverse hardware
- **Active community** - Extensive documentation and support

## Features

- **Dual Connection Modes**: USB Serial or WiFi TCP
- **Full Blueprint Support**: Events, functions, and properties exposed to Blueprints
- **Non-blocking I/O**: Threaded communication that won't freeze your game
- **Ready-to-use Arduino Sketches**: Example firmware included
- **Connection Test Actor**: Built-in testing tools for quick setup verification

## Quick Start

### 1. Hardware Setup

Upload the included Arduino sketch to your ESP32/ESP8266:

**For Serial (USB) connection:**
```
Plugins/ArduinoCommunication/ArduinoSketches/ESP8266_Serial_Communication/
```

**For WiFi connection:**
```
Plugins/ArduinoCommunication/ArduinoSketches/ESP8266_WiFi_Communication/
```

### 2. Unreal Engine Setup

1. Open the project in Unreal Engine 5.7
2. Add `ArduinoCommunicationComponent` to any actor
3. Configure connection settings:
   - **Serial**: Set `SerialPort` (e.g., "COM3") and `BaudRate` (default: 115200)
   - **WiFi**: Set `ConnectionMode` to TCP, then set `IPAddress` and `TCPPort`
4. Call `Connect()` and start communicating

### 3. Blueprint Example

```
Event BeginPlay
    -> Get ArduinoCommunicationComponent
    -> Set Serial Port = "COM3"
    -> Bind Event to OnDataReceived
    -> Connect

On Data Received (Data)
    -> Print String: Data

Custom Event: SendToArduino
    -> Send Line: "LED_ON"
```

### 4. C++ Example

```cpp
// In your actor
UPROPERTY(VisibleAnywhere)
UArduinoCommunicationComponent* ArduinoComm;

// In constructor
ArduinoComm = CreateDefaultSubobject<UArduinoCommunicationComponent>(TEXT("ArduinoComm"));
ArduinoComm->SerialPort = TEXT("COM3");

// In BeginPlay
ArduinoComm->OnDataReceived.AddDynamic(this, &AMyActor::HandleData);
ArduinoComm->Connect();

// Send commands
ArduinoComm->SendLine(TEXT("LED_ON"));
```

## Supported Commands

| Command | Description | Response |
|---------|-------------|----------|
| `PING` | Test connection | `PONG` |
| `LED_ON` | Turn on LED | `OK:LED_ON` |
| `LED_OFF` | Turn off LED | `OK:LED_OFF` |
| `LED_TOGGLE` | Toggle LED | `OK:LED_ON` or `OK:LED_OFF` |
| `STATUS` | Get device status | `STATUS:LED=ON,UPTIME=...` |
| `ANALOG` | Read analog A0 | `ANALOG:512` |
| `DIGITAL:pin` | Read GPIO pin | `DIGITAL:5=0` |
| `DIGITAL:pin,value` | Write GPIO pin | `DIGITAL:5=1` |
| `PWM:pin,value` | Set PWM (0-1023) | `OK:PWM:5=128` |
| `INFO` | Get device info | Multiple INFO lines |

## Example Applications

- **Escape Rooms** - Trigger physical locks, lights, and effects from game logic
- **Arcade Cabinets** - Read custom button panels and control feedback devices
- **Art Installations** - Synchronize visual experiences with physical elements
- **Training Simulators** - Interface with custom control panels and haptic feedback
- **Smart Home Demos** - Prototype IoT interfaces with 3D visualization

## Project Structure

```
Unduinocpp/
├── Source/Unduinocpp/              # Main game module
├── Plugins/
│   └── ArduinoCommunication/       # Communication plugin
│       ├── Source/                 # Plugin C++ code
│       ├── ArduinoSketches/        # Arduino firmware
│       └── README.md               # Detailed API documentation
├── Config/                         # Engine configuration
└── Unduinocpp.uproject             # Project file
```

## Requirements

- **Unreal Engine**: 5.7
- **Hardware**: ESP32 or ESP8266 microcontroller
- **Platform**: Windows (Linux/Mac serial support coming soon)

## Documentation

For detailed API documentation, troubleshooting, and advanced usage, see:
- [Plugin README](Plugins/ArduinoCommunication/README.md)

## License

MIT License - Use freely in your projects.
