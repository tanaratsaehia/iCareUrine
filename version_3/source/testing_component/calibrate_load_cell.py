import os
from hx711 import HX711  # Import HX711 library
import RPi.GPIO as GPIO

# Load Cell Pins
HX711_DT = 6  # Data pin
HX711_SCK = 5  # Clock pin

# Initialize GPIO for Load Cell
GPIO.setmode(GPIO.BCM)
hx = HX711(dout_pin=HX711_DT, pd_sck_pin=HX711_SCK)

def calibrate_load_cell(known_weight):
    """Calibrate the load cell using a known weight."""
    print("Make sure the load cell is empty and press Enter to tare.")
    input("Press Enter to continue...")
    hx.zero()  # Tare the scale (set to zero)
    tare_value = hx.get_raw_data_mean()
    print(f"Tare value: {tare_value}")

    print(f"Place the known weight ({known_weight} grams) on the load cell and press Enter.")
    input("Press Enter to continue...")
    raw_value = hx.get_raw_data_mean()
    print(f"Raw value with known weight: {raw_value}")

    calibration_factor = (raw_value - tare_value) / known_weight
    print(f"Calibration factor: {calibration_factor}")
    return calibration_factor

if __name__ == "__main__":
    try:
        known_weight = float(input("Enter the known weight in grams: "))
        calibration_factor = calibrate_load_cell(known_weight)

        # Save the calibration factor to a file
        current_directory = os.path.dirname(os.path.abspath(__file__))
        calibration_file = os.path.join(current_directory, "calibration_factor.txt")
        with open(calibration_file, "w") as f:
            f.write(str(calibration_factor))
        print(f"Calibration complete. Factor saved to {calibration_file}")
    except KeyboardInterrupt:
        print("Calibration interrupted.")
    finally:
        GPIO.cleanup()
