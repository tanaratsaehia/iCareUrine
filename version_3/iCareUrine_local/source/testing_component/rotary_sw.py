from gpiozero import Button
from signal import pause

# Rotary Encoder Pins
ROTARY_A = 22  # Clock pin
ROTARY_B = 27  # Data pin
ROTARY_SW = 17  # Switch pin

# Track rotary encoder position
position = 0

# Define button inputs
rotary_a = Button(ROTARY_A)
rotary_b = Button(ROTARY_B)
rotary_switch = Button(ROTARY_SW, pull_up=True)

def rotary_turned():
    global position
    if rotary_a.is_pressed and not rotary_b.is_pressed:
        position += 1  # Clockwise
    elif rotary_b.is_pressed and not rotary_a.is_pressed:
        position -= 1  # Counterclockwise
    print(f"Position: {position}")

def switch_pressed():
    print("Rotary switch pressed!")

# Setup event detection
rotary_a.when_pressed = rotary_turned
rotary_b.when_pressed = rotary_turned
rotary_switch.when_pressed = switch_pressed

print("Rotary encoder test initialized. Turn the encoder or press the switch.")
pause()  # Keep the script running
