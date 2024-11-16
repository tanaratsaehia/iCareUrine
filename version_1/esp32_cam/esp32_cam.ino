#include <WiFi.h>
#include "esp_camera.h"
#include <WiFiManager.h>
#include <TridentTD_LineNotify.h>

#define FLASH_GPIO_NUM    4

// #define LINE_TOKEN  "xlPCf7LajMbAHq0E9xNLST6gZlxnhm4BdaVIi9VwwxE" // TOKEN
#define LINE_TOKEN_1  "xlPCf7LajMbAHq0E9xNLST6gZlxnhm4BdaVIi9VwwxE" // TOKEN
#define LINE_TOKEN_2  "q0aSBfvTv3YoNrubTM8guFUdwctRQFYNFafgW9Gyy1m" // TOKEN
#define LINE_TOKEN_3  "0xVEI3flguwS2Q2aeTX0X3ZsFqTNIFkQpM1Y3c7pblB" // TOKEN
#define googleScriptID "https://script.google.com/macros/s/AKfycbwUHt-JbJW4xp3ubmpTMTsT3LXqvfvpHDDsUUgPyTGBO-lYmXIJhIjyjJyMFid4C-E/exec"

unsigned long millisCouting;
String record_file_name;
WiFiManager wm;

bool presentMode = false;

int nextHourRecord;
int nextMinuteRecord;
float previousUrineWeight;
bool firstTimeOpenDevice = true;
bool stateNotiEightHour = false;
int lineTokenCounter = 1;
bool checkRecStatus = false;
bool wifiState = false;

String* dateTime;
int timeIntArr[3];

void setup() {
  // Initialize Serial Communication
  Serial.begin(115200);

  // Initialize GPIO Pins
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);

  checkForPresentMode();

  // Initialize Wi-Fi
  initializeWiFi();

  Serial.println("Finish init wifi");
  // Initialize Camera, SD Card, and Time
  initializeHardware();

  // Initialize LINE Notification
  LINE.setToken(LINE_TOKEN_1);

  // Set Up Initial Recording Parameters
  setupRecordingParameters();

  // Clear Serial Buffer
  clearSerialBuffer();

  // Check and Prepare Record File
  prepareRecordFile();

  // Notify Ready State
  String timeForFirstNoti[2];
  if (nextHourRecord < 10){
    timeForFirstNoti[0] = "0" + String(nextHourRecord);
  }else{
    timeForFirstNoti[0] = String(nextHourRecord);
  }
  if (nextMinuteRecord < 10){
    timeForFirstNoti[1] = "0" + String(nextMinuteRecord);
  }else{
    timeForFirstNoti[1] = String(nextMinuteRecord);
  }
  LINE.notify("เครื่องพร้อมใช้งาน\nแจ้งเตือนครั้งต่อไปเวลา " + timeForFirstNoti[0] + ":" + timeForFirstNoti[1] + " น.");
}

void initializeWiFi() {
  // WiFiManager wm;
  wm.setDebugOutput(false);
  // wm.setConfigPortalBlocking(false);
  wm.setTimeout(300);
  
  // Attempt to connect to known Wi-Fi or open the config portal
  Serial.println("wait user config");
  handleConfigWiFi();
  if (!wm.autoConnect("I Care Urine")) {
    // If autoConnect fails, check if the user configured Wi-Fi or not
    if (WiFi.status() != WL_CONNECTED) {
      handleWiFiFail();
    }
  } else {
    // Wi-Fi was successfully connected without user intervention
    wifiState = true;
    handleWiFiConnected();
  }
}

void checkForPresentMode(){
  // waitForCommand("present_mode", "present_ok");
  while (true){
    if (Serial.available()){
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      if (cmd.startsWith("c_")) {
        cmd = cmd.substring(2);
        if (cmd == "present_mode") {
          presentMode = true;
          Serial.println("c_present_ok");
          delay(1500);
          Serial.println("c_present_ok");
          delay(1500);
          Serial.println("c_present_ok");
          break;
        }
      }else if (cmd = "hi"){
        break;
      }
    }
  }
}

void handleWiFiConnected() {
  waitForCommand("connected_ok", "c_wifi_connected");
  // Serial.println("c_wifi_connected");
}

void handleConfigWiFi() {
  waitForCommand("config_ok", "c_config_wifi");
  // Serial.println("c_config_wifi");
}

void handleWiFiFail() {
  waitForCommand("fail_ok", "c_wifi_fail");
  // Serial.println("c_config_wifi");
}

void waitForCommand(const String& expectedCommand, const String& statusMessage) {
  unsigned long waittingMillis = millis();
  while (true) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      if (cmd.startsWith("c_")) {
        cmd = cmd.substring(2);
        cmd.trim();
        if (cmd == expectedCommand) {
          // Serial.println("here");
          // break;
          return;
        }
      }
    }
    if (millis() - waittingMillis >= 1500){
      waittingMillis = millis();
      Serial.println(statusMessage);
    }
    // delay(1500);
  }
}

