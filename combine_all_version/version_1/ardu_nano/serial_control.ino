unsigned long timeOutReadCmdMillis;
void read_command(String times){
  timeOutReadCmdMillis = millis();
  while (true){
    // if (times == "inf"){
    //   if (millis() - timeOutReadCmdMillis >= 30000){
    //     Serial.println("stop with time out");
    //     break;
    //   }
    // }
    if (Serial.available()){
      String command = Serial.readStringUntil("\n");
      Serial.println(command);

      if (command.startsWith("c_")){
        Serial.println(command);
        command = command.substring(2);
        command.trim();

        if (command == "file_already_exist"){
          // file_already_exist
          clear_display();
          display_custom("select option", 2, 0);
          display_custom("keep record", 2, 1);

          bool btnState = false;
          bool disCase  = false;
          unsigned long pressBtnMillis;

          while (true){
            if (buttonPressed() & !btnState){
              btnState = true;
              pressBtnMillis = millis();
              // Serial.println("reset time counter");
            }else if (!buttonPressed() & btnState & millis() - pressBtnMillis>=100){
              btnState = false;
              if (!disCase & !buttonPressed()){
                disCase = true;
                clear_display();
                display_custom("select option", 2, 0);
                display_custom("delete record", 2, 1);
                while (buttonPressed()){}
              }else if (disCase & !buttonPressed()){
                disCase = false;
                clear_display();
                display_custom("select option", 2, 0);
                display_custom("keep record", 2, 1);
                while (buttonPressed()){}
              }
            }else if (buttonPressed()&millis()-pressBtnMillis>=3000&btnState){
              if (disCase){
                clear_display();
                display_custom("record deleted", 1, 0);
                onBuzzer(0.1);
                Serial.println("c_delete_file");
                while (buttonPressed()){}
                return;
              }else if (!disCase){
                clear_display();
                display_custom("keep record", 2, 0);
                onBuzzer(0.1);
                Serial.println("c_keep_file");
                while (buttonPressed()){}
                return;
              }
            }
          }
          return;
        }else if (command == "file_created"){
          return;
        }else if (command == "wifi_connected"){
          clear_display();
          display_custom("WiFi connected", 1, 0);
          onBuzzer(0.05);
          onBuzzer(0.05);
          Serial.println("c_connected_ok");
          clearSerialBuffer();
          wifiState = true;
          delay(2500);
          return;
        }else if (command == "wifi_fail"){
          clear_display();
          display_custom("connect fail", 2, 0);
          display_custom("pls try again", 1, 1);
          Serial.println("c_fail_ok");
          clearSerialBuffer();
          onBuzzer(0.5);
          // reset mcu here
          return;
        }else if (command == "wifi_loss"){
          clear_display();
          display_custom("connect loss", 2, 0);
          display_custom("pls check wifi", 1, 1);
          wifiState = false;
          // onBuzzer(0.5);
          return;
        }else if (command == "config_wifi"){
          clear_display();
          display_custom("Pls config wifi", 1, 0);
          display_custom("N: I Care Urine", 1, 1);
          Serial.println("c_config_ok");
          clearSerialBuffer();
          // return;
        }else if (command == "wake_up"){
          if (sleepMode){
            load_cell_wake();
            delay(1500);
          }
          // Serial.println("c_weight_"+String(get_weight(10)));
          while (true){
            if (Serial.available()){
              String command = Serial.readStringUntil('\n');
              if (command.startsWith("c_")){
                command = command.substring(2);
                command.trim();
                if (command == "weight_ok"){
                  clearSerialBuffer();
                  return;
                }
              }
            }else{
              Serial.println("c_weight_"+String(get_weight(10)));
            }
          }
        }else if (command == "alert_buz"){
          buzzerAlert = true;
          buzAlertMillis = millis();
          sleepModeMillis = millis();
          Serial.println("c_buz_ok");
          delay(100);
          Serial.println("c_buz_ok");
        }
      }
    }

    if (times == "one"){
      return;
    }

    // Serial.println("here");
  }
}

void clearSerialBuffer() {
  while (Serial.available()) {
    String temp = Serial.readStringUntil('\n');
  }
}