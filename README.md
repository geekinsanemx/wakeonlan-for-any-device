# Wake-on-LAN Controller for Any Device

Multi-platform Wake-on-LAN (WoL) solution to remotely power on and manage non-smart devices. Supports ESP32 variants with Telnet server and Raspberry Pi Pico W with CircuitPython implementations.

<a href="https://github.com/user-attachments/assets/76ba4802-1771-4245-a3b3-d3f26970409a">
  <img src="https://github.com/user-attachments/assets/76ba4802-1771-4245-a3b3-d3f26970409a" width="250" alt="image">
</a>

<a href="https://github.com/user-attachments/assets/68440759-e6fd-442c-b813-6fcb433a4477">
  <img src="https://github.com/user-attachments/assets/68440759-e6fd-442c-b813-6fcb433a4477" width="250" alt="IMG_7401">
</a>

## Supported Hardware Platforms

| Platform | Language | Status | Key Features |
|----------|----------|--------|--------------|
| **ESP32 Dev Module** | Arduino C++ | Production | Telnet + UART console, RGB LEDs, FreeRTOS multitasking |
| **ESP32-S3-Zero-Mini** | Arduino C++ | Stable (v1.0.8) | Compact, NeoPixel RGB, double-press poweroff, improved console |
| **Raspberry Pi Pico W** | CircuitPython | Alternative | External config file, async variants, lower cost |

## Core Features

- **Wake-on-LAN Listener**: Monitors UDP port 9 for magic packets
- **Power Button Simulation**: Relay/transistor-based power button control
- **Device Status Monitoring**: Detects external device ON/OFF state via GPIO input
- **Telnet Server**: Remote command interface on port 23 (ESP32 platforms)
- **UART Console Pass-through**: Direct serial access to connected device
- **WiFi Auto-reconnection**: Automatic recovery with system reboot on persistent failures
- **Visual Feedback**: LED status indicators (RGB or NeoPixel)

---

## ESP32 Dev Module (Primary Implementation)

### Hardware Requirements
- ESP32 Dev Module (30-pin)
- NPN Transistor (2N2222 or BC547)
- Resistors: 1kΩ (transistor base), 10kΩ (status pull-down), 220Ω × 3 (LEDs)
- LEDs: Separate Red, Green, Blue (5mm standard)
- Jumper wires

### GPIO Configuration
```
GPIO18  - Transistor control (power button simulation)
GPIO19  - Device status input (3.3V when device ON)
GPIO21  - Green LED
GPIO22  - Red LED
GPIO23  - Blue LED
GPIO2   - Onboard LED (WiFi connection indicator)
GPIO16  - UART RX (to device TX)
GPIO17  - UART TX (to device RX)
```

### Wiring Diagram
```
ESP32 Dev Module
+3.3V ----------------------------------- 3V3
GND ------------------------------------- GND

GPIO18 --------------------------------[2N2222 Transistor]
GPIO19 -------------[10kΩ]-------------[Device Status Input (3.3V when ON)]

GPIO21 ---------[220Ω]----------------[Green LED]--[GND]
GPIO22 ---------[220Ω]----------------[Red LED]----[GND]
GPIO23 ---------[220Ω]----------------[Blue LED]---[GND]

GPIO16 (UART RX) ----------------------[To Device TX]
GPIO17 (UART TX) ----------------------[To Device RX]
```

### 2N2222 Transistor Wiring
```
                                    |---[Collector (C)]---[Power Button Wire 1 / 3.3V side]
GPIO18 ------[1kΩ]------[(2N2222)]-----|---[Base (B)]
                                    |---[Emitter (E)]-----[Power Button Wire 2 / GND side]
```

### Configuration
Edit `esp32_dev_module/TelnetCommandServer.ino`:
```cpp
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"
uint8_t targetMAC[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};  // Your device MAC
```

---

## ESP32-S3-Zero-Mini (Compact Alternative)

### Improvements Over Dev Module
- **Smaller form factor** - Compact S3 board with USB-C
- **Single NeoPixel RGB LED** - Replaces 3 separate LEDs, color-coded status
- **Double-press poweroff** - More reliable power-down sequence (2× 500ms pulses)
- **Enhanced console escape** - Ctrl+Q only (v1.0.8), all other chars pass through
- **ESP-IDF 5.x compatible** - Updated socket handling and watchdog management
- **Monitoring MAC display** - Shows target MAC in status command

### Hardware Requirements
- ESP32-S3-Zero-Mini board
- NPN Transistor (2N2222 or BC547)
- Resistors: 1kΩ (transistor base), 10kΩ (status pull-down)
- **No separate LEDs required** (uses onboard NeoPixel)
- Jumper wires

### GPIO Configuration
```
GPIO3   - Power control (transistor base)
GPIO2   - Device status input (3.3V when device ON)
GPIO21  - NeoPixel RGB LED (WS2812B)
GPIO7   - UART TX (to device RX)
GPIO8   - UART RX (from device TX)
```

