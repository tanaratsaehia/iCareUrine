#include <WiFi.h>
#include <WiFiManager.h>
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
int line_token_counter = 1;

#define ToggleBTN 19 // button pin
#define Buzzer 23 // buzzer pin
#define Battery 35 // pin for read battery voltage 
bool DEVICE_MODE = false; // true->present mode, false->normal mode
bool alert_buzzer_flag = false;
unsigned long alert_buzzer_millis;
float previous_weight;
unsigned long sleep_mode_millis;
unsigned long lcd_millis;
bool sleep_mode_flag = false;
int battery_loss_counter = 0;
bool batt_loss_flag = false;

int next_hour_notify;
int next_minute_notify;

int WAIT_TO_SLEEP = 3; // wait minutes into sleep

WiFiManager wm;

void setup() {
  pinMode_begin();
  lcd_init();
  change_device_mode_check();
  welcome_action();
  load_cell_init();
  wait_for_sero_point();
  sleep_mode_millis = millis();
}

void loop() {
  notify_event();
  sleep_mode_event();

  // common display when active
  if (millis() - lcd_millis >= 1000 & !sleep_mode_flag & !batt_loss_flag){
    lcd_millis = millis();
    clear_display();
    display_batt_and_weight();
    batt_loss_event();
  }

  // battery low event
  if (millis() - lcd_millis >= 60000 & batt_loss_flag){
    lcd_millis = millis();
    batt_loss_event();
  }
  
  // buzzer alert for any event!
  if (millis() - alert_buzzer_millis <= 50000 & alert_buzzer_flag){onBuzzer(0.35);}
  else{alert_buzzer_flag = false;}
  if (alert_buzzer_flag & is_btn_pressed()){alert_buzzer_flag = false;}
}

void sleep_mode_event(){
  if (is_btn_pressed()){
    sleep_mode_millis = millis();
  }

  if (millis() - sleep_mode_millis >= (WAIT_TO_SLEEP*60)*1000 & !sleep_mode_flag & !DEVICE_MODE){
    sleep_mode_flag = true;
    lcd_sleep();
    load_cell_sleep();
  }else if (millis() - sleep_mode_millis < (WAIT_TO_SLEEP*60)*1000 & sleep_mode_flag){
    sleep_mode_flag = false;
    lcd_wake();
    load_cell_wake();
  }
}

void batt_loss_event(){
  if (get_batt_percent() < 20){
    battery_loss_counter++;
  }else{
    battery_loss_counter = 0;
    batt_loss_flag = false;
  }

  if (battery_loss_counter > 5){
    batt_loss_flag = true;
    alert_buzzer_millis = millis();
    alert_buzzer_flag = true;

    String noti_str = "แบตเตอรี่น้อยกว่า 20% กรุณาชาร์จแบต!";
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
  }

  if (batt_loss_flag){
    clear_display();
    display_custom("battery low!", 2, 0);
  }
}

void welcome_action(){
  // WiFi.begin("D", "11223344");
  clear_display();
  display_custom("welcome to", 3, 0);
  display_custom("I Care Urine", 2, 1);
  onBuzzer(0.05);
  delay(3000);

  if (!DEVICE_MODE){wm.setDebugOutput(false);}
  else{Serial.println("wait user config");}
  wm.setTimeout(300);
  wm.setConfigPortalBlocking(false);
  wm.autoConnect("I Care Urine");
  unsigned long time_out = millis();
  unsigned long abort_time_out = millis();

  clear_display();
  display_custom("Pls config wifi", 1, 0);
  display_custom("N: I Care Urine", 1, 1);

  while (true){
    wm.process();
    if (!is_btn_pressed()){abort_time_out = millis();} // wait for use press button
    if (is_device_online() || millis() - abort_time_out >= 2000 || millis() - time_out >= 300000){
      if (!is_device_online()){ // stop config portal if user abort
        wm.stopConfigPortal();
        clear_display();
        display_custom("offline mode", 2, 0);
        onBuzzer(1.5);

        // set notify time on start device
        if (DEVICE_MODE){
          next_hour_notify = 0;
          next_minute_notify = 1;
        }else{
          next_hour_notify = 1;
          next_minute_notify = 0;
        }
      } 
      else{ // play buzzer connected wifi
        clear_display();
        display_custom("WiFi connected", 1, 0);
        onBuzzer(0.05);
        onBuzzer(0.05);
        date_time_init();
        LINE.setToken(LINE_TOKEN_1);

        // set notify time on start device
        int time_array[3];
        String* dateTime = get_date_time();
        splitTimeString(dateTime[1], time_array);
        if (DEVICE_MODE){ // present mode noti every minute
          if (next_minute_notify < 59){
            next_hour_notify = time_array[0];
            next_minute_notify = time_array[1]+1;
          }else if (next_minute_notify == 59){
            next_minute_notify = 0;
            next_hour_notify = time_array[0]+1;
          }
        }else{ // normal mode noti every hour
          if (time_array[0] < 23 ){
            next_hour_notify = time_array[0]+1;
          }else if (time_array[0] == 23){
            next_hour_notify = 0;
          }
          next_minute_notify = time_array[1];
        }

        String timeForFirstNoti[2];
        if (next_hour_notify < 10){
          timeForFirstNoti[0] = "0" + String(next_hour_notify);
        }else{
          timeForFirstNoti[0] = String(next_hour_notify);
        }
        if (next_minute_notify < 10){
          timeForFirstNoti[1] = "0" + String(next_minute_notify);
        }else{
          timeForFirstNoti[1] = String(next_minute_notify);
        }
        LINE.notify("เครื่องพร้อมใช้งาน\nแจ้งเตือนครั้งต่อไปเวลา " + timeForFirstNoti[0] + ":" + timeForFirstNoti[1] + " น.");
        if (!is_device_online()){
          next_hour_notify = 0;
          next_minute_notify = 1;
        }
      } 
      break;
    }
  }
  delay(1000);
}

bool is_device_online(){
  return WiFi.status() == WL_CONNECTED;
}

void wait_for_sero_point(){
  clear_display();
  display_custom("Press button for", 0, 0);
  display_custom("Set Zero & Start", 0, 1);
  
  unsigned long time_out = millis();
  // Wait for Button Press to Set Zero
  while (!is_btn_pressed()) {
    if (millis() - time_out >= (5*60)*1000){break;}
  }
  
  onBuzzer(0.05);
  load_cell_set_zero();
}

// for define pin mode and set pin to init state
void pinMode_begin(){
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(ToggleBTN, INPUT);
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);
}

bool is_btn_pressed(){
  return digitalRead(ToggleBTN);
}

void onBuzzer(float second){
  digitalWrite(Buzzer, HIGH);
  delay(second*1000);
  digitalWrite(Buzzer, LOW);
  delay(second*1000);
}