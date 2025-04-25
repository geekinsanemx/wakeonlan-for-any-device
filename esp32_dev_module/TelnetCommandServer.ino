#include <WiFi.h>
#include <WiFiUdp.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Configuración WiFi
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASS "WIFI_PASS"

// Configuración UDP
#define UDP_PORT 9  // Puerto WoL

// Dirección MAC del dispositivo a monitorear
uint8_t targetMAC[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

// Configuración del GPIO para el transistor, LEDs y estado del dispositivo
#define RELAY_PIN 18
#define DEVICE_STATUS_PIN 19  // Pin que recibe 3.3V cuando el dispositivo está encendido
#define GREEN_LED 21
#define RED_LED 22
#define BLUE_LED 23


// Onboard LED
#define ONBOARD_LED 2  // GPIO 2 (onboard LED)

// Configuración Telnet
#define TELNET_PORT 23
#define UART_RX_PIN 16
#define UART_TX_PIN 17

// Variables
int wifiReconnectAttempts = 0;
bool deviceStatus = false;

// Objetos WiFi y UDP
WiFiUDP udp;
WiFiServer telnetServer(TELNET_PORT);
WiFiClient telnetClient;

// Función para parpadear LED
void blinkLED(int pin, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(delayMs));
        digitalWrite(pin, LOW);
        vTaskDelay(pdMS_TO_TICKS(delayMs));
    }
}

// Función para verificar si el dispositivo está encendido
bool isDeviceOn() {
    return digitalRead(DEVICE_STATUS_PIN) == HIGH;
}

// Función para simular el botón de encendido/apagado
void power_button(int delayMs) {
    digitalWrite(ONBOARD_LED, HIGH); // Encender LED verde mientras se pulsa el botón
    digitalWrite(RELAY_PIN, HIGH); // Pulsar el botón
    vTaskDelay(pdMS_TO_TICKS(delayMs));
    digitalWrite(RELAY_PIN, LOW); // Soltar el botón
    digitalWrite(ONBOARD_LED, LOW); // Apagar LED verde
}

// Función para verificar la conexión WiFi
void checkWiFiConnection(void *parameter) {
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi desconectado, intentando reconectar...");
            blinkLED(ONBOARD_LED, 10, 200);  // Blink onboard LED
            WiFi.disconnect();
            WiFi.reconnect();
            wifiReconnectAttempts++;
            if (wifiReconnectAttempts >= 10) {
                Serial.println("No se pudo reconectar. Reiniciando ESP32...");
                blinkLED(ONBOARD_LED, 1, 3000);  // Fast blink before restart
                ESP.restart();
            }
        } else {
            blinkLED(ONBOARD_LED, 2, 200);  // Parpadea LED onboard (conectado)
            wifiReconnectAttempts = 0;
            Serial.println("WiFi connection is OK.");
        }
        vTaskDelay(pdMS_TO_TICKS(15000));  // Chequea cada 15 segundos
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
                    power_button(500);  // Simula presionar el botón de encendido
                    blinkLED(ONBOARD_LED, 10, 100);  // Blink onboard LED to indicate activation
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Revisa por paquetes WoL constantemente
    }
}

// Function to handle UART console mode
void handleUARTConsole(WiFiClient &client) {
    Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    client.println("Entering UART Console mode. Press 'Ctrl+Q' to exit.");

    bool inConsoleMode = true;
    unsigned long lastYieldTime = millis();

    while (inConsoleMode && client.connected()) {
        if (client.available()) {
            char c = client.read();

            // Skip Telnet negotiation bytes (0xFF)
            if (c == 255) continue;

            if (c == 17) { // Ctrl+Q
                client.println("\nExited UART console.");
                inConsoleMode = false;
                break;
            }
            Serial1.write(c);
        }

        if (Serial1.available()) {
            char c = Serial1.read();
            client.write(c);
        }

        if (millis() - lastYieldTime >= 10) {
            vTaskDelay(pdMS_TO_TICKS(1));
            lastYieldTime = millis();
        }
    }

    Serial1.end();
    client.flush();
    client.println("UART connection closed.");
}

// Función para manejar el servidor Telnet
void handleTelnetServer(void *parameter) {
    telnetServer.begin();
    while (true) {
        if (!telnetClient || !telnetClient.connected()) {
            telnetClient = telnetServer.available();
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
                String macAddress = WiFi.macAddress();
                macAddress.replace(":", "-"); // Optional: Format with dashes instead of colons
                telnetClient.printf(
                    "Wi-Fi SSID: %s\n"
                    "IP Address: %s\n"
                    "MAC Address: %s\n"
                    "Device Status: %s\n",
                    WIFI_SSID,
                    WiFi.localIP().toString().c_str(),
                    macAddress.c_str(),
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

    // Configurar pines
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    pinMode(DEVICE_STATUS_PIN, INPUT_PULLDOWN);
    pinMode(ONBOARD_LED, OUTPUT);  // Configurar el LED onboard como salida
    digitalWrite(RELAY_PIN, LOW);

    // Conectar a la red WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.printf("\nConectado a WiFi, IP: %s\n", WiFi.localIP().toString().c_str());
    blinkLED(ONBOARD_LED, 10, 200);  // Parpadea LED onboard (conectado)
    udp.begin(UDP_PORT);

    // Crear tareas para manejar la conexión WiFi, WoL y Telnet
    xTaskCreatePinnedToCore(checkWiFiConnection, "WiFiCheck", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(checkWakeOnLan, "WoL", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(handleTelnetServer, "TelnetServer", 4096, NULL, 2, NULL, 0); // Higher priority
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));  // Mantener FreeRTOS en funcionamiento
}
