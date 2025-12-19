// ESP32-S3 Zero Mini - WoL Telnet Command Server
// Version: 1.0.8
//
// Changelog:
// v1.0.8 - Reverted to simple Ctrl+Q escape only, all other chars pass through
// v1.0.7 - Fixed console escape: Ctrl+] and Ctrl+D exit cleanly, Ctrl+C passes through
// v1.0.6 - Added multiple console exit methods: Ctrl+Q, Ctrl+D, ~~~
// v1.0.5 - Fixed UART pins (RX=7, TX=8), poweroff now double press
// v1.0.4 - Fixed NeoPixel colors: WiFi=BLUE, Power=GREEN, Error=RED
// v1.0.3 - Added monitoring MAC to status, changed MAC format to colons
// v1.0.2 - Removed manual watchdog init (Arduino core auto-manages it)
// v1.0.1 - Fixed ESP-IDF 5.x compatibility (esp_task_wdt_init, flush, accept)
// v1.0.0 - ESP32-S3 Zero Mini adaptation with NeoPixel LED support

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

// Configuración WiFi
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASS "WIFI_PASS"

// Configuración UDP
#define UDP_PORT 9  // Puerto WoL

// Dirección MAC del dispositivo a monitorear
uint8_t targetMAC[] = {0x28, 0xc6, 0x8e, 0xd5, 0xc8, 0xdc};

// GPIO Configuration for ESP32-S3 Zero Mini
#define PIN_POWER_CTRL 3
#define PIN_DEVICE_STATUS 2
#define PIN_UART_TX 7    // ESP TX → Device RX (GPIO 7)
#define PIN_UART_RX 8    // ESP RX → Device TX (GPIO 8)


// NeoPixel Configuration
#define NEOPIXEL_PIN 21
#define NEOPIXEL_COUNT 1
#define NEOPIXEL_BRIGHTNESS 60

// Configuración Telnet
#define TELNET_PORT 23

// Variables
int wifiReconnectAttempts = 0;
bool deviceStatus = false;

// Objetos WiFi y UDP
WiFiUDP udp;
WiFiServer telnetServer(TELNET_PORT);
WiFiClient telnetClient;

// NeoPixel object
Adafruit_NeoPixel neopixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setNeoPixelColor(uint8_t r, uint8_t g, uint8_t b) {
    neopixel.setPixelColor(0, neopixel.Color(r, g, b));
    neopixel.show();
}

void blinkNeoPixel(uint8_t r, uint8_t g, uint8_t b, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        setNeoPixelColor(r, g, b);
        vTaskDelay(pdMS_TO_TICKS(delayMs));
        setNeoPixelColor(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(delayMs));
    }
}

bool isDeviceOn() {
    return digitalRead(PIN_DEVICE_STATUS) == HIGH;
}

String formatMAC(uint8_t* mac) {
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buffer);
}

void power_button(int delayMs) {
    setNeoPixelColor(NEOPIXEL_BRIGHTNESS, 0, 0);
    digitalWrite(PIN_POWER_CTRL, HIGH);
    vTaskDelay(pdMS_TO_TICKS(delayMs));
    digitalWrite(PIN_POWER_CTRL, LOW);
    setNeoPixelColor(0, 0, 0);
}

void checkWiFiConnection(void *parameter) {
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi desconectado, intentando reconectar...");
            blinkNeoPixel(0, 0, NEOPIXEL_BRIGHTNESS, 10, 200);
            WiFi.disconnect();
            WiFi.reconnect();
            wifiReconnectAttempts++;
            if (wifiReconnectAttempts >= 10) {
                Serial.println("No se pudo reconectar. Reiniciando ESP32...");
                blinkNeoPixel(0, NEOPIXEL_BRIGHTNESS, 0, 1, 3000);
                ESP.restart();
            }
        } else {
            blinkNeoPixel(0, 0, NEOPIXEL_BRIGHTNESS, 2, 200);
            wifiReconnectAttempts = 0;
            Serial.println("WiFi connection is OK.");
        }
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}