### NeoPixel Color Coding
- **Blue**: WiFi status checking
- **Green**: Power button activation
- **Red**: Error / WiFi connection failure

### Wiring Diagram
```
ESP32-S3-Zero-Mini
+3.3V ----------------------------------- 3V3
GND ------------------------------------- GND

GPIO3  --------------------------------[2N2222 Transistor]
GPIO2  -------------[10kΩ]-------------[Device Status Input (3.3V when ON)]

GPIO21 --------------------------------[NeoPixel RGB LED (built-in or external WS2812B)]

GPIO7 (UART TX) -----------------------[To Device RX]
GPIO8 (UART RX) -----------------------[From Device TX]
```

### Configuration
Edit `ESP32-S3-Zero-Mini/ESP32-S3-Zero-Mini.ino`:
```cpp
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"
uint8_t targetMAC[] = {0x28, 0xc6, 0x8e, 0xd5, 0xc8, 0xdc};  // Your device MAC
#define NEOPIXEL_BRIGHTNESS 60  // Adjust 0-255
```

### Version History (ESP32-S3-Zero-Mini)
- **v1.0.8**: Reverted to simple Ctrl+Q escape only, all other chars pass through
- **v1.0.7**: Fixed console escape (Ctrl+], Ctrl+D exit cleanly; Ctrl+C passes through)
- **v1.0.6**: Added multiple console exit methods (Ctrl+Q, Ctrl+D, ~~~)
- **v1.0.5**: Fixed UART pins (RX=7, TX=8), poweroff now double press
- **v1.0.4**: Fixed NeoPixel colors (WiFi=BLUE, Power=GREEN, Error=RED)
- **v1.0.3**: Added monitoring MAC to status, changed MAC format to colons
- **v1.0.2**: Removed manual watchdog init (Arduino core auto-manages)
- **v1.0.1**: Fixed ESP-IDF 5.x compatibility (esp_task_wdt_init, flush, accept)
- **v1.0.0**: Initial ESP32-S3 Zero Mini adaptation with NeoPixel support

---

## Raspberry Pi Pico W (CircuitPython Alternatives)

### Available Variants
1. **code_v1.py** - Simple synchronous implementation
2. **code_v2_async.py** - Asynchronous with better CPU efficiency
3. **code_v2_telnet-ro.py** - Async with Telnet server + UART console (read-only)

### Hardware Requirements
- Raspberry Pi Pico W
- NPN Transistor (2N2222 or BC547)
- Resistors: 1kΩ (transistor base), 10kΩ (status pull-down), 220Ω × 3 (LEDs)
- LEDs: Separate Red, Green, Blue
- Jumper wires

### GPIO Configuration
```
GP13  - Transistor control
GP14  - Device status input
GP5   - Red LED
GP6   - Green LED
GP7   - Blue LED
GP0   - UART TX (telnet-ro variant only)
GP1   - UART RX (telnet-ro variant only)
```

### Configuration
Edit `rpi_pico_w/settings.ini`:
```ini
TARGET_MAC = 00:01:02:03:04:05
WIFI_SSID = YOUR_SSID
WIFI_PASSWORD = YOUR_PASSWORD
WIFI_FREQ_CHECK = 10
WIFI_FAILED_ATTEMPTS = 10
STATIC_IP =                    # Leave empty for DHCP
STATIC_SUBNET_MASK =
STATIC_GATEWAY =
STATIC_DNS =
```

---

## Telnet Commands (ESP32 Platforms Only)

Connect via `telnet <device-ip> 23`:

| Command | Description |
|---------|-------------|
| `status` | Show WiFi SSID, IP, MAC, device state, monitoring MAC |
| `reset` | Restart ESP32 |
| `console` | Enter UART console mode (Ctrl+Q to exit) |
| `poweron` | Turn device on (500ms press) |
| `poweroff` | Turn device off (500ms press, S3: double-press) |
| `poweron -c` | Power on and enter console mode |
| `poweroff -f` | Force power off (5000ms long press) |
| `exit` | Disconnect from Telnet session |

---

## UART Console Access

### ESP32 Dev Module
- **UART Pins**: GPIO16 (RX), GPIO17 (TX)
- **Baud Rate**: 115200
- **Exit**: Ctrl+Q

### ESP32-S3-Zero-Mini
- **UART Pins**: GPIO8 (RX), GPIO7 (TX)
- **Baud Rate**: 115200
- **Exit**: Ctrl+Q (v1.0.8: passes all other chars including Ctrl+C)

### Raspberry Pi Pico W (telnet-ro variant)
- **UART Pins**: GP0 (TX), GP1 (RX)
- **Baud Rate**: 115200
- **Access**: Telnet read-only forwarding

---

## Setup Instructions

### 1. Choose Your Platform
- **ESP32 Dev Module** - Most features, full Telnet + UART, standard form factor
- **ESP32-S3-Zero-Mini** - Compact, refined features, single NeoPixel LED
- **Raspberry Pi Pico W** - External configuration, CircuitPython, budget option

