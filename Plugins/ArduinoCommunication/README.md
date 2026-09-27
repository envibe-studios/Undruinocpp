# Arduino Communication Plugin for Unreal Engine

Send and receive text commands between Unreal Engine and Arduino ESP8266 via Serial (USB) or WiFi (TCP).

## Features

- **Serial Communication**: Connect via USB COM port
- **WiFi/TCP Communication**: Connect wirelessly over your network
- **Blueprint Support**: Full Blueprint integration with events and functions
- **Actor Component**: Easy-to-use component for any actor
- **Threaded I/O**: Non-blocking asynchronous communication
- **Cross-platform**: Windows support (Linux/Mac serial coming soon)

## Quick Start

### 1. Using the Connection Test Actor (Easiest)

For quick testing of your ESP32/ESP8266 connection:

1. Place `AArduinoConnectionTestActor` in your level
2. Configure connection settings on the ArduinoComponent:
   - For Serial: Set `SerialPort` (e.g., "COM3") and `BaudRate`
   - For TCP/WiFi: Set `ConnectionMode` to TCP, then set `IPAddress` and `TCPPort`
3. Optionally enable `bAutoTestOnBeginPlay` for automatic testing
4. Play the level and call `RunConnectionTest()` from Blueprint or C++
5. Check the test results via events or status properties

### 2. Using the Actor Component (Recommended)

1. Add `ArduinoCommunicationComponent` to any actor
2. Configure connection settings in the Details panel
3. Bind to the events (OnDataReceived, OnConnectionChanged, OnError)
4. Call `Connect()` to establish connection
5. Use `SendCommand()` or `SendLine()` to send messages

### 3. Blueprint Example

```
Event BeginPlay
    -> Get ArduinoCommunicationComponent
    -> Set Serial Port = "COM3"
    -> Set Baud Rate = 115200
    -> Bind Event to OnDataReceived
    -> Connect

On Data Received (Data)
    -> Print String: Data

Custom Event: SendToArduino
    -> Send Line: "LED_ON"
```

### 4. C++ Example

```cpp
// In your actor header
UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
UArduinoCommunicationComponent* ArduinoComm;

// In constructor
ArduinoComm = CreateDefaultSubobject<UArduinoCommunicationComponent>(TEXT("ArduinoComm"));
ArduinoComm->SerialPort = TEXT("COM3");
ArduinoComm->BaudRate = 115200;

// In BeginPlay
ArduinoComm->OnDataReceived.AddDynamic(this, &AMyActor::HandleArduinoData);
ArduinoComm->Connect();

// Send command
ArduinoComm->SendLine(TEXT("LED_ON"));
```

## Arduino Setup

### Serial Communication

1. Open `ArduinoSketches/ESP8266_Serial_Communication/ESP8266_Serial_Communication.ino`
2. Upload to your ESP8266
3. Note the COM port in Arduino IDE
4. Use that COM port in Unreal

### WiFi Communication

1. Open `ArduinoSketches/ESP8266_WiFi_Communication/ESP8266_WiFi_Communication.ino`
2. Set your WiFi credentials in the sketch
3. Upload to your ESP8266
4. Open Serial Monitor to see the IP address
5. Use that IP address in Unreal

## Supported Commands (Default Arduino Sketches)

| Command | Description | Example | Response |
|---------|-------------|---------|----------|
| PING | Test connection | `PING` | `PONG` |
| LED_ON | Turn on LED | `LED_ON` | `OK:LED_ON` |
| LED_OFF | Turn off LED | `LED_OFF` | `OK:LED_OFF` |
| LED_TOGGLE | Toggle LED | `LED_TOGGLE` | `OK:LED_ON` or `OK:LED_OFF` |
| STATUS | Get device status | `STATUS` | `STATUS:LED=ON,UPTIME=12345,...` |
| ECHO | Echo back text | `ECHO:Hello` | `ECHO:Hello` |
| ANALOG | Read analog A0 | `ANALOG` | `ANALOG:512` |
| DIGITAL | Read/write GPIO | `DIGITAL:5` or `DIGITAL:5,1` | `DIGITAL:5=0` |
| PWM | Set PWM output | `PWM:5,128` | `OK:PWM:5=128` |
| INFO | Get device info | `INFO` | Multiple INFO lines |

## API Reference

### UArduinoCommunicationComponent

**Properties:**
- `ConnectionMode` - Serial or TCP
- `SerialPort` - COM port name (e.g., "COM3")
- `BaudRate` - Serial baud rate (default: 115200)
- `IPAddress` - ESP8266 IP for TCP mode
- `TCPPort` - TCP port (default: 80)
- `bAutoConnect` - Connect automatically on BeginPlay

