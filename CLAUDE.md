# CLAUDE.md - Project Guide for AI Assistants

## Purpose

The primary goal of this project is to create a reusable Unreal Engine plugin that enables seamless, bidirectional communication between Unreal Engine applications and ESP32/ESP8266 microcontrollers. By bridging the gap between game development and physical hardware, this plugin empowers developers to:

- **Control physical devices** from within Unreal Engine (LEDs, motors, servos, relays)
- **Read sensor data** into Unreal Engine (temperature, humidity, motion, light sensors)
- **Create interactive installations** where game events trigger real-world effects
- **Build hardware prototypes** with a powerful visualization and control interface
- **Develop IoT applications** with Unreal Engine as the central hub

The plugin supports both USB serial and WiFi TCP connections, making it flexible for wired development setups and wireless production deployments.

### Why ESP32/ESP8266?

The ESP32 and ESP8266 microcontrollers are ideal for this plugin because they offer:
- **Low cost** - Affordable hardware for prototyping and production
- **Built-in WiFi** - Native wireless connectivity without additional modules
- **Arduino compatibility** - Easy programming with familiar Arduino IDE and libraries
- **GPIO flexibility** - Multiple digital and analog pins for diverse hardware interfacing
- **Active community** - Extensive documentation, libraries, and community support

### Example Applications

- **Escape rooms** - Trigger physical locks, lights, and effects from game logic
- **Arcade cabinets** - Read custom button panels and control feedback devices
- **Art installations** - Synchronize visual experiences with physical elements
- **Training simulators** - Interface with custom control panels and haptic feedback
- **Smart home demos** - Prototype IoT control interfaces with rich 3D visualization

## Project Overview

This is an **Unreal Engine 5.7** project called **Unduinocpp** with a custom plugin for bidirectional communication between Unreal Engine and Arduino ESP32/ESP8266 microcontrollers.

## Tech Stack

- **Engine**: Unreal Engine 5.7
- **Language**: C++
- **Build System**: Unreal Build Tool (UBT)
- **Hardware**: Arduino ESP32/ESP8266 (Serial USB or WiFi/TCP)

## Project Structure

```
/
├── Source/Unduinocpp/           # Main game module
├── Plugins/ArduinoCommunication/ # Arduino communication plugin
│   ├── Source/                   # Plugin C++ source
│   │   └── ArduinoCommunication/
│   │       ├── Public/           # Public headers
│   │       └── Private/          # Implementation files
│   ├── ArduinoSketches/          # Arduino firmware (.ino files)
│   └── README.md                 # Detailed plugin documentation
├── Config/                       # Engine configuration files
├── Binaries/                     # Compiled binaries
└── Unduinocpp.uproject          # Project file
```

## Key Files

- **Plugin Component**: `Plugins/ArduinoCommunication/Source/ArduinoCommunication/Public/ArduinoCommunicationComponent.h`
- **Serial Port**: `Plugins/ArduinoCommunication/Source/ArduinoCommunication/Public/ArduinoSerialPort.h`
- **TCP Client**: `Plugins/ArduinoCommunication/Source/ArduinoCommunication/Public/ArduinoTcpClient.h`
- **Blueprint Library**: `Plugins/ArduinoCommunication/Source/ArduinoCommunication/Public/ArduinoBlueprintLibrary.h`
- **Build Config**: `Plugins/ArduinoCommunication/Source/ArduinoCommunication/ArduinoCommunication.Build.cs`

## Build Commands

Open the project in Unreal Editor 5.7 and use the Compile button, or from command line:

```bash
# Windows
UnrealEngine\UE_5.7\Engine\Build\BatchFiles\Build.bat Unduinocpp Win64 Development "/path/to/Unduinocpp.uproject"

# Linux
/path/to/UnrealEngine/UE_5.7/Engine/Build/BatchFiles/Linux/Build.sh Unduinocpp Linux Development "/path/to/Unduinocpp.uproject"

# Generate project files (Windows)
UnrealEngine\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat "/path/to/Unduinocpp.uproject"

# Generate project files (Linux)
/path/to/UnrealEngine/UE_5.7/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh "/path/to/Unduinocpp.uproject"
```

### Compiling from Unreal Editor

1. Open the project in Unreal Editor 5.7
2. Click the **Compile** button in the toolbar (or press Ctrl+Alt+F11)
3. Check the **Output Log** window for any compilation errors
4. Fix any errors before proceeding with other tasks

> **IMPORTANT FOR AI ASSISTANTS**: Before completing any task that modifies C++ code, you MUST compile the project and fix any compilation errors. Do not consider a task finished until the project compiles successfully without errors.

## Architecture

### Communication Modes
- **Serial Mode**: USB connection via COM port (default: 115200 baud)
- **TCP Mode**: WiFi connection via TCP socket (default port: 80)

### Threading Model
- Non-blocking I/O with separate read threads
- Thread-safe message queues (`TQueue<FString>`)
- Game thread processing via timers
- `FCriticalSection` for thread safety

### Event System (Delegates)
- `FOnArduinoDataReceived` - Data received from Arduino
- `FOnArduinoConnectionChanged` - Connection status changes
- `FOnArduinoError` - Error events

## Command Protocol

Text-based commands with newline termination:
```
Format:   COMMAND[:PARAMETER]\n
Response: TYPE[:DATA]\n

Examples:
  PING      -> PONG
  LED_ON    -> OK:LED_ON
  STATUS    -> STATUS:LED=ON,UPTIME=12345,...
```

### Available Commands
| Command | Description |
|---------|-------------|
| PING | Test connection |
| LED_ON/OFF/TOGGLE | Control onboard LED |
| STATUS | Get device status |
| ANALOG | Read analog input (A0) |
| DIGITAL:pin,value | Read/write GPIO |
| PWM:pin,value | Set PWM output (0-1023) |
| INFO | Get device information |

## Module Dependencies

**Game Module** (`Unduinocpp`):
- Core, CoreUObject, Engine, InputCore, EnhancedInput

**Plugin Module** (`ArduinoCommunication`):
- Core, CoreUObject, Engine, Sockets, Networking

## Code Conventions

- Full Blueprint support via `UCLASS`, `UFUNCTION`, `UPROPERTY` macros
- Properties organized by category: `Arduino|Connection`, `Arduino|Serial`, `Arduino|TCP`, etc.
- Explicit PCH usage (`bUseUnity = false`)
- Platform-specific implementations with stubs for unsupported platforms

## Arduino Setup

Arduino sketches are in `Plugins/ArduinoCommunication/ArduinoSketches/`:
- `ESP8266_Serial_Communication/` - USB serial communication
- `ESP8266_WiFi_Communication/` - WiFi TCP server mode

Configure WiFi credentials in the WiFi sketch before uploading.

## Testing

1. Upload appropriate Arduino sketch to ESP8266
2. Add `UArduinoCommunicationComponent` to an actor
3. Configure connection mode (Serial or TCP)
4. Connect and send commands via Blueprint or C++

## Additional Documentation

See `Plugins/ArduinoCommunication/README.md` for comprehensive documentation including:
- Quick start guide
- Complete API reference
- Troubleshooting guide
- Blueprint and C++ examples
