import board
import digitalio
import time
import wifi
import socketpool
import os

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

# Replace with the MAC address of the device you want to wake up
TARGET_MAC = b"\x00\x00\x00\x00\x00\x00"  # Target MAC address: 00:00:00:00:00:00

# Wi-Fi credentials
WIFI_SSID = "your-wifi-ssid"
WIFI_PASSWORD = "your-wifi-password"

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

# Function to verify Wi-Fi connection and reconnect if necessary
def verify_and_reconnect_wifi():
    if not wifi.radio.connected:
        print("Wi-Fi connection lost. Attempting to reconnect...")
        try:
            wifi.radio.connect(WIFI_SSID, WIFI_PASSWORD)
            print("Wi-Fi reconnected:", wifi.radio.ipv4_address)
            blink_led(BLUE_LED, times=2)  # Blink blue LED twice on successful reconnection
            return True
        except Exception as e:
            print("Failed to reconnect to Wi-Fi:", e)
            blink_led(RED_LED, times=2)  # Blink red LED twice on reconnection failure
            return False
    else:
        blink_led(BLUE_LED, times=2)  # Blink blue LED twice if still connected
        return True

# Connect to Wi-Fi
def connect_wifi():
    print("Connecting to Wi-Fi...")
    try:
        wifi.radio.connect(WIFI_SSID, WIFI_PASSWORD)
        print("Wi-Fi connected:", wifi.radio.ipv4_address)
        blink_led(BLUE_LED, times=2)  # Blink blue LED twice on successful connection
        return True
    except Exception as e:
        print("Failed to connect to Wi-Fi:", e)
        blink_led(RED_LED, times=2)  # Blink red LED twice on connection failure
        return False

# Main function to listen for WoL magic packets
def listen_for_wol():
    # Create a UDP socket
    pool = socketpool.SocketPool(wifi.radio)
    sock = pool.socket(pool.AF_INET, pool.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 9))  # Listen on port 9 (default WoL port)

    print("Listening for Wake-on-LAN magic packets...")
    start_time = time.monotonic()  # Track time for Wi-Fi verification

    sock.settimeout(10)  # Set a timeout of 10 seconds

    while True:
        try:
            buffer = bytearray(1024)
            num_bytes, addr = sock.recvfrom_into(buffer)  # Read data into buffer
            packet = buffer[:num_bytes]

            if is_wol_magic_packet(packet, TARGET_MAC):
                print("Received WoL magic packet for target MAC:", TARGET_MAC)
                if not is_device_on():
                    print("Device is OFF. Pressing power button...")
                    press_power_button()
                else:
                    print("Device is already ON. Ignoring WoL packet.")

        except OSError:  # Raised if the socket times out
            print("No WoL packet received. Checking Wi-Fi status...")
            verify_and_reconnect_wifi()  # Check Wi-Fi connection

# Main program
if __name__ == "__main__":
    while True:
        if connect_wifi():  # Attempt to connect to Wi-Fi
            listen_for_wol()  # Start listening for WoL packets
        else:
            print("Raspberry Pi Pico W is not connected to Wi-Fi. Red LED will blink twice.")