**Functions:**
- `Connect()` - Establish connection
- `Disconnect()` - Close connection
- `IsConnected()` - Check connection status
- `SendCommand(FString)` - Send raw text
- `SendLine(FString)` - Send text with newline
- `GetAvailablePorts()` - List COM ports

**Events:**
- `OnDataReceived(FString)` - Data received from Arduino
- `OnConnectionChanged(bool)` - Connection status changed
- `OnError(FString)` - Error occurred

### AArduinoConnectionTestActor

A ready-to-use actor for testing ESP32/ESP8266 connections.

**Properties:**
- `ArduinoComponent` - The embedded communication component (configure connection settings here)
- `bAutoTestOnBeginPlay` - Run test automatically when game starts
- `TestTimeoutSeconds` - Timeout for test responses (default: 5 seconds)
- `CurrentTestStatus` - Current test state (Idle, Testing, Success, Failed)
- `LastTestMessage` - Result message from last test
- `SuccessfulTests` / `FailedTests` - Test counters

**Functions:**
- `RunConnectionTest()` - Full test: connects (if needed) and sends PING
- `RunPingTest()` - Send PING, expect PONG (must be connected)
- `RunLedToggleTest()` - Send LED_TOGGLE command
- `RunStatusTest()` - Query device status
- `RunCustomCommandTest(Command)` - Test any custom command
- `CancelTest()` - Cancel running test
- `ResetTestStats()` - Reset success/failure counters
- `GetStatusString()` - Human-readable status
- `IsConnected()` - Check connection status

**Events:**
- `OnTestCompleted(bSuccess, Message)` - Test finished with result
- `OnTestStatusChanged(NewStatus)` - Test state changed

### UArduinoSerialPort

Direct serial port access for advanced use cases.

### UArduinoTcpClient

Direct TCP client access for advanced use cases.

### UArduinoBlueprintLibrary

Static utility functions:
- `GetAvailableComPorts()` - List available COM ports
- `ParseArduinoResponse(Response, OutType, OutData)` - Parse response
- `MakeCommand(Command, Parameter)` - Create command string
- `IsValidIPAddress(IP)` - Validate IP address
- `ParseKeyValue(Data, Key, OutValue)` - Extract key=value pairs

## Troubleshooting

### Serial Connection Issues

1. **Port not found**: Ensure ESP8266 is connected and drivers are installed
2. **Permission denied**: Close Arduino IDE Serial Monitor
3. **No data received**: Check baud rate matches (default: 115200)
4. **Garbled text**: Verify both sides use same baud rate

### WiFi Connection Issues

1. **Connection refused**: Check IP address and port
2. **Timeout**: Ensure ESP8266 is on same network
3. **No response**: Check firewall settings
4. **Disconnects**: Verify WiFi signal strength

## Multi-Ship Architecture (AndySerialSubsystem)

For games with multiple ships, each with its own ESP32 "Andy" hub, use the `UAndySerialSubsystem` GameInstanceSubsystem. This provides centralized management of multiple serial ports that persist across level loads.

### Architecture Overview

```
UAndySerialSubsystem (GameInstanceSubsystem)
├── TMap<FName, FAndyPortConnection>
│   ├── "ShipA" -> SerialPort, Parser, EventHandler
│   └── "ShipB" -> SerialPort, Parser, EventHandler
└── Events: OnFrameParsed, OnConnectionChanged

UShipHardwareInputComponent (ActorComponent on Ship Pawn)
├── ShipId: "ShipA"
├── Filters events by ShipId
└── Events: OnWeaponImu, OnWheelTurn, OnWeaponTag, OnReloadTag
```

### Blueprint Usage - Setting Up Ports

```
// In your GameMode or persistent manager Blueprint:

Event BeginPlay
    -> Get Game Instance Subsystem (UAndySerialSubsystem)
    -> Add Port (ShipId: "ShipA", PortName: "COM3", BaudRate: 115200)
    -> Add Port (ShipId: "ShipB", PortName: "COM4", BaudRate: 115200)
    -> Start All
```

### Blueprint Usage - Ship Actor with Hardware Input

