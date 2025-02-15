# Raspberry Pi Pico W Wake-on-LAN (WoL) Controller

I started this project due I picked up a ReadyNAS system and managed to install Debian 12 on it, but it looks like there is an existing bug causing the NIC to completely shut down after power off, making me unable to power on remotely once it is already off.

had some time thinking about making a device that can power on another non-smart device in a "smart" way and immediately thought of Wake On Lan, but it can be easily modified by any other way (for example, <ip>/poweron); it depends on your needs. Wake On Lan is easy to implement in any smart device like Alexa, https://www.wolskill.com/ so you can turn on any not-smart device using a combination of this one.

![image](https://github.com/user-attachments/assets/a11c19eb-2fe2-43be-bb2b-4bba3a75905b)
![image](https://github.com/user-attachments/assets/79c3de8e-bb15-4e1f-923c-83ee1974432c)

# Wake-on-LAN (WoL) Controller for Any Device

This project provides a solution to remotely power on a non-smart device using Wake-on-LAN (WoL). It can be implemented using either a Raspberry Pi Pico W or an ESP32 Dev Module. Both boards listen for WoL magic packets and simulate pressing the power button of an external device. The project includes status LEDs for visual feedback and supports Wi-Fi connection verification, device state detection, and automatic reconnection.

## Features

- **Wake-on-LAN Listener**: Listens for WoL magic packets on UDP port 9.
- **Power Button Simulation**: Simulates pressing the power button of an external device using a transistor.
- **Device State Detection**: Checks if the external device is already ON and ignores WoL requests if the device is powered.
- **Wi-Fi Connection Verification**: Checks the Wi-Fi connection periodically (configurable) and reconnects if necessary.
- **Static IP Configuration**: Supports static IP configuration (optional, falls back to DHCP if not configured).
- **Reboot on Wi-Fi Failure**: Automatically reboots the device after a configurable number of failed Wi-Fi connection attempts.
- **Status LEDs**:
  - **Red LED**: Indicates Wi-Fi connection issues.
  - **Green LED**: Turns on when the power button is pressed.
  - **Blue LED**: Indicates successful Wi-Fi connection.

## Hardware Requirements

### For Raspberry Pi Pico W:
- Raspberry Pi Pico W
- NPN Transistor (e.g., 2N2222 or BC547)
- Resistors:
  - 1kΩ resistor (for transistor base)
  - 10kΩ resistor (for power device state)
  - 220Ω resistor (common ground for LEDs)
- LEDs:
  - Red LED (GP5)
  - Green LED (GP6)
  - Blue LED (GP7)
- External Device with a power button
- Jumper Wires

### For ESP32 Dev Module:
- ESP32 Dev Module
- NPN Transistor (e.g., 2N2222 or BC547)
- Resistors:
  - 1kΩ resistor (for transistor base)
  - 10kΩ resistor (for power device state)
  - 220Ω resistor (common ground for LEDs)
- LEDs:
  - Red LED (GP19)
  - Green LED (GP18)
  - Blue LED (GP21)
- External Device with a power button
- Jumper Wires

## Wiring Diagram

### Raspberry Pi Pico W
```
Raspberry Pi Pico W (Powered Independently)
   +3.3V (3V3) ----------------------------------- 3V3 (Raspberry Pi Pico W)
   GND (Ground) ---------------------------------- GND (Raspberry Pi Pico W)

   GP5 ---------------------------------| Red LED (Anode)
                                        |
   GP6 ---------------------------------| Green LED (Anode)
                                        |
   GP7 ---------------------------------| Blue LED (Anode)
                                        |
   [Common Ground for LEDs] ------------|----[220Ω]---- GND

                                        |---[Collector (C)]---[Wire 1 of Power Button / 3.3v / Powered side ]
   GP13 ------[1kΩ]------[(2N2222)]-----|---[Base (B)]
                                        |---[Emitter (E)]-----[Wire 2 of Power Button / GND side)

   GP14 ------[10kΩ]--------------------| Device State Detection Wire [3.3v]
```

### ESP32 Dev Module
```
ESP32 Dev Module (Powered Independently)
   +3.3V (3V3) ----------------------------------- 3V3 (ESP32 Dev Module)
   GND (Ground) ---------------------------------- GND (ESP32 Dev Module)

   GP19 ---------------------------------| Red LED (Anode)
                                         |
   GP18 ---------------------------------| Green LED (Anode)
                                         |
   GP21 ---------------------------------| Blue LED (Anode)
                                         |
   [Common Ground for LEDs] -------------|----[220Ω]---- GND

                                         |---[Collector (C)]---[Wire 1 of Power Button / 3.3v / Powered side ]
   GP5 --------[1kΩ]------[(2N2222)]-----|---[Base (B)]
                                         |---[Emitter (E)]-----[Wire 2 of Power Button / GND side)

   GP22 --------[10kΩ]-------------------| Device State Detection Wire [3.3v]
```

## Software Requirements

### For Raspberry Pi Pico W:
- CircuitPython installed on the Raspberry Pi Pico W.
- CircuitPython Library Bundle for required libraries.

### For ESP32 Dev Module:
- Arduino IDE or PlatformIO for ESP32 development.
- WiFi.h and WiFiUdp.h libraries for Wi-Fi and UDP functionality.

## Setup Instructions

### For Raspberry Pi Pico W:
1. **Install CircuitPython**
2. **Install Required Libraries**
3. **Upload the Code**
4. **Connect the Hardware**

### For ESP32 Dev Module:
1. **Install Arduino IDE or PlatformIO**
2. **Upload the Code**
3. **Connect the Hardware**

## Power On and Operation
1. Power on the device.
2. If the Wi-Fi credentials are correct, the blue LED blinks twice and starts listening for WoL packets.
3. If Wi-Fi fails, the red LED blinks twice.
4. If a WoL packet is received:
   - If the external device is OFF, the green LED turns on and the power button is pressed.
   - If the device is ON, the WoL request is ignored.

## Features to Add
- **UART TX/RX TTL Connection**
- **Web Interface**
- **Telemetry Data**
- **Dynamic Configuration**
- **OTA Updates**
- **Multi-Device Support**
- **Energy Monitoring**
- **Integration with Smart Home Systems**
- **Enhanced Security**
- **Customizable LED Patterns**
- **Logging and Analytics**

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE. See the LICENSE file for details.

## Acknowledgments
Inspired by Wake-on-LAN functionality and the capabilities of the Raspberry Pi Pico W and ESP32 Dev Module.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

## Contact
For questions or feedback, please open an issue on the GitHub repository.
