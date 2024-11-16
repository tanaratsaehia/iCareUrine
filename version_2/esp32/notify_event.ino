#include <TridentTD_LineNotify.h>

// Test Token
// #define LINE_TOKEN_1  "AVG4lIB1KIuajDtR49TUTl87VzV8VIEGOGuVPB1HRi9"
// #define LINE_TOKEN_2  "AVG4lIB1KIuajDtR49TUTl87VzV8VIEGOGuVPB1HRi9"
// #define LINE_TOKEN_3  "AVG4lIB1KIuajDtR49TUTl87VzV8VIEGOGuVPB1HRi9"

// Real Token
// #define LINE_TOKEN  "xlPCf7LajMbAHq0E9xNLST6gZlxnhm4BdaVIi9VwwxE"
#define LINE_TOKEN_1  "xlPCf7LajMbAHq0E9xNLST6gZlxnhm4BdaVIi9VwwxE" // TOKEN
#define LINE_TOKEN_2  "q0aSBfvTv3YoNrubTM8guFUdwctRQFYNFafgW9Gyy1m" // TOKEN
#define LINE_TOKEN_3  "0xVEI3flguwS2Q2aeTX0X3ZsFqTNIFkQpM1Y3c7pblB" // TOKEN

bool notify_eight_hour_flag;

void change_device_mode_check(){
  unsigned long time_out = millis();
  while (is_btn_pressed()){
    if (millis() - time_out >= 2000){
      DEVICE_MODE = true;
      onBuzzer(0.05);
      onBuzzer(0.05);
      clear_display();
      display_custom("Present Mode", 2, 0);
      display_custom("Ready Now!", 3, 1);
      wm.resetSettings();
      delay(3000);
      break;
    }
  }
  Serial.println(DEVICE_MODE? "device run on 'present' mode!": "device run on normal mode");
  while (is_btn_pressed()) {}
}

void notify_event(){
  int time_array[3];
  if (is_device_online()){
    String* dateTime = get_date_time();
    splitTimeString(dateTime[1], time_array);
  }else{
    int *tmp_time = calculate_time_from_millis();
    time_array[0] = tmp_time[0];
    time_array[1] = tmp_time[1];
  }
  
  if ((time_array[0] == next_hour_notify & time_array[1] == next_minute_notify) || (time_array[0] >= next_hour_notify & time_array[1] >= next_minute_notify & next_hour_notify != 0)){
    // notify condition
    sleep_mode_millis = millis();
    sleep_mode_flag = false;
    lcd_wake();
    load_cell_wake();
    delay(1000);

    String noti_str;
    float weight = get_weight(5);
    if (weight - previous_weight <= 30){
      alert_buzzer_flag = true;
      alert_buzzer_millis = millis();
      noti_str = "\n!!! ปริมาณปัสสาวะน้อยกว่า 30 มล. !!!\nชั่วโมงที่แล้ว: "+String(previous_weight)+" มล.\nปัจจุบัน: "+String(weight)+" มล.";
    }else{
      noti_str = "\nปริมาณปัสสาวะปกติ\nชั่วโมงที่แล้ว: "+String(previous_weight)+" มล.\nปัจจุบัน: "+String(weight)+" มล.";
    }

    if (is_device_online()){
      bool line_res = LINE.notify(noti_str);
      int try_counter = 0;
      while (!line_res & try_counter <= 5){
        try_counter++;
        if (line_token_counter == 1){
          line_token_counter++;
          LINE.setToken(LINE_TOKEN_1);
        }else if (line_token_counter == 2){
          line_token_counter++;
          LINE.setToken(LINE_TOKEN_2);
        }else if (line_token_counter == 3){
          line_token_counter = 1;
          LINE.setToken(LINE_TOKEN_3);
        }

        line_res = LINE.notify(noti_str);
      }
    }

    if (DEVICE_MODE){ // present mode noti every minute
      if (next_minute_notify < 59){
        next_minute_notify++;
      }else if (next_minute_notify == 59){
        next_minute_notify = 0;
        next_hour_notify++;
      }
    }else{ // normal mode noti every hour
      if (next_hour_notify < 23 & is_device_online()){
        next_hour_notify++;
      }else if (next_hour_notify == 23){
        next_hour_notify = 0;
      }else if (!is_device_online()){
        next_hour_notify++;
      }
    }

    previous_weight = weight;
  }

  if ((time_array[0] == 6 | time_array[0] == 14 | time_array[0] == 22) & time_array[1] == 0 & is_device_online() & !notify_eight_hour_flag){
    notify_eight_hour_flag = true;
    sleep_mode_millis = millis();
    sleep_mode_flag = false;
    lcd_wake();
    load_cell_wake();
    delay(1000);

    float weight = get_weight(5);
    String noti_str = "นี่คือข้อความรายงานผลของปริมาณปัสสาวะตามเวลาเปลี่ยนเวร\nปริมาณปัสสาวะปัจจุบัน: "+String(weight)+" มล.";
    bool line_res = LINE.notify(noti_str);
    int try_counter = 0;
    while (!line_res & try_counter <= 5){
      try_counter++;
      if (line_token_counter == 1){
        line_token_counter++;
        LINE.setToken(LINE_TOKEN_1);
      }else if (line_token_counter == 2){
        line_token_counter++;
        LINE.setToken(LINE_TOKEN_2);
      }else if (line_token_counter == 3){
        line_token_counter = 1;
        LINE.setToken(LINE_TOKEN_3);
      }

      line_res = LINE.notify(noti_str);
    }
  }
  if (time_array[1] == 2 & notify_eight_hour_flag){
    notify_eight_hour_flag = false;
  }

  if (millis() % 3500 <= 15 & !is_device_online()){
    WiFi.begin();
  }
}

