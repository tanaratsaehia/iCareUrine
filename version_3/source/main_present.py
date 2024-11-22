import threading
import cv2
import board
import digitalio
import busio
from adafruit_rgb_display import ili9341
from PIL import Image, ImageDraw, ImageFont
import RPi.GPIO as GPIO  # For load cell
from hx711 import HX711  # For load cell
from gpiozero import Button, Buzzer  # For rotary switch
from time import sleep
from datetime import datetime, timedelta
from adafruit_ds3231 import DS3231  # For DS3231 RTC
import requests  # For Line notifications
import subprocess  # For checking Wi-Fi status
import os
current_directory = os.path.dirname(os.path.abspath(__file__))

DEVICE_MODE = True # True->1hr mode, False->1min mode
# buzzer = Buzzer(21) # Real buzzer
buzzer = Buzzer(7) # Fake buzzer for mute
fan = Buzzer(4)

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

display = ili9341.ILI9341(
    spi,
    cs=cs_pin,
    dc=dc_pin,
    rst=reset_pin,
    baudrate=99999999,
    width=240,
    height=320,
)

# Shared Variables
current_weight = 0.0
notify_time = None
font_path="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
# line_token = "AVG4lIB1KIuajDtR49TUTl87VzV8VIEGOGuVPB1HRi9"  # test token
line_token = "xlPCf7LajMbAHq0E9xNLST6gZlxnhm4BdaVIi9VwwxE" # real token

# Functions
def on_buzzer(times=1, _sleep=0.3):
    for _ in range(times):
        # print(_)
        buzzer.on()
        sleep(_sleep)
        buzzer.off()
        sleep(_sleep)

def buzzer_alert(stop_event, button, buzzer, duration=50, toggle_interval=0.3):
    """
    Toggles the buzzer for a specified duration or until the button is pressed.
    Runs in a separate thread.
    
    Args:
        stop_event: A threading.Event to signal when to stop.
        button: The button object to check for a press.
        buzzer: The buzzer object to toggle.
        duration: Duration in seconds for the buzzer to run.
        toggle_interval: Interval in seconds to toggle the buzzer.
    """
    start_time = datetime.now()
    while (datetime.now() - start_time).total_seconds() < duration:
        if stop_event.is_set() or button.is_pressed:
            break
        buzzer.on()
        sleep(toggle_interval)
        buzzer.off()
        sleep(toggle_interval)

def control_fan():
    """
    Reads the CPU temperature and toggles the fan based on thresholds:
    - Turns on the fan if temperature > 55°C.
    - Turns off the fan if temperature < 45°C.
    """
    # Read CPU temperature
    with open("/sys/class/thermal/thermal_zone0/temp", "r") as f:
        cpu_temp = int(f.read()) / 1000.0  # Convert from millidegree Celsius to Celsius

    print(f"CPU Temperature: {cpu_temp:.2f}°C")

    # Control the fan based on temperature thresholds
    if cpu_temp > 55.0:
        fan.on()  # Turn on the fan
        print("Fan turned ON")
    elif cpu_temp < 45.0:
        fan.off()  # Turn off the fan
        print("Fan turned OFF")


def check_wifi():
    """Check if Wi-Fi is connected."""
    try:
        result = subprocess.check_output(["hostname", "-I"], stderr=subprocess.DEVNULL).decode("utf-8").strip()
        if result:
            on_buzzer(times=2, _sleep=0.04)
            show_text_on_tft(["Connected.", f"Device IP: {result.split()[0]}"], y=100)
            return 
        else:
            show_text_on_tft(["Please set Wi-Fi","Name: A","Password: 11223344"], y=100)
            while not result:
                result = subprocess.check_output(["hostname", "-I"], stderr=subprocess.DEVNULL).decode("utf-8").strip()
            check_wifi()
            return 
    except Exception:
        return ["Please set Wi-Fi","Name: A","Password: 11223344"]

def show_text_on_tft(lines, x=5, y=50, sep=5, font_size=16):
    """Display text horizontally on the TFT screen with adjustable font size."""
    image = Image.new("RGB", (display.width, display.height), "black")
    draw = ImageDraw.Draw(image)
    
    # Load a TrueType font with the specified font size
    try:
        font = ImageFont.truetype(font_path, font_size)
    except IOError:
        print("TrueType font not found; using default font.")
        font = ImageFont.load_default()  # Fallback to default font

    for line in lines:
        draw.text((x, y), line, fill="white", font=font)
        y += font_size + sep  # Adjust vertical spacing dynamically

    display.image(image.rotate(90))

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
    if ret:
        # Rotate the frame by 90 degrees clockwise
        rotated_frame = cv2.rotate(frame, cv2.ROTATE_90_CLOCKWISE)
        return rotated_frame
    else:
        return None

def show_camera_feed():
    """Show live camera feed on the TFT display with rotated text."""
    cap = cv2.VideoCapture(0)
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break
        frame = cv2.resize(frame, (display.width, display.height))
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image = Image.fromarray(frame_rgb)

        # Create a temporary image to hold the rotated text
        text_image = Image.new("RGBA", (display.width, display.height), (0, 0, 0, 0))
        text_draw = ImageDraw.Draw(text_image)
        # Load a TrueType font with the specified font size
        try:
            font = ImageFont.truetype(font_path, 18)
        except IOError:
            print("TrueType font not found; using default font.")
            font = ImageFont.load_default()  # Fallback to default font

        # Draw text on the temporary image
        text_draw.text((0, 10), "Press to continue", fill="white", font=font)

        # Rotate the text image by 90 degrees
        rotated_text_image = text_image.rotate(90, expand=True)

        # Composite the rotated text onto the original camera frame
        image.paste(rotated_text_image, (0, 0), rotated_text_image)

        display.image(image)

        if rotary_switch.is_pressed:
            cap.release()
            break

