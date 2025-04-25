# ESP32 Wake-on-LAN (WoL) Controller with Telnet Server

This project provides a solution to remotely power on and manage a non-smart device using an ESP32. The device listens for WoL magic packets and includes a Telnet server for remote management. It can simulate pressing the power button of an external device and provides status monitoring.

## Features

- **Wake-on-LAN Listener**: Listens for WoL magic packets on UDP port 9
- **Power Button Simulation**: Simulates pressing the power button via relay control
- **Device Status Monitoring**: Detects if the external device is ON/OFF via status pin
- **Telnet Server**: Provides remote command interface on port 23
- **UART Console**: Direct serial access to connected device via hardware UART pins
- **WiFi Management**: Automatic reconnection and system reboot on persistent failures
- **Visual Feedback**: Multiple LEDs for status indication (onboard + RGB LEDs)

## Hardware Requirements

- ESP32 Dev Module
- NPN Transistor (e.g., 2N2222 or BC547)
- Resistors:
  - 1kΩ resistor (for transistor base)
  - 10kΩ resistor (for power device state)
  - 220Ω resistors for LEDs
- LEDs:
  - Green LED (GP21)
  - Red LED (GP22)
  - Blue LED (GP23)
- 2N2222 Transistor
- External Device with power button
- Jumper wires

## Wiring Diagram

ESP32 Dev Module
```
+3.3V ----------------------------------- 3V3
GND ------------------------------------- GND

GPIO18 --------------------------------[2N2222 Transistor]
GPIO19 -------------[10Ω]--------------[Device Status Input (3.3V when ON)]

GPIO21 ---------------------------------[Green LED]
GPIO22 ---------------------------------[Red LED]
GPIO23 ---------------------------------[Blue LED]
[Common Ground for LEDs] ---------------[220Ω]---- GND

GPIO16 (UART RX) -----------------------[To Device TX]
GPIO17 (UART TX) -----------------------[To Device RX]
```

2N2222 Transistor Wiring
```
                                        |---[Collector (C)]---[Wire 1 of Power Button / 3.3v / Powered side ]
   GPIO18 ------[1kΩ]------[(2N2222)]-----|---[Base (B)]
                                        |---[Emitter (E)]-----[Wire 2 of Power Button / GND side)
```

## Telnet Commands

Connect to the ESP32 via Telnet (port 23) to access these commands:
- `status` - Show system status (WiFi, IP, MAC, device state)
- `reset` - Restart the ESP32
- `console` - Enter UART console mode with connected device
- `poweron` - Turn device on (if off)
- `poweroff` - Turn device off (if on)
- `poweron -c` - Turn device on and enter console mode
- `poweroff -f` - Force power off (long press)
- `exit` - Disconnect from Telnet session

## UART Console Access

The ESP32 provides direct UART pass-through to the connected device:
- **UART Pins**: GPIO16 (RX), GPIO17 (TX)
- **Baud Rate**: 115200
- **Access Methods**:
  1. Via Telnet `console` command
  2. Direct serial connection to ESP32's UART pins
- **Exit Console**: Press Ctrl+Q (ASCII 17)

## Setup Instructions

1. Update the code with your WiFi credentials:
```
   #define WIFI_SSID "YOUR_SSID"
   #define WIFI_PASS "YOUR_PASSWORD"
```

Configure the target MAC address:
```
uint8_t targetMAC[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
```

2. Upload the code to your ESP32 using Arduino IDE or PlatformIO

3. Connect the hardware as per wiring diagram

4. Power on the system - the onboard LED will blink when WiFi connects
- Operation When powered:
- Blue LED indicates WiFi status
- Onboard LED (GP2) blinks during WiFi connection
- Red/Green LEDs show device power state

5. To wake device:
- Send WoL magic packet to UDP port 9
- OR use Telnet poweron command
- For direct console access:
- Connect to device UART via GPIO16/17
- OR use Telnet console command

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE. See the LICENSE file for details.

## Acknowledgments
Inspired by Wake-on-LAN functionality and ESP32 capabilities

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

## Contact
For questions or feedback, please open an issue on the GitHub repository.
