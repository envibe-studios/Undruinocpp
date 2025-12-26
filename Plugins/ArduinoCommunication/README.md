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

### 1. Using the Actor Component (Recommended)

1. Add `ArduinoCommunicationComponent` to any actor
2. Configure connection settings in the Details panel
3. Bind to the events (OnDataReceived, OnConnectionChanged, OnError)
4. Call `Connect()` to establish connection
5. Use `SendCommand()` or `SendLine()` to send messages

### 2. Blueprint Example

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

### 3. C++ Example

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

## License

MIT License - Use freely in your projects.