def start_monitoring():
    """Start the main monitoring logic."""
    global current_weight, notify_time

    # Initialize the fan control check
    last_fan_check_time = datetime.now()

    # Tare the load cell
    hx.zero()
    current_weight = int(hx.get_weight_mean(10))

    if DEVICE_MODE:
        notify_time = datetime.now() + timedelta(hours=1)
    else:
        notify_time = datetime.now() + timedelta(minutes=1)

    frame = read_camera_frame()
    if frame is not None:
        save_path = os.path.join(current_directory, "tmp/start_up.jpg")
        cv2.imwrite(save_path, frame)
        send_line_notification(f"พร้อมใช้งาน\nแจ้งเตือนครั้งต่อไปเมื่อ {notify_time.strftime('%H:%M:%S')}", save_path)
    else:
        send_line_notification(f"พร้อมใช้งาน\nแจ้งเตือนครั้งต่อไปเมื่อ {notify_time.strftime('%H:%M:%S')}")

    # List of target times in 24-hour format
    target_hours = [6, 14, 22]
    last_notified_hour = None  # Track the last notified hour to avoid duplicate notifications

    while True:
        # Check CPU temperature and control fan every 30 seconds
        if (datetime.now() - last_fan_check_time).total_seconds() > 30:
            control_fan()
            last_fan_check_time = datetime.now()
        
        time_remaining = int((notify_time - datetime.now()).total_seconds())
        if time_remaining < 0:
            time_remaining = 0

        # Calculate minutes and seconds
        minutes, seconds = divmod(time_remaining, 60)

        # Update display with min:sec format
        show_text_on_tft(
            ["I Care Urine",
             f"Urine: {int(abs(hx.get_weight_mean(30)))} ml",
             f"Notify in: {minutes:02d}:{seconds:02d}"
            ],
            x=5, y=100, sep=15, font_size=26
        )

        # Check if the current time matches any target time
        now = datetime.now()
        if now.hour in target_hours and now.hour != last_notified_hour:
            current_weight = int(abs(hx.get_weight_mean(10)))  # Get the current weight
            last_notified_hour = now.hour  # Update the last notified hour
            frame = read_camera_frame()

            if frame is not None:
                save_path = os.path.join(current_directory, "tmp/target_hours.jpg")
                cv2.imwrite(save_path, frame)
                send_line_notification(f"\nรายงานผลตามเวลาเปลี่ยนเวร\nปริมาณปัสสาวะปัจจุบัน: {current_weight} ml", save_path)
            else:
                send_line_notification(f"\nรายงานผลตามเวลาเปลี่ยนเวร\nปริมาณปัสสาวะปัจจุบัน: {current_weight} ml (no image available)")

        if datetime.now() >= notify_time:
            print('notify at', str(datetime.now()))
            new_weight = int(hx.get_weight_mean(20))
            diff = (new_weight - current_weight)

            if diff > 30:
                frame = read_camera_frame()
                if frame is not None:
                    save_path = os.path.join(current_directory, "tmp/normal.jpg")
                    cv2.imwrite(save_path, frame)
                    send_line_notification(f"\nปริมาณปัสสาวะปกติ\nชั่วโมงที่แล้ว: {abs(current_weight)} ml\nปัจจุบัน: {abs(new_weight)} ml", save_path)
                else:
                    send_line_notification(f"\nปริมาณปัสสาวะปกติ\nชั่วโมงที่แล้ว: {abs(current_weight)} ml\nปัจจุบัน: {abs(new_weight)} ml (no image available)")
            else:
                # Start a new thread for the buzzer alert
                stop_event = threading.Event()
                buzzer_thread = threading.Thread(
                    target=buzzer_alert,
                    args=(stop_event, rotary_switch, buzzer),
                    daemon=True  # Allows the thread to exit when the main program exits
                )
                buzzer_thread.start()

                # Capture and send an image if "Danger!"
                frame = read_camera_frame()
                if frame is not None:
                    save_path = os.path.join(current_directory, "tmp/danger.jpg")
                    cv2.imwrite(save_path, frame)
                    send_line_notification(f"\n!! ปริมาณปัสสาวะน้อยกว่า 30 ml !!\nชั่วโมงที่แล้ว: {abs(current_weight)} ml\nปัจจุบัน: {abs(new_weight)} ml", save_path)
                else:
                    send_line_notification(f"\n!! ปริมาณปัสสาวะน้อยกว่า 30 ml !!\nชั่วโมงที่แล้ว: {abs(current_weight)} ml\nปัจจุบัน: {abs(new_weight)} ml (no image available)")

            current_weight = new_weight
            if DEVICE_MODE:
                notify_time = datetime.now() + timedelta(hours=1)
            else:
                notify_time = datetime.now() + timedelta(minutes=1)

# Main Logic
if __name__ == "__main__":
    try:
        fan.on()
        # Step 0: Set device mode
        if rotary_switch.is_pressed:
            DEVICE_MODE = False
            on_buzzer(_sleep=0.1)
            sleep(3)

        # Step 1: Show Wi-Fi status
        check_wifi()
        sleep(6)

        # Step 2: Show live camera feed
        show_camera_feed()
        fan.off()
        
        hx.set_scale_ratio(244.5)

        # Step 3: Start monitoring
        start_monitoring()

    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        GPIO.cleanup()
