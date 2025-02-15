import board
import digitalio
import time
import wifi
import socketpool
import os
import microcontroller
import ipaddress
import asyncio

# Define GPIO pins
TRANSISTOR_PIN = digitalio.DigitalInOut(board.GP13)  # GPIO pin to control the transistor (Base)
TRANSISTOR_PIN.direction = digitalio.Direction.OUTPUT

DEVICE_STATE_PIN = digitalio.DigitalInOut(board.GP14)  # GPIO pin to detect external device state
DEVICE_STATE_PIN.direction = digitalio.Direction.INPUT

# LED pins
RED_LED = digitalio.DigitalInOut(board.GP5)  # Red LED
RED_LED.direction = digitalio.Direction.OUTPUT

GREEN_LED = digitalio.DigitalInOut(board.GP6)  # Green LED
GREEN_LED.direction = digitalio.Direction.OUTPUT

BLUE_LED = digitalio.DigitalInOut(board.GP7)  # Blue LED
BLUE_LED.direction = digitalio.Direction.OUTPUT

# Function to read settings from settings.ini
def read_settings():
    # Default values for all settings
    default_settings = {
        "TARGET_MAC": "00:00:00:00:00:00",
        "WIFI_SSID": "your-fallback-wifi-ssid",
        "WIFI_PASSWORD": "your-fallback-wifi-password",
        "STATIC_IP": "",
        "STATIC_SUBNET_MASK": "",
        "STATIC_GATEWAY": "",
        "STATIC_DNS": "",
        "WIFI_FREQ_CHECK": "10",
        "WIFI_FAILED_ATTEMPTS": "10",
    }

    settings = default_settings.copy()  # Start with default values

    try:
        with open("settings.ini", "r") as file:
            for line in file:
                line = line.strip()
                # Skip empty lines and comments (lines starting with #)
                if line and not line.startswith("#"):
                    # Split the line into key and value using the first "="
                    if "=" in line:
                        key, value = line.split("=", 1)  # Split on the first "="
                        key = key.strip()
                        value = value.strip()
                        # Update the setting if the key exists in defaults
                        if key in settings:
                            settings[key] = value
    except Exception as e:
        print("Error reading settings.ini:", e)
        # If there's an error, fall back to default values for all settings

    return settings

# Read settings from settings.ini
settings = read_settings()

# Convert TARGET_MAC from string to bytes
TARGET_MAC = bytes.fromhex(settings["TARGET_MAC"].replace(":", ""))

# Wi-Fi credentials
WIFI_SSID = settings["WIFI_SSID"]
WIFI_PASSWORD = settings["WIFI_PASSWORD"]

# Static IP settings (optional)
STATIC_IP = settings["STATIC_IP"]
STATIC_SUBNET_MASK = settings["STATIC_SUBNET_MASK"]
STATIC_GATEWAY = settings["STATIC_GATEWAY"]
STATIC_DNS = settings["STATIC_DNS"]

# WIFI_FREQ_CHECK (socket timeout) and WIFI_FAILED_ATTEMPTS
WIFI_FREQ_CHECK = int(settings["WIFI_FREQ_CHECK"])  # Default to 10 if not defined
WIFI_FAILED_ATTEMPTS = int(settings["WIFI_FAILED_ATTEMPTS"])  # Default to 10 if not defined

# Counter for failed Wi-Fi connection attempts
wifi_failed_attempts = 0

# Function to configure Wi-Fi with static IP or DHCP
def configure_wifi():
    if STATIC_IP and STATIC_SUBNET_MASK and STATIC_GATEWAY:
        try:
            # Convert strings to ipaddress objects
            ip = ipaddress.IPv4Address(STATIC_IP)
            subnet_mask = ipaddress.IPv4Address(STATIC_SUBNET_MASK)
            gateway = ipaddress.IPv4Address(STATIC_GATEWAY)
            dns = ipaddress.IPv4Address(STATIC_DNS) if STATIC_DNS else None

            # Configure static IP
            wifi.radio.set_ipv4_address(ipv4=ip, netmask=subnet_mask, gateway=gateway, ipv4_dns=dns)
            print("Configured static IP:", ip)
        except Exception as e:
            print("Error configuring static IP:", e)
            print("Falling back to DHCP...")
    else:
        print("No static IP settings found. Using DHCP...")

# Function to blink an LED
def blink_led(led, times=1, delay=0.2):
    for _ in range(times):
        led.value = True
        time.sleep(delay)
        led.value = False
        time.sleep(delay)