### 2. Configure WiFi and Target MAC
- **ESP32 platforms**: Edit `#define` statements in .ino file
- **Pico W**: Edit `settings.ini` file

### 3. Upload Code
- **ESP32**: Use Arduino IDE or PlatformIO
- **Pico W**: Copy .py files to CIRCUITPY drive

### 4. Wire Hardware
- Follow platform-specific wiring diagram above
- Verify transistor pinout (flat side = BCE for 2N2222)
- Use 10kΩ pull-down resistor on status input

### 5. Power On and Test
- **WiFi Connection**: Onboard LED blinks (ESP32) or status LEDs indicate connection
- **Wake Device**: Send WoL magic packet to UDP port 9 OR use Telnet `poweron`
- **Console Access**: Use Telnet `console` command (ESP32 only)

---

## LED Status Indicators

### ESP32 Dev Module
- **Blue LED**: WiFi status checking
- **Red/Green LEDs**: Device power state
- **Onboard LED (GPIO2)**: Blinks during WiFi connection

### ESP32-S3-Zero-Mini (NeoPixel)
- **Blue**: WiFi status checking
- **Green**: Power button activation
- **Red**: Error / WiFi connection failure

### Raspberry Pi Pico W
- **Blue/Green LEDs**: WiFi and device status
- **Red LED**: Data transmission (telnet-ro variant)

---

## Architecture Notes

### Multi-threading (ESP32 Platforms)
Both ESP32 implementations use FreeRTOS tasks for concurrent operations:
- **Task 1 (Core 0)**: WiFi connection monitoring (15-second intervals)
- **Task 2 (Core 0)**: WoL packet listener (10ms polling)
- **Task 3 (Core 1)**: Telnet server handler

### WiFi Recovery Strategy
- **ESP32**: 10 reconnection attempts, then system restart
- **Pico W**: 10 failed attempts with exponential backoff, then reboot

### Watchdog Timer
- **ESP32 Dev Module**: Manual 30-second watchdog with reset during console mode
- **ESP32-S3**: Auto-managed by Arduino core (ESP-IDF 5.x)
- **Pico W**: No hardware watchdog (relies on software reset)

---

## 3D Printable Case

**Available for ESP32 Dev Module:**
- File: `esp32_dev_module/3d_print/ESP32 DevKit Case v1.stl`
- Enclosure for complete assembled hardware

---

## Project Structure

```
wakeonlan-for-any-device/
├── esp32_dev_module/
│   ├── TelnetCommandServer.ino         # ESP32 DevKit implementation
│   └── 3d_print/
│       └── ESP32 DevKit Case v1.stl
├── ESP32-S3-Zero-Mini/
│   └── ESP32-S3-Zero-Mini.ino          # ESP32-S3 compact variant (v1.0.8)
├── rpi_pico_w/
│   ├── code_v1.py                      # Synchronous Pico W
│   ├── code_v2_async.py                # Async Pico W
│   ├── code_v2_telnet-ro.py            # Async with Telnet + UART
│   └── settings.ini                    # External configuration
├── README.md
├── LICENSE
└── CLAUDE.md
```

---

## License

This project is licensed under the **GNU General Public License v3.0**. See LICENSE file for details.

---

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Test on target hardware
4. Submit pull request with detailed description

---

## Acknowledgments

- Inspired by Wake-on-LAN protocol (AMD Magic Packet)
- Built on ESP32/ESP-IDF and CircuitPython ecosystems
- Community feedback for console escape improvements

---

## Contact

For questions, issues, or feature requests:
- Open an issue on the GitHub repository
- Repository: https://github.com/geekinsanemx/wakeonlan-for-any-device

---

## Troubleshooting

### Common Issues

**WiFi won't connect:**
- Verify SSID/password in code or settings.ini
- Check 2.4GHz network (ESP32/Pico W don't support 5GHz)
- Monitor serial console at 115200 baud for connection logs

**WoL magic packet not working:**
- Confirm target MAC address matches device exactly
- Verify UDP port 9 not blocked by firewall
- Check device supports Wake-on-LAN (BIOS/UEFI setting)
- Test with `wakeonlan` CLI tool: `wakeonlan -i <broadcast-ip> <MAC>`

**Telnet connection refused:**
- Ensure ESP32 WiFi connected (check serial console)
- Verify port 23 not blocked by firewall
- Test with: `telnet <esp32-ip> 23`

**Console mode garbled text (ESP32-S3):**
- Verify UART baud rate matches device (default: 115200)
- Check TX/RX not swapped (GPIO7=TX, GPIO8=RX)
- Ensure common ground between ESP32-S3 and target device

**Power button simulation not working:**
- Verify transistor wiring (Base ← GPIO via 1kΩ, Collector ↔ Button, Emitter → GND)
- Test transistor with multimeter (should conduct when GPIO HIGH)
- Check power button wires correctly identified (may need to swap)

**Status detection always shows OFF:**
- Verify 10kΩ pull-down resistor installed on status GPIO
- Check target device provides 3.3V when ON (use multimeter)
- Ensure GPIO configured as INPUT (code default)
