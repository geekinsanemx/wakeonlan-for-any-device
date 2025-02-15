#include <WiFi.h>
#include <WiFiUdp.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Configuración WiFi
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

// Configuración UDP
WiFiUDP udp;
const int udpPort = 9;  // Puerto WoL

// Dirección MAC del dispositivo a monitorear
uint8_t targetMAC[] = {0xA1, 0xB2, 0xC3, 0xd4, 0xE5, 0xF6};

// Configuración del GPIO para el transistor, LEDs y estado del dispositivo
const int relayPin = 5;
const int greenLed = 18;
const int redLed = 19;
const int blueLed = 21;
const int deviceStatusPin = 22;  // Pin que recibe 3.3V cuando el dispositivo está encendido

// Onboard LED
const int onboardLED = 2;  // GPIO 2 (onboard LED)

// Variables
int wifiReconnectAttempts = 0;
bool deviceStatus = false;

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
    return digitalRead(deviceStatusPin) == HIGH;
}

// Función para simular el botón de encendido/apagado
void power_button(int delayMs) {
    digitalWrite(greenLed, HIGH); // Encender LED verde mientras se pulsa el botón
    digitalWrite(relayPin, HIGH); // Pulsar el botón
    vTaskDelay(pdMS_TO_TICKS(delayMs));
    digitalWrite(relayPin, LOW); // Soltar el botón
    digitalWrite(greenLed, LOW); // Apagar LED verde
}

// Función para verificar la conexión WiFi
void checkWiFiConnection(void *parameter) {
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi desconectado, intentando reconectar...");
            blinkLED(onboardLED, 10, 200);  // Blink onboard LED
            WiFi.disconnect();
            WiFi.reconnect();
            wifiReconnectAttempts++;
            if (wifiReconnectAttempts >= 10) {
                Serial.println("No se pudo reconectar. Reiniciando ESP32...");
                blinkLED(onboardLED, 1, 3000);  // Fast blink before restart
                ESP.restart();
            }
        } else {
            blinkLED(onboardLED, 2, 200);  // Parpadea LED onboard (conectado)
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
                    blinkLED(onboardLED, 10, 100);  // Blink onboard LED to indicate activation
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Revisa por paquetes WoL constantemente
    }
}

void setup() {
    Serial.begin(115200);

    // Configurar pines
    pinMode(relayPin, OUTPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(redLed, OUTPUT);
    pinMode(blueLed, OUTPUT);
    pinMode(deviceStatusPin, INPUT);
    pinMode(onboardLED, OUTPUT);  // Configurar el LED onboard como salida
    digitalWrite(relayPin, LOW);

    // Conectar a la red WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.printf("\nConectado a WiFi, IP: %s\n", WiFi.localIP().toString().c_str());
    blinkLED(onboardLED, 10, 200);  // Parpadea LED onboard (conectado)
    udp.begin(udpPort);

    // Crear tareas para manejar la conexión WiFi y WoL
    xTaskCreatePinnedToCore(checkWiFiConnection, "WiFiCheck", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(checkWakeOnLan, "WoL", 4096, NULL, 1, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));  // Mantener FreeRTOS en funcionamiento
}
