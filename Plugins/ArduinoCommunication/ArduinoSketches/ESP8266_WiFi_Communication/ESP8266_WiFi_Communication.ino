/**
 * ESP8266 WiFi/TCP Communication for Unreal Engine
 *
 * This sketch enables bidirectional text communication between
 * an ESP8266 and Unreal Engine via WiFi TCP connection.
 *
 * The ESP8266 acts as a TCP server that Unreal connects to.
 *
 * Commands are sent as text strings terminated by newline (\n)
 *
 * Configuration:
 * - Set your WiFi credentials below
 * - Default TCP port: 80
 *
 * Example usage:
 * - Unreal connects to ESP8266 IP address
 * - Unreal sends: "LED_ON\n"
 * - ESP8266 responds: "OK:LED_ON\n"
 */

#include <ESP8266WiFi.h>

// ============== CONFIGURE THESE ==============
const char* WIFI_SSID = "YOUR_WIFI_SSID";      // Your WiFi network name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASS";  // Your WiFi password
const int TCP_PORT = 80;                        // TCP port for server
// =============================================

// Built-in LED pin
#define LED_PIN LED_BUILTIN

// TCP Server
WiFiServer server(TCP_PORT);
WiFiClient client;

// Buffer for incoming commands
String inputBuffer = "";
bool commandReady = false;

// Connection status tracking
bool wasConnected = false;

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println("\n\nESP8266 WiFi Communication Starting...");

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED off (inverted on most ESP8266 boards)

  // Connect to WiFi
  connectWiFi();

  // Start TCP server
  server.begin();
  Serial.print("TCP Server started on port ");
  Serial.println(TCP_PORT);

  // Print IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Blink LED to indicate ready
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
  }

  inputBuffer.reserve(256);
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectWiFi();
  }

  // Check for new client connections
  if (server.hasClient()) {
    if (client && client.connected()) {
      // Already have a client, reject new one
      WiFiClient newClient = server.available();
      newClient.stop();
    } else {
      // Accept new client
      client = server.available();
      Serial.println("Client connected");
      client.println("READY:ESP8266_WiFi");
      wasConnected = true;
    }
  }

  // Handle connected client
  if (client && client.connected()) {
    // Read incoming data
    while (client.available() > 0) {
      char c = client.read();

      if (c == '\n') {
        commandReady = true;
      } else if (c != '\r') {
        inputBuffer += c;
      }
    }

    // Process command if ready
    if (commandReady) {
      processCommand(inputBuffer);
      inputBuffer = "";
      commandReady = false;
    }
  } else if (wasConnected) {
    // Client disconnected
    Serial.println("Client disconnected");
    wasConnected = false;
    inputBuffer = "";
    commandReady = false;
  }

  // Small delay to prevent watchdog issues
  yield();
}

/**
 * Connect to WiFi network
 */
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Blink LED while connecting
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_PIN, HIGH);  // LED off
  } else {
    Serial.println("\nFailed to connect to WiFi!");
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

  Serial.print("Received command: ");
  Serial.println(command);

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
    digitalWrite(LED_PIN, LOW);  // Inverted
    sendResponse("OK", "LED_ON");
  }
  else if (cmd == "LED_OFF") {
    digitalWrite(LED_PIN, HIGH);  // Inverted
    sendResponse("OK", "LED_OFF");
  }
  else if (cmd == "LED_TOGGLE") {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    sendResponse("OK", digitalRead(LED_PIN) ? "LED_OFF" : "LED_ON");  // Inverted
  }
  else if (cmd == "STATUS") {
    sendStatus();
  }
  else if (cmd == "ECHO") {
    sendResponse("ECHO", params);
  }
  else if (cmd == "ANALOG") {
    int value = analogRead(A0);
    sendResponse("ANALOG", String(value));
  }
  else if (cmd == "DIGITAL") {
    int commaIndex = params.indexOf(',');
    if (commaIndex > 0) {
      int pin = params.substring(0, commaIndex).toInt();
      int value = params.substring(commaIndex + 1).toInt();
      pinMode(pin, OUTPUT);
      digitalWrite(pin, value ? HIGH : LOW);
      sendResponse("OK", "DIGITAL:" + String(pin) + "=" + String(value));
    } else {
      int pin = params.toInt();
      pinMode(pin, INPUT);
      int value = digitalRead(pin);
      sendResponse("DIGITAL", String(pin) + "=" + String(value));
    }
  }
  else if (cmd == "PWM") {
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
  else if (cmd == "WIFI") {
    sendWiFiInfo();
  }
  else {
    sendResponse("ERROR", "Unknown command: " + cmd);
  }
}

/**
 * Send a response back to Unreal
 */
void sendResponse(String type, String data) {
  String response;
  if (data.length() > 0) {
    response = type + ":" + data;
  } else {
    response = type;
  }

  if (client && client.connected()) {
    client.println(response);
  }
  Serial.println("Sent: " + response);
}

/**
 * Send device status
 */
void sendStatus() {
  String status = "STATUS:";
  status += "LED=" + String(digitalRead(LED_PIN) ? "OFF" : "ON");  // Inverted
  status += ",UPTIME=" + String(millis());
  status += ",FREE_HEAP=" + String(ESP.getFreeHeap());
  status += ",WIFI_RSSI=" + String(WiFi.RSSI());

  if (client && client.connected()) {
    client.println(status);
  }
  Serial.println("Sent: " + status);
}

/**
 * Send device info
 */
void sendInfo() {
  if (client && client.connected()) {
    client.println("INFO:DEVICE=ESP8266");
    client.println("INFO:SDK=" + String(ESP.getSdkVersion()));
    client.println("INFO:CHIP_ID=" + String(ESP.getChipId(), HEX));
    client.println("INFO:FLASH_SIZE=" + String(ESP.getFlashChipSize()));
    client.println("INFO:FREE_HEAP=" + String(ESP.getFreeHeap()));
  }
}

/**
 * Send WiFi connection info
 */
void sendWiFiInfo() {
  if (client && client.connected()) {
    client.println("WIFI:SSID=" + WiFi.SSID());
    client.println("WIFI:IP=" + WiFi.localIP().toString());
    client.println("WIFI:MAC=" + WiFi.macAddress());
    client.println("WIFI:RSSI=" + String(WiFi.RSSI()));
    client.println("WIFI:CHANNEL=" + String(WiFi.channel()));
  }
}
