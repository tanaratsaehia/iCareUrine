
// Voltage divider resistor values (in ohms)
const float R1 = 10000.0;  // Resistor connected to Vbat (top)
const float R2 = 10000.0;  // Resistor connected to GND (bottom)

// Reference voltages
const float maxVoltage = 3.95;  // Maximum voltage when fully charged
const float minVoltage = 3.0;  // Minimum voltage when fully discharged

// Calibration factor
const float calibrationFactor = 3.82 / 3.59;  // Adjust this ratio based on multimeter and ADC readings

// Function to read the battery voltage with calibration
float readBatteryVoltage() {
  int analogValue = analogRead(Battery);  // Read the analog pin
  float voltage = (analogValue / 4095.0) * 3.3;  // Convert to voltage (ESP32 has 3.3V ADC reference)

  // Adjust voltage reading based on the voltage divider
  voltage = voltage * (R1 + R2) / R2;

  // Apply calibration factor
  voltage = voltage * calibrationFactor;

  return voltage;
}

// Function to calculate battery percentage based on voltage
int get_batt_percent() {
  float voltage = readBatteryVoltage();
  if (voltage >= maxVoltage) {
    return 100;
  } else if (voltage <= minVoltage) {
    return 0;
  } else {
    return (int)((voltage - minVoltage) / (maxVoltage - minVoltage) * 100);
  }
}

unsigned long serial_display_batt_millis;
void serial_display_battery(){
  if (millis() - serial_display_batt_millis >= 1000){
    serial_display_batt_millis = millis();
    float batteryVoltage = readBatteryVoltage();
    int batteryPercentage = get_batt_percent();

    Serial.print("Battery Voltage: ");
    Serial.print(batteryVoltage);
    Serial.println(" V");

    Serial.print("Battery Percentage: ");
    Serial.print(batteryPercentage);
    Serial.println(" %");
  }
}