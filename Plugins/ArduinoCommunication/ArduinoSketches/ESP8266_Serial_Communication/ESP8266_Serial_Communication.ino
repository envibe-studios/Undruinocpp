/**
 * ESP8266 Serial Communication for Unreal Engine
 *
 * This sketch enables bidirectional text communication between
 * an ESP8266 and Unreal Engine via USB Serial connection.
 *
 * Connection: USB to computer running Unreal Engine
 * Baud Rate: 115200 (configurable)
 *
 * Commands are sent as text strings terminated by newline (\n)
 *
 * Example usage:
 * - Unreal sends: "LED_ON\n"
 * - ESP8266 responds: "OK:LED_ON\n"
 */

// Built-in LED pin (varies by board)
#define LED_PIN LED_BUILTIN

// Serial baud rate - must match Unreal settings
#define BAUD_RATE 115200

// Buffer for incoming commands
String inputBuffer = "";
bool commandReady = false;

void setup() {
  // Initialize serial communication
  Serial.begin(BAUD_RATE);

  // Wait for serial port to connect
  while (!Serial) {
    delay(10);
  }

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Send ready message
  Serial.println("READY:ESP8266_Serial");

  // Reserve buffer space
  inputBuffer.reserve(256);
}

void loop() {
  // Read incoming serial data
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      // Command complete
      commandReady = true;
    } else if (c != '\r') {
      // Add to buffer (ignore carriage return)
      inputBuffer += c;
    }
  }

  // Process command if ready
  if (commandReady) {
    processCommand(inputBuffer);
    inputBuffer = "";
    commandReady = false;
  }
}

/**
 * Process incoming command from Unreal
 */
void processCommand(String command) {
  command.trim();

  if (command.length() == 0) {
    return;
  }

  // Parse command and parameters
  int separatorIndex = command.indexOf(':');
  String cmd = (separatorIndex > 0) ? command.substring(0, separatorIndex) : command;
  String params = (separatorIndex > 0) ? command.substring(separatorIndex + 1) : "";

  cmd.toUpperCase();

  // Handle commands
  if (cmd == "PING") {
    sendResponse("PONG", "");
  }
  else if (cmd == "LED_ON") {
    digitalWrite(LED_PIN, HIGH);
    sendResponse("OK", "LED_ON");
  }
  else if (cmd == "LED_OFF") {
    digitalWrite(LED_PIN, LOW);
    sendResponse("OK", "LED_OFF");
  }
  else if (cmd == "LED_TOGGLE") {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    sendResponse("OK", digitalRead(LED_PIN) ? "LED_ON" : "LED_OFF");
  }
  else if (cmd == "STATUS") {
    sendStatus();
  }
  else if (cmd == "ECHO") {
    sendResponse("ECHO", params);
  }
  else if (cmd == "ANALOG") {
    // Read analog value (A0)
    int value = analogRead(A0);
    sendResponse("ANALOG", String(value));
  }
  else if (cmd == "DIGITAL") {
    // Read or write digital pin
    // Format: DIGITAL:PIN,VALUE or DIGITAL:PIN
    int commaIndex = params.indexOf(',');
    if (commaIndex > 0) {
      // Write mode
      int pin = params.substring(0, commaIndex).toInt();
      int value = params.substring(commaIndex + 1).toInt();
      pinMode(pin, OUTPUT);
      digitalWrite(pin, value ? HIGH : LOW);
      sendResponse("OK", "DIGITAL:" + String(pin) + "=" + String(value));
    } else {
      // Read mode
      int pin = params.toInt();
      pinMode(pin, INPUT);
      int value = digitalRead(pin);
      sendResponse("DIGITAL", String(pin) + "=" + String(value));
    }
  }
  else if (cmd == "PWM") {
    // Set PWM value
    // Format: PWM:PIN,VALUE (0-255 or 0-1023 depending on board)
    int commaIndex = params.indexOf(',');
    if (commaIndex > 0) {
      int pin = params.substring(0, commaIndex).toInt();
      int value = params.substring(commaIndex + 1).toInt();
      pinMode(pin, OUTPUT);
      analogWrite(pin, value);
      sendResponse("OK", "PWM:" + String(pin) + "=" + String(value));
    } else {
      sendResponse("ERROR", "PWM requires PIN,VALUE");
    }
  }
  else if (cmd == "INFO") {
    sendInfo();
  }
  else {
    sendResponse("ERROR", "Unknown command: " + cmd);
  }
}

/**
 * Send a response back to Unreal
 */
void sendResponse(String type, String data) {
  if (data.length() > 0) {
    Serial.println(type + ":" + data);
  } else {
    Serial.println(type);
  }
}

/**
 * Send device status
 */
void sendStatus() {
  String status = "STATUS:";
  status += "LED=" + String(digitalRead(LED_PIN) ? "ON" : "OFF");
  status += ",UPTIME=" + String(millis());
  status += ",FREE_HEAP=" + String(ESP.getFreeHeap());
  Serial.println(status);
}

/**
 * Send device info
 */
void sendInfo() {
  Serial.println("INFO:DEVICE=ESP8266");
  Serial.println("INFO:SDK=" + String(ESP.getSdkVersion()));
  Serial.println("INFO:CHIP_ID=" + String(ESP.getChipId(), HEX));
  Serial.println("INFO:FLASH_SIZE=" + String(ESP.getFlashChipSize()));
  Serial.println("INFO:FREE_HEAP=" + String(ESP.getFreeHeap()));
}