```
// On your Ship Pawn/Actor:

1. Add Component -> Ship Hardware Input Component
2. Set ShipId = "ShipA" (must match port registration)
3. Bind events in Event Graph:

Event On Weapon Imu (Src, Orientation, bTriggerHeld)
    -> Set Actor Rotation from Quaternion
    -> Branch on bTriggerHeld -> Fire Weapon

Event On Wheel Turn (Src, Delta)
    -> Add to Steering Angle (Delta * SteeringSensitivity)

Event On Weapon Tag (Src, TagId, bInserted)
    -> Branch on bInserted
        -> True: Equip Weapon by TagId
        -> False: Holster Weapon

Event On Reload Tag (Src, TagId, bInserted)
    -> Branch on bInserted
        -> True: Start Reload
```

### C++ Usage

```cpp
// Get the subsystem
UAndySerialSubsystem* AndySub = GetGameInstance()->GetSubsystem<UAndySerialSubsystem>();

// Register ports
AndySub->AddPort(FName("ShipA"), TEXT("COM3"), 115200);
AndySub->AddPort(FName("ShipB"), TEXT("COM4"), 115200);

// Start all ports
AndySub->StartAll();

// Check connection status
if (AndySub->IsConnected(FName("ShipA")))
{
    // Send data
    AndySub->SendLine(FName("ShipA"), TEXT("!calibrate"));
}

// In your Ship Actor, add the component:
UPROPERTY(VisibleAnywhere)
UShipHardwareInputComponent* HardwareInput;

// Constructor
HardwareInput = CreateDefaultSubobject<UShipHardwareInputComponent>(TEXT("HardwareInput"));
HardwareInput->ShipId = FName("ShipA");

// BeginPlay - bind events
HardwareInput->OnWeaponImu.AddDynamic(this, &AMyShip::HandleWeaponImu);
HardwareInput->OnWheelTurn.AddDynamic(this, &AMyShip::HandleWheelTurn);

void AMyShip::HandleWeaponImu(uint8 Src, FQuat Orientation, bool bTriggerHeld)
{
    WeaponMesh->SetWorldRotation(Orientation.Rotator());
    if (bTriggerHeld) FireWeapon();
}

void AMyShip::HandleWheelTurn(uint8 Src, int32 Delta)
{
    SteeringAngle += Delta * SteeringSensitivity;
}
```

### UAndySerialSubsystem API

**Port Management:**
- `AddPort(FName ShipId, FString PortName, int32 BaudRate)` - Register a new serial port
- `RemovePort(FName ShipId)` - Remove and close a port
- `StartAll()` - Open all registered ports
- `StopAll()` - Close all ports
- `IsConnected(FName ShipId)` - Check if a port is connected
- `GetAllShipIds()` - Get list of registered ship IDs

**Data Transmission:**
- `SendBytes(FName ShipId, TArray<uint8> Data)` - Send raw bytes
- `SendLine(FName ShipId, FString Line)` - Send text with newline

**Events:**
- `OnFrameParsed(FName ShipId, uint8 Src, uint8 Type, int32 Seq, TArray<uint8> Payload)` - Parsed packet received
- `OnConnectionChanged(FName ShipId, bool bConnected)` - Connection status changed

### UShipHardwareInputComponent API

**Configuration:**
- `ShipId` - Must match the ShipId in UAndySerialSubsystem
- `bServerOnly` - If true, only processes events on server (default: true)

**Status:**
- `IsConnected()` - Check if this ship's port is connected
- `GetSerialSubsystem()` - Get reference to the subsystem

**Events:**
- `OnWeaponImu(uint8 Src, FQuat Orientation, bool bTriggerHeld)` - IMU orientation data
- `OnWheelTurn(uint8 Src, int32 Delta)` - Rotary encoder turn (+1/-1)
- `OnWeaponTag(uint8 Src, int64 TagId, bool bInserted)` - Weapon RFID tag
- `OnReloadTag(uint8 Src, int64 TagId, bool bInserted)` - Reload RFID tag
- `OnShipConnectionChanged(bool bConnected)` - Connection status for this ship

### Thread Safety

- Serial reading happens on background threads
- All delegate broadcasts are marshaled to the game thread via `AsyncTask(ENamedThreads::GameThread, ...)`
- Each port has its own independent parser instance (no shared state)

## Multi-Display Camera System

For outputting camera views to multiple monitors, see **[MULTI_DISPLAY_SETUP.md](MULTI_DISPLAY_SETUP.md)**.

The `UMultiDisplayCameraComponent` extends `USceneCaptureComponent2D` to open a secondary OS window on any connected monitor, displaying a real-time camera view. Add one component per monitor, set the `TargetDisplayIndex`, and run as Standalone Game.

## License

MIT License - Use freely in your projects.
