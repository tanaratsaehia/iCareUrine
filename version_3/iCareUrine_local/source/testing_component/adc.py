import Adafruit_ADS1x15

# Initialize the ADS1115 ADC
adc = Adafruit_ADS1x15.ADS1115()

# Gain settings for the ADC
GAIN = 1  # +/-4.096V range

# Read single-ended input on A3
value = adc.read_adc(3, gain=GAIN)  # A3 corresponds to channel 3

# Convert the raw ADC value to voltage (assuming 16-bit ADC)
max_voltage = 4.096  # Maximum voltage based on the gain setting
voltage = value * max_voltage / 32767.0

print(f"Raw ADC Value: {value}")
print(f"Voltage on A3: {voltage:.6f} V")