void initializeHardware() {
  // Serial.println("right here 1");
  esp_cam_init();
  sd_card_init();
  date_time_init();
}

void setupRecordingParameters() {
  dateTime = get_date_time();
  splitTimeString(dateTime[1], timeIntArr);

  if (presentMode) {
    if (timeIntArr[1] == 59){
      // nextHourRecord = timeIntArr[0]+1;
      // nextMinuteRecord = 5;
      nextHourRecord = timeIntArr[0]+1;
      nextMinuteRecord = 0;
    }else{
      nextHourRecord = timeIntArr[0];
      // nextMinuteRecord = timeIntArr[1] + 5;
      nextMinuteRecord = timeIntArr[1] + 1;
    }
  } else {
    if (timeIntArr[0] == 23){
      nextHourRecord = 0;
    }else{
      nextHourRecord = timeIntArr[0] + 1;
    }
    nextMinuteRecord = timeIntArr[1];
  }

  record_file_name = "/" + dateTime[0] + ".csv";
  // Serial.println(record_file_name);
  // record_file_name = dateTime[0] + ".csv";
}

void clearSerialBuffer() {
  while (Serial.available()) {
    String temp = Serial.readStringUntil('\n');
  }
}

void prepareRecordFile() {
  check_record(record_file_name);
  readAndResendCSVRecords(record_file_name.c_str());
}

unsigned long lossWifiPrintMillis;
unsigned long lossWifiReconntMillis;