// Función para escuchar los paquetes WoL
void checkWakeOnLan(void *parameter) {
    while (true) {
        int packetSize = udp.parsePacket();
        if (packetSize) {
            uint8_t packetBuffer[102];
            udp.read(packetBuffer, sizeof(packetBuffer));

            bool match = true;
            for (int i = 6; i < 12; i++) {
                if (packetBuffer[i] != targetMAC[i - 6]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                if (isDeviceOn()) {
                    Serial.println("WoL Packet Received - Device is already ON, ignoring...");
                } else {
                    Serial.println("WoL Packet Received - Activating Device");
                    power_button(500);
                    blinkNeoPixel(NEOPIXEL_BRIGHTNESS, 0, 0, 10, 100);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Revisa por paquetes WoL constantemente
    }
}

void handleUARTConsole(WiFiClient &client) {
    Serial1.begin(115200, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
    client.println("Entering UART Console mode. Press Ctrl+Q to exit.");

    bool inConsoleMode = true;
    uint8_t uartBuffer[256];
    uint8_t telnetBuffer[256];
    unsigned long lastActivity = millis();

    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);

    while (inConsoleMode && client.connected()) {
        bool dataProcessed = false;

        if (client.available()) {
            size_t bytesRead = 0;
            while (client.available() && bytesRead < sizeof(telnetBuffer)) {
                uint8_t c = client.read();

                if (c == 255) continue;

                if (c == 17) {
                    client.println("\nExited UART console.");
                    inConsoleMode = false;
                    goto exit_console;
                }

                telnetBuffer[bytesRead++] = c;
            }

            if (bytesRead > 0) {
                Serial1.write(telnetBuffer, bytesRead);
                dataProcessed = true;
                lastActivity = millis();
            }
        }

        // Handle UART to telnet direction (bulk read/write)
        if (Serial1.available()) {
            size_t bytesRead = 0;
            while (Serial1.available() && bytesRead < sizeof(uartBuffer)) {
                uartBuffer[bytesRead++] = Serial1.read();
            }
            
            if (bytesRead > 0) {
                client.write(uartBuffer, bytesRead);
                dataProcessed = true;
                lastActivity = millis();
            }
        }

        // Adaptive yielding - only yield when no data for longer periods
        if (!dataProcessed) {
            if (millis() - lastActivity > 50) {
                // No activity for 50ms, yield briefly
                vTaskDelay(pdMS_TO_TICKS(1));
            } else {
                // Recent activity, just yield CPU without delay
                taskYIELD();
            }
        }
        
        // Reset watchdog timer during active streaming
        if (dataProcessed) {
            esp_task_wdt_reset();
        }
    }

exit_console:
    // Restore original task priority
    vTaskPrioritySet(NULL, 2);

    Serial1.end();
    client.println("UART connection closed.");
}

// Función para manejar el servidor Telnet
void handleTelnetServer(void *parameter) {
    telnetServer.begin();
    while (true) {
        if (!telnetClient || !telnetClient.connected()) {
            telnetClient = telnetServer.accept();
            if (telnetClient) {
                Serial.println("New Telnet connection!");
                telnetClient.println("ESP32 Telnet Server\nAvailable commands: status, reset, console, poweron, poweroff, poweron -c");
                telnetClient.print("> "); // Print initial prompt
            }
        }

        if (telnetClient.available()) {
            String command = telnetClient.readStringUntil('\n');
            command.trim();

            // Ignore empty lines or Telnet negotiation commands
            if (command.length() == 0 || command.charAt(0) == 255) { // 255 = IAC (Telnet negotiation)
                continue;
            }

            if (command == "status") {
                String wifiMAC = WiFi.macAddress();
                wifiMAC.toLowerCase();
                String monitoringMAC = formatMAC(targetMAC);
                telnetClient.printf(
                    "Wi-Fi SSID: %s\n"
                    "IP Address: %s\n"
                    "MAC Address: %s\n"
                    "Monitoring MAC: %s\n"
                    "Device Status: %s\n",
                    WIFI_SSID,
                    WiFi.localIP().toString().c_str(),
                    wifiMAC.c_str(),
                    monitoringMAC.c_str(),
                    isDeviceOn() ? "ON" : "OFF"
                );
            } else if (command == "reset") {
                telnetClient.println("Restarting ESP...");
                delay(1000);
                ESP.restart();
            } else if (command == "console") {
                handleUARTConsole(telnetClient);
            } else if (command == "poweron") {
                if (!isDeviceOn()) {
                    telnetClient.println("Turning device on...");
                    power_button(500);
                } else {
                    telnetClient.println("Error: Device is already ON.");
                }
            } else if (command == "poweron -c") {
                if (!isDeviceOn()) {
                    telnetClient.println("Turning device on and entering console mode...");
                    power_button(500);
                    // Wait a moment for the device to start
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    handleUARTConsole(telnetClient);
                } else {
                    telnetClient.println("Device is already ON, entering console mode...");
                    handleUARTConsole(telnetClient);
                }
            } else if (command == "poweroff") {
                if (isDeviceOn()) {
                    telnetClient.println("Turning device off...");
                    power_button(500);
                    vTaskDelay(pdMS_TO_TICKS(200));
                    power_button(500);
                } else {
                    telnetClient.println("Error: Device is already OFF.");
                }
            } else if (command == "poweroff -f") {
                if (isDeviceOn()) {
                    telnetClient.println("Forcing device off...");
                    power_button(5000);
                } else {
                    telnetClient.println("Error: Device is already OFF.");
                }
            } else if (command == "exit") {
                telnetClient.println("Goodbye!");
                telnetClient.stop();
                continue;  // Skip printing prompt for disconnected client
            } else {
                telnetClient.println("Unknown command");
            }

            // Print prompt after processing the command
            telnetClient.print("> ");
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // Yield control to avoid watchdog trigger
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_POWER_CTRL, OUTPUT);
    pinMode(PIN_DEVICE_STATUS, INPUT_PULLDOWN);
    digitalWrite(PIN_POWER_CTRL, LOW);

    neopixel.begin();
    neopixel.setBrightness(NEOPIXEL_BRIGHTNESS);
    neopixel.show();

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.printf("\nConectado a WiFi, IP: %s\n", WiFi.localIP().toString().c_str());
    blinkNeoPixel(0, 0, NEOPIXEL_BRIGHTNESS, 10, 200);
    udp.begin(UDP_PORT);

    xTaskCreatePinnedToCore(checkWiFiConnection, "WiFiCheck", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(checkWakeOnLan, "WoL", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(handleTelnetServer, "TelnetServer", 8192, NULL, 2, NULL, 1);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));  // Mantener FreeRTOS en funcionamiento
}
