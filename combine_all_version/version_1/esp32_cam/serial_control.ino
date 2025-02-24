float get_weight(){
  String weight_data;
  while (true){
    if (Serial.available()){
      weight_data = Serial.readStringUntil('\n');
      if (weight_data.startsWith("c_weight_")){
        weight_data = weight_data.substring(9);
        // weight_data.trim();
        Serial.println("c_weight_ok");
        return weight_data.toFloat();
      }
    }

    if (millis() % 2000 == 0){
      Serial.println("c_wake_up");
      delay(10);
    }
  }
}