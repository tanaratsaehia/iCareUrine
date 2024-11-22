import threading
import cv2
import board
import digitalio
import busio
from adafruit_rgb_display import ili9341
from PIL import Image, ImageDraw, ImageFont
import RPi.GPIO as GPIO  # For load cell
from hx711 import HX711  # For load cell
from gpiozero import Button  # For rotary switch
from time import sleep
from datetime import datetime, timedelta
from adafruit_ds3231 import DS3231  # For DS3231 RTC
import requests  # For Line notifications
import subprocess  # For checking Wi-Fi status

# Rotary Encoder Pins
ROTARY_SW = 17  # Switch pin

# Load Cell Pins
HX711_DT = 6  # Data pin
HX711_SCK = 5  # Clock pin

# TFT Display Configuration
TFT_CS = board.D8  # Chip Select
TFT_DC = board.D24  # Data/Command
TFT_RST = board.D25  # Reset
TFT_MOSI = board.MOSI  # SPI MOSI
TFT_SCLK = board.SCK  # SPI Clock
TFT_LED = board.D18  # Backlight (optional)

# Initialize GPIO for Load Cell
GPIO.setmode(GPIO.BCM)
hx = HX711(dout_pin=HX711_DT, pd_sck_pin=HX711_SCK)

# Initialize I2C for RTC
i2c = busio.I2C(board.SCL, board.SDA)
rtc = DS3231(i2c)

# Initialize Rotary Switch
rotary_switch = Button(ROTARY_SW, pull_up=True)

# TFT Display Initialization
spi = busio.SPI(TFT_SCLK, MOSI=TFT_MOSI)
cs_pin = digitalio.DigitalInOut(TFT_CS)
dc_pin = digitalio.DigitalInOut(TFT_DC)
reset_pin = digitalio.DigitalInOut(TFT_RST)
backlight = digitalio.DigitalInOut(TFT_LED)
backlight.direction = digitalio.Direction.OUTPUT
backlight.value = True  # Turn on the backlight

display = ili9341.ILI9341(
    spi,
    cs=cs_pin,
    dc=dc_pin,
    rst=reset_pin,
    baudrate=24000000,
    width=240,
    height=320,
)

# Shared Variables
current_weight = 0.0
notify_time = None
line_token = "AVG4lIB1KIuajDtR49TUTl87VzV8VIEGOGuVPB1HRi9"  # Replace with your Line Notify token

# Functions
def check_wifi():
    """Check if Wi-Fi is connected."""
    try:
        result = subprocess.check_output(
            ["hostname", "-I"], stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()
        if result:
            return f"Connected. Device IP: {result.split()[0]}"
        else:
            return "Please set Wi-Fi: Name: A, Password: 11223344"
    except Exception:
        return "Please set Wi-Fi: Name: A, Password: 11223344"

def show_text_on_tft(lines):
    """Display text horizontally on the TFT screen."""
    image = Image.new("RGB", (display.width, display.height), "black")
    draw = ImageDraw.Draw(image)
    font = ImageFont.load_default()
    y = 10

    for line in lines:
        draw.text((10, y), line, fill="white", font=font)
        y += 20  # Adjust vertical spacing

    display.image(image)

def send_line_notification(message, image=None):
    """Send a Line notification with an optional image."""
    headers = {"Authorization": f"Bearer {line_token}"}
    data = {"message": message}
    files = {"imageFile": open(image, "rb")} if image else None
    try:
        requests.post(
            "https://notify-api.line.me/api/notify", headers=headers, data=data, files=files
        )
    except Exception as e:
        print(f"Failed to send Line notification: {e}")

def read_camera_frame():
    """Capture a frame from the camera."""
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        return None
    ret, frame = cap.read()
    cap.release()
    return frame if ret else None

def show_camera_feed():
    """Show live camera feed on the TFT display."""
    cap = cv2.VideoCapture(0)
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break
        frame = cv2.resize(frame, (display.width, display.height))
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image = Image.fromarray(frame_rgb)
        draw = ImageDraw.Draw(image)
        draw.text((10, 10), "Press to continue", fill="white")
        display.image(image)
        if rotary_switch.is_pressed:
            cap.release()
            break

def start_monitoring():
    """Start the main monitoring logic."""
    global current_weight, notify_time

    # Tare the load cell
    hx.zero()
    current_weight = hx.get_weight_mean(10)
    notify_time = datetime.now() + timedelta(minutes=1)

    send_line_notification(f"Next notify at: {notify_time.strftime('%Y-%m-%d %H:%M:%S')}")

    while True:
        time_remaining = int((notify_time - datetime.now()).total_seconds())
        if time_remaining < 0:
            time_remaining = 0

        # Update display more frequently
        show_text_on_tft([
            "Monitoring...",
            f"Weight: {current_weight:.2f} g",
            f"Next Notify: {time_remaining}s"
        ])

        if datetime.now() >= notify_time:
            new_weight = hx.get_weight_mean(10)
            diff = abs(new_weight - current_weight)

            if diff > 30:
                send_line_notification("Normally.")
            else:
                # Capture and send an image if "Danger!"
                frame = read_camera_frame()
                if frame is not None:
                    image_path = "/tmp/danger.jpg"
                    cv2.imwrite(image_path, frame)
                    send_line_notification("Danger!!", image_path)
                else:
                    send_line_notification("Danger!! (no image available)")

            current_weight = new_weight
            notify_time = datetime.now() + timedelta(minutes=1)

# Main Logic
if __name__ == "__main__":
    try:
        # Step 1: Show Wi-Fi status
        wifi_status = check_wifi()
        show_text_on_tft([wifi_status])
        sleep(6)

        # Step 2: Show live camera feed
        show_camera_feed()

        # Step 3: Start monitoring
        start_monitoring()

    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        GPIO.cleanup()
