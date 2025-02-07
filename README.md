# Raspberry Pi Pico W Wake-on-LAN (WoL) Controller

I started this project due I picked up a ReadyNAS system and managed to install Debian 12 on it, but it looks like there is an existing bug causing the NIC to completely shut down after power off, making me unable to power on remotely once it is already off.

had some time thinking about making a device that can power on another non-smart device in a "smart" way and immediately thought of Wake On Lan, but it can be easily modified by any other way (for example, <ip>/poweron); it depends on your needs. Wake On Lan is easy to implement in any smart device like Alexa, https://www.wolskill.com/ so you can turn on any not-smart device using a combination of this one.

![image](https://github.com/user-attachments/assets/a11c19eb-2fe2-43be-bb2b-4bba3a75905b)
![image](https://github.com/user-attachments/assets/79c3de8e-bb15-4e1f-923c-83ee1974432c)

This project uses a **Raspberry Pi Pico W** to listen for **Wake-on-LAN (WoL) magic packets** and simulate pressing the power button of an external device. The Pico W is powered independently, and the project includes three LEDs for status indication:
- **Red LED**: Indicates Wi-Fi connection issues.
- **Green LED**: Turns on when the power button is pressed.
- **Blue LED**: Indicates successful Wi-Fi connection.

The Pico W also verifies the Wi-Fi connection every 10 seconds (configurable) and attempts to reconnect if the connection is lost. Additionally, it uses a **device state detection wire** to check if the external device is already ON and ignores WoL requests if the device is already powered.

---

## Features
- **Wake-on-LAN Listener**: Listens for WoL magic packets on UDP port 9.
- **Power Button Simulation**: Simulates pressing the power button of an external device using a transistor.
- **Device State Detection**: Checks if the external device is already ON and ignores WoL requests if the device is powered.
- **Wi-Fi Connection Verification**: Checks the Wi-Fi connection periodically (configurable) and reconnects if necessary.
- **Static IP Configuration**: Supports static IP configuration for the Pico W (optional, falls back to DHCP if not configured).
- **Reboot on Wi-Fi Failure**: Automatically reboots the Pico W after a configurable number of failed Wi-Fi connection attempts.
- **Status LEDs**:
  - **Red LED**: Blinks twice if Wi-Fi connection validation fails.
  - **Green LED**: Turns on when the power button is pressed.
  - **Blue LED**: Blinks twice if Wi-Fi connection is successfully verified.

---

## Hardware Requirements
- **Raspberry Pi Pico W**
- **NPN Transistor** (e.g., 2N2222 or BC547)
- **Resistors**:
  - 1kΩ resistor (for transistor base)
  - 10kΩ resistor (for power device state)
  - 220Ω resistor (common ground for LEDs)
- **LEDs**:
  - Red LED (GP5)
  - Green LED (GP6)
  - Blue LED (GP7)
- **External Device** with a power button
- **Jumper Wires**

---

## Wiring Diagram
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

* Device State Detection Wire: is a device wire which usually powered meanwhile device is ON, keep 0v when device is OFF.
```
---


---

## Software Requirements
- **CircuitPython** installed on the Raspberry Pi Pico W.

---

## Setup Instructions

### 1. Install CircuitPython
1. Download the latest CircuitPython firmware for the Raspberry Pi Pico W from the [official website](https://circuitpython.org/board/raspberry_pi_pico_w/).
2. Flash the firmware onto the Pico W.

### 2. Install Required Libraries
1. Download the [CircuitPython Library Bundle](https://circuitpython.org/libraries).

### 3. Upload the Code
1. Save the provided CircuitPython script as `code.py` on the Raspberry Pi Pico W.
2. Create a `settings.ini` file with the following content (update the values as needed):

```
# Settings for Wake-on-LAN and Wi-Fi
TARGET_MAC = a1:b2:c3:d4:e5:f6
WIFI_SSID = your-wifi-ssid
WIFI_PASSWORD = your-wifi-password

# Wi-Fi frequency check (socket timeout) and failed attempts
WIFI_FREQ_CHECK = 10
WIFI_FAILED_ATTEMPTS = 10

# Static IP settings (optional)
STATIC_IP =
STATIC_SUBNET_MASK =
STATIC_GATEWAY =
STATIC_DNS =
```

### 4. Connect the Hardware
1. Connect the components as shown in the wiring diagram.
2. Ensure the Raspberry Pi Pico W is powered independently.

---

## Power on the Raspberry Pi Pico W.

1. Power on the Raspberry Pi Pico W.
2. If the Wi-Fi credentials are correct and the network is available, the blue LED will blink twice, and the device will start listening for WoL packets.
3. If the Wi-Fi credentials are incorrect or the network is unavailable, the red LED will blink twice.
4. Every `WIFI_FREQ_CHECK` seconds (default: 10 seconds), the script will verify the Wi-Fi connection and attempt to reconnect if necessary.
5. When a valid WoL magic packet is received:
  - If the external device is OFF, the green LED will turn on, and the power button will be pressed.
  - If the external device is already ON, the WoL request will be ignored.

---

## Code Overview
The script performs the following tasks:
- Connects to the specified Wi-Fi network (supports static IP or DHCP).
- Listens for WoL magic packets on UDP port 9.
- Checks the external device's state using the device state detection wire.
- Simulates pressing the power button if the external device is OFF.
- Verifies the Wi-Fi connection periodically and reconnects if necessary.
- Automatically reboots the Pico W after `WIFI_FAILED_ATTEMPTS` (default: 10) failed Wi-Fi connection attempts.
- Controls the status LEDs to indicate the system state.

---

## Features to Add
- **UART TX/RX TTL Connection**: Console interaction with the device if supported.
- **Web Interface**:
  - Terminal interaction through a web page (e.g., `device_ip/terminal`).
  - Device status page (e.g., `device_ip/state`).
  - Power on/off functionality (e.g., `device_ip/poweron` and `device_ip/poweroff`) with authentication.
- **Telemetry Data**: Send telemetry data for monitoring purposes.
- **Dynamic Configuration**: Allow updating settings (e.g., Wi-Fi credentials, static IP) via a web interface or API.
- **OTA Updates**: Enable over-the-air (OTA) firmware updates for the Pico W.
- **Multi-Device Support**: Extend the project to support multiple devices with unique MAC addresses.
- **Energy Monitoring**: Add energy monitoring capabilities to track power consumption of the external device.
- **Integration with Smart Home Systems**: Add support for integration with platforms like Home Assistant, Alexa, or Google Home.
- **Enhanced Security**: Implement HTTPS for secure communication and add user authentication for web interfaces.
- **Customizable LED Patterns**: Allow users to customize LED patterns for different system states.
- **Logging and Analytics**: Add logging functionality to track system events and analyze performance.

---

## License
This project is licensed under the **GNU GENERAL PUBLIC LICENSE**. See the [LICENSE](LICENSE) file for details.

---

## Acknowledgments
- Inspired by Wake-on-LAN functionality and Raspberry Pi Pico W capabilities.
- Thanks to the CircuitPython community for providing excellent libraries and documentation. [Adafruit CircuitPython](https://circuitpython.org/)
---

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

---

## **Contact**
For questions or feedback, please open an issue on the [GitHub repository](https://github.com/geekinsanemx/wakeonlan-for-any-device).

---
