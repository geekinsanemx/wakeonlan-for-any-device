#include <WiFi.h>
#include <WiFiUdp.h>
#include <TelnetStream.h>
#include <Arduino.h>
#include <TaskScheduler.h>

// Configuración WiFi
const char* ssid = "TU_SSID";
const char* password = "TU_PASSWORD";

// Configuración UDP
WiFiUDP udp;
const int udpPort = 9;  // Puerto WoL

// Dirección MAC del dispositivo a monitorear
uint8_t targetMAC[] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};

// Configuración del GPIO para el transistor, LEDs y estado del dispositivo
const int relayPin = 5;
const int greenLed = 18;
const int redLed = 19;
const int blueLed = 21;
const int deviceStatusPin = 22;

// Configuración UART para el puente TTL
const int uartTx = 17;
const int uartRx = 16;
HardwareSerial externalDevice(1);

int wifiReconnectAttempts = 0;
bool authenticated = false;
bool inTelnetPrompt = false;

Scheduler runner;

void blinkLED(int pin, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayMs);
        digitalWrite(pin, LOW);
        delay(delayMs);
    }
}

bool isDeviceOn() {
    return digitalRead(deviceStatusPin) == HIGH;
}

void power_button(int delayMs) {
    digitalWrite(greenLed, HIGH);
    digitalWrite(relayPin, HIGH);
    delay(delayMs);
    digitalWrite(relayPin, LOW);
    digitalWrite(greenLed, LOW);
}

void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado, intentando reconectar...");
        blinkLED(blueLed, 10, 200);
        WiFi.disconnect();
        WiFi.reconnect();
        wifiReconnectAttempts++;
        if (wifiReconnectAttempts >= 10) {
            Serial.println("No se pudo reconectar. Reiniciando ESP32...");
            blinkLED(redLed, 10, 200);
            ESP.restart();
        }
    } else {
        blinkLED(blueLed, 2, 200);
        wifiReconnectAttempts = 0;
    }
}

Task taskCheckWiFi(15000, TASK_FOREVER, &checkWiFiConnection);

void checkWakeOnLan() {
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
                power_button(1000);
            }
        }
    }
}

void handleTelnetPrompt() {
    TelnetStream.print("telnet> ");
    String command = TelnetStream.readStringUntil('\n');
    command.trim();

    if (command == "poweron") {
        if (isDeviceOn()) {
            TelnetStream.println("El dispositivo ya está encendido.");
        } else {
            TelnetStream.println("Encendiendo dispositivo...");
            power_button(1000);
        }
    } else if (command == "poweroff") {
        if (!isDeviceOn()) {
            TelnetStream.println("El dispositivo ya está apagado.");
        } else {
            TelnetStream.println("Apagando dispositivo...");
            power_button(1000);
        }
    } else if (command == "poweroff -f") {
        if (!isDeviceOn()) {
            TelnetStream.println("El dispositivo ya está apagado.");
        } else {
            TelnetStream.println("Forzando apagado del dispositivo...");
            power_button(5000);
        }
    } else if (command == "exit") {
        TelnetStream.println("Saliendo del modo de comandos...");
        inTelnetPrompt = false;
    } else {
        TelnetStream.println("Comando no reconocido.");
    }
}

void handleTelnet() {
    if (TelnetStream.available()) {
        char input = TelnetStream.read();

        if (input == 4) {
            TelnetStream.println("Desconectando Telnet...");
            TelnetStream.stop();
            authenticated = false;
            return;
        } else if (input == 26) {
            TelnetStream.println("Entrando en el modo de comandos...");
            inTelnetPrompt = true;
            return;
        }

        if (inTelnetPrompt) {
            handleTelnetPrompt();
            return;
        }

        String command = TelnetStream.readStringUntil('\n');
        command.trim();

        if (!authenticated) {
            if (command == "admin/passw0rd") {
                TelnetStream.println("Login exitoso!");
                authenticated = true;
            } else {
                TelnetStream.println("Acceso denegado.");
            }
            return;
        }

        externalDevice.println(command);
    }

    while (externalDevice.available()) {
        TelnetStream.write(externalDevice.read());
    }
}

void setup() {
    Serial.begin(115200);
    externalDevice.begin(115200, SERIAL_8N1, uartRx, uartTx);

    pinMode(relayPin, OUTPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(redLed, OUTPUT);
    pinMode(blueLed, OUTPUT);
    pinMode(deviceStatusPin, INPUT);
    digitalWrite(relayPin, LOW);
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, LOW);
    digitalWrite(blueLed, LOW);

    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado!");
    blinkLED(blueLed, 10, 200);
    udp.begin(udpPort);
    TelnetStream.begin();

    runner.init();
    runner.addTask(taskCheckWiFi);
    taskCheckWiFi.enable();
}

void loop() {
    checkWakeOnLan();
    handleTelnet();
    runner.execute();
}
