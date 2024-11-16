const int batteryPin = A1;
const float R1 = 10000.0;
const float R2 = 10000.0;
const float minBatt = 3.4;
const float maxBatt = 4.2;

float get_batt_percent(){
  int sensorValue = analogRead(batteryPin);
  float voltage = (sensorValue * (5.0 / 1023.0))+0.25;
  float batteryVoltage = voltage / (R2 / (R1 + R2));
  float percentage = ((batteryVoltage - minBatt) / (maxBatt - minBatt)) * 100.0;

  // Serial.println("Analog :" + String(sensorValue));
  // Serial.println("Voltage :" + String(voltage));
  // Serial.println("battery Voltage :" + String(batteryVoltage));
  // Serial.println("battery percentage :" + String(percentage));

  if (percentage > 100) {
    percentage = 100;
  } else if (percentage < 0) {
    percentage = 0;
  }

  return percentage;
}