void loop() {
  if (timeIntArr[1] % 5 == 0 & !checkRecStatus){
    checkRecStatus == true;
    if (is_file_exist(record_file_name)){
      readAndResendCSVRecords(record_file_name.c_str());
    }
  }else if (timeIntArr[1] % 5 > 0 & checkRecStatus){
    checkRecStatus == false;
  }
  if (WiFi.status() != WL_CONNECTED & millis() - lossWifiPrintMillis >= 2000){
    lossWifiPrintMillis = millis();
    Serial.println("c_wifi_loss");
    wifiState = false;
  }else if (WiFi.status() == WL_CONNECTED & !wifiState){
    wifiState = true;
    handleWiFiConnected();
    // on_flash();
    // delay(50);
    // off_flash();
    readAndResendCSVRecords(record_file_name.c_str());
  }
  if (WiFi.status() != WL_CONNECTED & millis() - lossWifiReconntMillis >= 4500){
    lossWifiReconntMillis = millis();
    WiFi.begin();
  }
  
  if (Serial.available()){
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("c_")){
      command = command.substring(2);
      command.trim();
      if (command == "batt_loss"){
        reconnectWiFi();
        LINE.notify("แบตเตอรี่น้อยกว่า 20% กรุณาชาร์จแบต");
        delay(60000);
        return;
      }
    }
  }

  if (firstTimeOpenDevice){
    firstTimeOpenDevice = false;
    Serial.println("c_wake_up");
    float weight = get_weight();
    previousUrineWeight = weight;
  }

  dateTime = get_date_time();
  splitTimeString(dateTime[1], timeIntArr);
  String file_name = getFileName(record_file_name);
  if (dateTime[0]  != file_name){
    Serial.println("change file name");
    Serial.println(file_name);
    record_file_name = "/" + dateTime[0] + ".csv";
  }
  if ((timeIntArr[0] == nextHourRecord & timeIntArr[1] == nextMinuteRecord) || (timeIntArr[0] >= nextHourRecord & timeIntArr[1] >= nextMinuteRecord & nextHourRecord != 0)){
    if (lineTokenCounter == 1){
      LINE.setToken(LINE_TOKEN_1);
    }else if (lineTokenCounter == 2){
      LINE.setToken(LINE_TOKEN_2);
    }else if (lineTokenCounter == 3){
      LINE.setToken(LINE_TOKEN_3);
    }
    // nextHourRecord++;
    if (presentMode){
      if (nextMinuteRecord < 59){
        // nextMinuteRecord+=5;
        nextMinuteRecord++;
      }else if (nextMinuteRecord == 59){
        // nextMinuteRecord = 5;
        // nextHourRecord++;
        nextMinuteRecord = 0;
        nextHourRecord++;
      }
    }else{
      if (nextHourRecord < 23){
        nextHourRecord++;
      }else if (nextHourRecord == 23){
        nextHourRecord = 0;
      }
    }
    
    Serial.println("c_wake_up");
    camera_fb_t* pic = get_picture(false);
    delay(250);
    float weight = get_weight();
    String pathImg = "/"+dateTime[0]+"_"+dateTime[1]+".jpg";
    String data = dateTime[0]+","+dateTime[1]+","+String(weight)+","+pathImg;
    if (!save_image_to_sd(pic, pathImg.c_str())) {
      if (!save_image_to_sd(pic, pathImg.c_str())){
        // Serial.println("can't save image");
        data = dateTime[0]+","+dateTime[1]+","+String(weight)+",N/A";
      }
    }
    String notiStr;
    if (weight - previousUrineWeight < 30){
      // on buzzer here 
      alarnByBuzzer();
      notiStr = "\n!!! ปริมาณปัสสาวะน้อยกว่า 30 มล. !!!\nชั่วโมงที่แล้ว: "+String(previousUrineWeight)+" มล.\nปัจจุบัน: "+String(weight)+" มล.";
    }else{
      notiStr = "\nปริมาณปัสสาวะปกติ\nชั่วโมงที่แล้ว: "+String(previousUrineWeight)+" มล.\nปัจจุบัน: "+String(weight)+" มล.";
    }
    bool gg_res = false;
    bool line_res = false;
    if (WiFi.status() == WL_CONNECTED){
      reconnectWiFi();
      line_res = LINE.notify(notiStr);
      line_res = LINE.notifyPicture(pic->buf, pic->len);
      // Serial.println("noti Done");
      // gg_res = sendToGoogleSheets(data);
    }
    if (gg_res){
      if (line_res){
        data = dateTime[0]+","+dateTime[1]+","+String(weight)+","+pathImg+",1,1";
      }else{
        data = dateTime[0]+","+dateTime[1]+","+String(weight)+","+pathImg+",0,1";
      }
    }else{
      if (line_res){
        data = dateTime[0]+","+dateTime[1]+","+String(weight)+","+pathImg+",1,1";
      }else{
        lineTokenCounter++;
        data = dateTime[0]+","+dateTime[1]+","+String(weight)+","+pathImg+",0,1";
      }
      // LINE.notify("can't upload data into google sheet");
    }
    // Serial.println("gg res"+String(gg_res));
    write_CSV(record_file_name.c_str(), data);
    // Serial.println("Write file Done");
    previousUrineWeight = weight;
    esp_camera_fb_return(pic);
  }

  if ((timeIntArr[0] == 6 | timeIntArr[0] == 14 | timeIntArr[0] == 22) & timeIntArr[1] == 0 & !stateNotiEightHour){
    stateNotiEightHour = true;
    Serial.println("c_wake_up");
    camera_fb_t* pic = get_picture(false);
    delay(250);
    float weight = get_weight();
    String notiStr = "นี่คือข้อความรายงานผลของปริมาณปัสสาวะตามเวลาเปลี่ยนเวร\nปริมาณปัสสาวะปัจจุบัน: "+String(weight)+" มล.";
    bool line_res = false;
    if (WiFi.status() == WL_CONNECTED){
      line_res = LINE.notifyPicture(notiStr, pic->buf, pic->len);
      int existCounter = 0;
      while (!line_res){
        existCounter++;
        lineTokenCounter++;
        if (lineTokenCounter == 1){
          LINE.setToken(LINE_TOKEN_1);
        }else if (lineTokenCounter == 2){
          LINE.setToken(LINE_TOKEN_2);
        }else if (lineTokenCounter == 3){
          LINE.setToken(LINE_TOKEN_3);
        }
        reconnectWiFi();
        line_res = LINE.notifyPicture(notiStr, pic->buf, pic->len);
        if (existCounter >= 10){
          break;
        }
      }
    }
    esp_camera_fb_return(pic);
  }
  if (timeIntArr[1] == 58){
    stateNotiEightHour = false;
  }
  if (timeIntArr[1] % 5 ==0){
    readAndResendCSVRecords(record_file_name.c_str());
  }
}


String getFileName(String filePath) {
  int lastSlashIndex = filePath.lastIndexOf('/');
  int dotIndex = filePath.lastIndexOf('.');
  
  if (dotIndex == -1) {
    // No extension found
    dotIndex = filePath.length();
  }
  
  String fileName = filePath.substring(lastSlashIndex + 1, dotIndex);
  return fileName;
}

void reconnectWiFi() {
  WiFi.disconnect();
  delay(1000);
  WiFi.begin();
  unsigned long timeOutMillis = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - timeOutMillis >= 10000){break;}
  }
}

void alarnByBuzzer(){
  unsigned long timeOutMillis = millis();
  unsigned long printMillis = millis();
  String data;

  while (true){
    if (Serial.available()){
      data = Serial.readStringUntil('\n');
      if (data.startsWith("c_buz_ok")){
        return;
      }
    }
    if (millis() - printMillis >= 2000){
      printMillis = millis();
      Serial.println("c_alert_buz");
    }
    if (millis() - timeOutMillis >= 6500){break;}
  }
}

