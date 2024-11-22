import time
import Adafruit_ADS1x15
import board
import busio
from adafruit_ds3231 import DS3231

# Initialize I2C for DS3231
i2c = busio.I2C(board.SCL, board.SDA)

# Initialize DS3231 RTC
rtc = DS3231(i2c)

# Initialize the ADS1115 ADC
adc = Adafruit_ADS1x15.ADS1115()

# Gain settings for the ADC
GAIN = 1  # +/-4.096V range

def read_rtc_time():
    """Read and print the time from DS3231 RTC."""
    current_time = rtc.datetime
    print("DS3231 Time:")
    print(f"{current_time.tm_year}-{current_time.tm_mon:02d}-{current_time.tm_mday:02d} "
          f"{current_time.tm_hour:02d}:{current_time.tm_min:02d}:{current_time.tm_sec:02d}")

def read_adc_value():
    """Read and print the voltage from ADS1115 on A3."""
    # Read single-ended input on A3
    value = adc.read_adc(3, gain=GAIN)  # A3 corresponds to channel 3

    # Convert the raw ADC value to voltage (assuming 16-bit ADC)
    max_voltage = 4.096  # Maximum voltage based on the gain setting
    voltage = value * max_voltage / 32767.0

    print("ADS1115 ADC Reading from A3:")
    print(f"Raw ADC Value: {value}")
    print(f"Voltage: {voltage:.6f} V")

if __name__ == "__main__":
    try:
        while True:
            print("-" * 40)
            read_rtc_time()
            read_adc_value()
            time.sleep(1)  # Wait 1 second between readings
    except KeyboardInterrupt:
        print("\nExiting program.")