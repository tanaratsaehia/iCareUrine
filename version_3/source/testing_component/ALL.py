import threading
import cv2
import board
import digitalio
import busio
from adafruit_rgb_display import ili9341
from PIL import Image
import RPi.GPIO as GPIO  # For load cell
from hx711 import HX711  # For load cell
from gpiozero import Button, Buzzer  # For rotary switch
from time import sleep
import Adafruit_ADS1x15  # For ADS1115
from adafruit_ds3231 import DS3231  # For DS3231 RTC

# Initialize peripherals
buzzer = Buzzer(21)

# Rotary Encoder Pins
ROTARY_A = 22  # Clock pin
ROTARY_B = 27  # Data pin
ROTARY_SW = 17  # Switch pin

# Load Cell Pins
HX711_DT = 6      # Data pin
HX711_SCK = 5     # Clock pin

# TFT Display Configuration
TFT_CS = board.D8       # Chip Select
TFT_DC = board.D24      # Data/Command
TFT_RST = board.D25     # Reset
TFT_MOSI = board.MOSI   # SPI MOSI
TFT_SCLK = board.SCK    # SPI Clock
TFT_LED = board.D18     # Backlight (optional)

# Shared variables
position = 0  # Rotary position
lock = threading.Lock()

# Initialize I2C for DS3231 and ADC
i2c = busio.I2C(board.SCL, board.SDA)
rtc = DS3231(i2c)
adc = Adafruit_ADS1x15.ADS1115()

GAIN = 1  # ADS1115 gain

# Rotary switch handling
def rotary_switch_task():
    global position
    rotary_a = Button(ROTARY_A)
    rotary_b = Button(ROTARY_B)
    rotary_switch = Button(ROTARY_SW, pull_up=True)

    def rotary_turned():
        global position
        with lock:
            if rotary_a.is_pressed and not rotary_b.is_pressed:
                position += 1  # Clockwise
            elif rotary_b.is_pressed and not rotary_a.is_pressed:
                position -= 1  # Counterclockwise
            print(f"Rotary Position: {position}")

    def switch_pressed():
        print("Rotary switch pressed!")
        buzzer.on()  # Turn on the buzzer
        sleep(0.5)   # Keep the buzzer on for 0.5 seconds
        buzzer.off()  # Turn off the buzzer

    rotary_a.when_pressed = rotary_turned
    rotary_b.when_pressed = rotary_turned
    rotary_switch.when_pressed = switch_pressed

    print("Rotary switch task running.")
    while True:
        sleep(0.1)

# Load cell handling
def load_cell_task():
    GPIO.setmode(GPIO.BCM)
    hx = HX711(dout_pin=HX711_DT, pd_sck_pin=HX711_SCK)
    hx.zero()  # Tare the scale

    print("Load cell task running.")
    while True:
        weight = hx.get_weight_mean(10)  # Average of 10 samples
        with lock:
            print(f"Weight: {weight:.2f} grams")
        sleep(0.5)

# Video playback handling
def video_playback_task():
    # Set up SPI bus and display interface
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
        baudrate=99999999,  # 99.99 MHz
        width=240,
        height=320,
    )

    video_path = 0  # Use 0 for the camera or replace with a video path
    cap = cv2.VideoCapture(video_path)

    target_width, target_height = display.width, display.height
    cv2.setUseOptimized(True)

    print("Video playback task running.")
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            print("End of video or error reading frame.")
            break

        if frame.shape[1] != target_width or frame.shape[0] != target_height:
            frame = cv2.resize(frame, (target_width, target_height))

        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image = Image.fromarray(frame_rgb)
        display.image(image)

    cap.release()
    print("Video playback finished.")

# RTC and ADC handling
def rtc_adc_task():
    print("RTC and ADC task running.")
    while True:
        # Read RTC
        current_time = rtc.datetime
        print(f"RTC Time: {current_time.tm_year}-{current_time.tm_mon:02d}-{current_time.tm_mday:02d} "
              f"{current_time.tm_hour:02d}:{current_time.tm_min:02d}:{current_time.tm_sec:02d}")

        # Read ADC (A3)
        value = adc.read_adc(3, gain=GAIN)
        voltage = value * 4.096 / 32767.0
        print(f"ADC Reading A3: {value}, Voltage: {voltage:.6f} V")

        sleep(1)

# Main function
if __name__ == "__main__":
    try:
        rotary_thread = threading.Thread(target=rotary_switch_task, daemon=True)
        load_cell_thread = threading.Thread(target=load_cell_task, daemon=True)
        video_thread = threading.Thread(target=video_playback_task, daemon=True)
        rtc_adc_thread = threading.Thread(target=rtc_adc_task, daemon=True)

        rotary_thread.start()
        load_cell_thread.start()
        video_thread.start()
        rtc_adc_thread.start()

        print("All tasks are running. Press CTRL+C to exit.")
        while True:
            sleep(1)
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        GPIO.cleanup()  # Cleanup for RPi.GPIO