# Function to simulate pressing the power button
def press_power_button():
    GREEN_LED.value = True  # Turn on the green LED
    TRANSISTOR_PIN.value = True  # Turn on the transistor (simulate button press)
    time.sleep(0.5)  # Hold the button for 0.5 seconds
    TRANSISTOR_PIN.value = False  # Turn off the transistor (simulate button release)
    GREEN_LED.value = False  # Turn off the green LED

# Function to check if the external device is ON
def is_device_on():
    return DEVICE_STATE_PIN.value  # Returns True if 3.3V is detected

# Function to check if the received packet is a valid WoL magic packet
def is_wol_magic_packet(packet, target_mac):
    # WoL magic packet format: 6 bytes of 0xFF followed by 16 repetitions of the target MAC address
    if len(packet) != 102:
        return False
    if packet[:6] != b"\xFF\xFF\xFF\xFF\xFF\xFF":
        return False
    for i in range(16):
        if packet[6 + i * 6:12 + i * 6] != target_mac:
            return False
    return True

# Unified function to handle Wi-Fi connection and reconnection
def manage_wifi_connection():
    global wifi_failed_attempts  # Access the global counter

    if not wifi.radio.connected:
        print("Wi-Fi connection lost. Attempting to connect/reconnect...")
        try:
            # Configure Wi-Fi (static IP or DHCP)
            configure_wifi()

            # Connect to Wi-Fi
            wifi.radio.connect(WIFI_SSID, WIFI_PASSWORD)
            print("Wi-Fi connected:", wifi.radio.ipv4_address)
            blink_led(BLUE_LED, times=10)  # Blink blue LED twice on successful connection
            wifi_failed_attempts = 0  # Reset the counter on successful connection
            return True
        except Exception as e:
            print("Failed to connect to Wi-Fi:", e)
            blink_led(RED_LED, times=2)  # Blink red LED twice on connection failure
            wifi_failed_attempts += 1  # Increment the failed attempts counter
            if wifi_failed_attempts >= WIFI_FAILED_ATTEMPTS:
                print("Too many failed attempts. Rebooting device...")
                blink_led(RED_LED, times=10)
                microcontroller.reset()  # Reboot the device
            return False
    else:
        blink_led(BLUE_LED, times=2)  # Blink blue LED twice if still connected
        return True

async def listen_for_wol():
    pool = socketpool.SocketPool(wifi.radio)
    sock = pool.socket(pool.AF_INET, pool.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 9))  # Listen on port 9 (default WoL port)

    print("Listening for Wake-on-LAN magic packets...")

    while True:
        try:
            buffer = bytearray(1024)

            # Set a short timeout (e.g., 1 second) and use try-except to avoid blocking
            sock.setblocking(False)

            try:
                num_bytes, addr = sock.recvfrom_into(buffer)
                packet = buffer[:num_bytes]

                if is_wol_magic_packet(packet, TARGET_MAC):
                    print("Received WoL magic packet for target MAC:", TARGET_MAC)
                    if not is_device_on():
                        print("Device is OFF. Pressing power button...")
                        press_power_button()
                    else:
                        print("Device is already ON. Ignoring WoL packet.")
            except OSError:
                # No data received, so just continue
                pass

        except Exception as e:
            print("Error receiving WoL packet:", e)

        # Yield control to other tasks
        await asyncio.sleep(1)

async def manage_wifi_connection_async():
    while True:
        # Instead of calling a blocking function, perform steps in async manner
        if not wifi.radio.connected:
            print("Wi-Fi connection lost. Attempting to reconnect...")
            try:
                configure_wifi()  # Static IP or DHCP (not blocking)
                
                # Use asyncio.sleep to yield control
                await asyncio.sleep(1)

                wifi.radio.connect(WIFI_SSID, WIFI_PASSWORD)  # This might still block, but will at least delay retries
                print("Wi-Fi connected:", wifi.radio.ipv4_address)
                blink_led(BLUE_LED, times=2)
            except Exception as e:
                print("Failed to connect to Wi-Fi:", e)
                blink_led(RED_LED, times=2)
                await asyncio.sleep(1)  # Wait before retrying
        
        blink_led(BLUE_LED, times=2)
        await asyncio.sleep(WIFI_FREQ_CHECK)  # Keep checking periodically

# Main program
if __name__ == "__main__":
    while True:
        if manage_wifi_connection():  # Initial Wi-Fi connection
            loop = asyncio.get_event_loop()
            loop.create_task(listen_for_wol())
            loop.create_task(manage_wifi_connection_async())
            loop.run_forever()
        else:
            print("Raspberry Pi Pico W is unable to connected to Wi-Fi")
