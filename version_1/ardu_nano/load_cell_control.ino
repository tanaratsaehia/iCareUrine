#include "HX711.h"
// https://github.com/bogde/HX711

#define LOADCELL_DOUT_PIN 5
#define LOADCELL_SCK_PIN 6
#define CALIBRATION_FACTOR_OLD 919.21 //ไม่มีตะขอ
#define CALIBRATION_FACTOR 243.8 //มีตะขอ
#define KNOWN_WEIGHT 0.00 // gx10

HX711 scale;

void load_cell_init(){
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(500);
  scale.set_scale();
  scale.tare();
  scale.set_scale(CALIBRATION_FACTOR);
}

void load_cell_set_zero(){
  scale.set_scale();
  scale.tare();
  scale.set_scale(CALIBRATION_FACTOR);
}

float load_cell_get_calibration_factor(){
  float calibration_factor;
  scale.set_scale();
  scale.tare();

  Serial.println("Start set scale...");
  delay(2000);
  Serial.println("Place the known weight on the scale...");
  delay(8000);
  long raw_value = scale.get_units(10);
  calibration_factor = raw_value / KNOWN_WEIGHT;
  scale.set_scale(calibration_factor);
  Serial.print("Calibration factor: ");
  Serial.println(calibration_factor);
  Serial.println("Calibration done");
}

float get_weight(int unit){
  return scale.get_units(unit);
}

void load_cell_sleep(){
  scale.power_down();
}

void load_cell_wake(){
  scale.power_up();
}