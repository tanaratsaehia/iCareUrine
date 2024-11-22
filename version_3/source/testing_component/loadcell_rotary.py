import RPi.GPIO as GPIO  # For load cell
from hx711 import HX711  # For load cell
from gpiozero import Button  # For rotary switch
from time import sleep

# Rotary Encoder Pins
ROTARY_A = 22  # Clock pin
ROTARY_B = 27  # Data pin
ROTARY_SW = 17  # Switch pin

# Load Cell Pins
HX711_DT = 6      # Data pin
HX711_SCK = 5     # Clock pin

# Initialize Load Cell with RPi.GPIO
GPIO.setmode(GPIO.BCM)
hx = HX711(dout_pin=HX711_DT, pd_sck_pin=HX711_SCK)
hx.zero()  # Tare the scale

# Rotary Switch Setup with gpiozero
position = 0
rotary_a = Button(ROTARY_A)
rotary_b = Button(ROTARY_B)
rotary_switch = Button(ROTARY_SW, pull_up=True)

# Rotary Switch Callback
def rotary_turned():
    global position
    if rotary_a.is_pressed and not rotary_b.is_pressed:
        position += 1  # Clockwise
    elif rotary_b.is_pressed and not rotary_a.is_pressed:
        position -= 1  # Counterclockwise
    print(f"Rotary Position: {position}")

def switch_pressed():
    print("Rotary switch pressed!")

# Attach event callbacks
rotary_a.when_pressed = rotary_turned
rotary_b.when_pressed = rotary_turned
rotary_switch.when_pressed = switch_pressed

# Main Loop
try:
    print("System initialized. Reading load cell and rotary encoder.")
    while True:
        # Read weight from load cell
        weight = hx.get_weight_mean(10)  # Average of 10 samples
        print(f"Weight: {weight:.2f} grams")
        sleep(0.5)

except KeyboardInterrupt:
    print("Exiting...")
finally:
    GPIO.cleanup()  # Cleanup for RPi.GPIO
