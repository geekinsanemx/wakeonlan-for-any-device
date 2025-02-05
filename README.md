# Raspberry Pi Pico W Wake-on-LAN (WoL) Controller

I started this project due I picked up a ReadyNAS system and managed to install Debian 12 on it, but it looks like there is an existing bug causing the NIC to completely shut down after power off, making me unable to power on remotely once it is already off.

had some time thinking about making a device that can power on another non-smart device in a "smart" way and immediately thought of Wake On Lan, but it can be easily modified by any other way (for example, <ip>/poweron); it depends on your needs. Wake On Lan is easy to implement in any smart device like Alexa, https://www.wolskill.com/ so you can turn on any not-smart device using a combination of this one.

![image](https://github.com/user-attachments/assets/a11c19eb-2fe2-43be-bb2b-4bba3a75905b)
![image](https://github.com/user-attachments/assets/79c3de8e-bb15-4e1f-923c-83ee1974432c)



This project uses a **Raspberry Pi Pico W** to listen for **Wake-on-LAN (WoL) magic packets** and simulate pressing the power button of an external device. The Pico W is powered independently, and the project includes three LEDs for status indication:
- **Red LED**: Indicates Wi-Fi connection issues.
- **Green LED**: Turns on when the power button is pressed.
- **Blue LED**: Indicates successful Wi-Fi connection.

The Pico W also verifies the Wi-Fi connection every 10 seconds and attempts to reconnect if the connection is lost. Additionally, it uses a **device state detection wire** to check if the external device is already ON and ignores WoL requests if the device is already powered.

---

## Features
- **Wake-on-LAN Listener**: Listens for WoL magic packets on UDP port 9.
- **Power Button Simulation**: Simulates pressing the power button of an external device using a transistor.
- **Device State Detection**: Checks if the external device is already ON and ignores WoL requests if the device is powered.
- **Wi-Fi Connection Verification**: Checks the Wi-Fi connection every 10 seconds and reconnects if necessary.
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
2. Update your `TARGET_MAC` to define to which mac_address magic package will be directed
3. Update your `WIFI_SSID` & `WIFI_PASSWORD` for your wireless network connection

### 4. Connect the Hardware
1. Connect the components as shown in the wiring diagram.
2. Ensure the Raspberry Pi Pico W is powered independently.

---

## Usage
1. Power on the Raspberry Pi Pico W.
2. If the Wi-Fi credentials are correct and the network is available, the blue LED will blink twice, and the device will start listening for WoL packets.
3. If the Wi-Fi credentials are incorrect or the network is unavailable, the red LED will blink twice.
4. Every 10 seconds, the script will verify the Wi-Fi connection and attempt to reconnect if necessary.
5. When a valid WoL magic packet is received:
   - If the external device is OFF, the green LED will turn on, and the power button will be pressed.
   - If the external device is already ON, the WoL request will be ignored.

---

## Code Overview
The script performs the following tasks:
- Connects to the specified Wi-Fi network.
- Listens for WoL magic packets on UDP port 9.
- Checks the external device's state using the **device state detection wire**.
- Simulates pressing the power button if the external device is OFF.
- Verifies the Wi-Fi connection every 10 seconds and reconnects if necessary.
- Controls the status LEDs to indicate the system state.

---

## Features to Add
- UART TX/RX TTL connection to console connection with the device if supported
  - possible terminal interaction through web page device_ip/terminal for supported devices
- device_ip/state status page with useful information
- device/poweron & device/poweroff to power on/off device, including x-header authentication
- send telemetry data for monitoring purposes

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
