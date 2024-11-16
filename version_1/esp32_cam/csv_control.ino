#include <FS.h>
#include <SD_MMC.h>
#include "esp_camera.h"

void sd_card_init(){
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  // uint64_t totalBytes = SD_MMC.totalBytes();
  // uint64_t usedBytes = SD_MMC.usedBytes();

  // if (usedBytes > (totalBytes * 0.95)) {
  //   Serial.println("SD Card is almost full, formatting...");
  //   if (SD_MMC.format()) {
  //     Serial.println("SD Card formatted successfully");
  //   } else {
  //     Serial.println("Failed to format the SD Card");
  //   }
  // } else {
  //   Serial.println("Sufficient space available on the SD Card.");
  // }
}

void write_CSV(const char* filename, String data) {
  // Serial.println("File : " + String(filename));
  File file = SD_MMC.open(filename, FILE_APPEND);
  if (!file) {
    // Serial.println("Failed to open file for appending");
    return;
  }
  if (file.println(data)) {
    // Serial.println("Data written successfully");
  } else {
    // Serial.println("Write failed");
  }
  file.close();
}

void read_CSV(const char* filename) {
  File file = SD_MMC.open(filename);
  if (!file) {
    // Serial.println("Failed to open file for reading");
    return;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    // Serial.println(line);
  }
  file.close();
}

void edit_CSV(const char* filename, String condition, String newValue) {
  File file = SD_MMC.open(filename, FILE_READ);
  if (!file) {
    // Serial.println("Failed to open file for reading");
    return;
  }

  String fileContent = "";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    String timestamp = line.substring(0, line.indexOf(',')); 
    if (timestamp == condition) {
      int lastCommaIndex = line.lastIndexOf(',');
      line = line.substring(0, lastCommaIndex + 1) + newValue; 
    }
    fileContent += line + "\n";
  }
  file.close();

  // Write modified content back to file
  file = SD_MMC.open(filename, FILE_WRITE);
  if (!file) {
    // Serial.println("Failed to open file for writing");
    return;
  }
  file.print(fileContent);
  file.close();

  // Serial.println("Modified CSV file and updated the last column based on the condition.");
}

// void record_data(const char* filename, String data){
//   if (is_file_exist(filename)){
//     // read csv
//   }else{
//     write_CSV(filename, data);
//   }
// }

void check_record(const String filename){
  unsigned long waittingMillis = millis();
  if (is_file_exist(filename)){
    Serial.println("c_file_already_exist");

    while (true){
      if (Serial.available()){
        String command = Serial.readStringUntil('\n');
        if (command.startsWith("c_")){
          command = command.substring(2);
          command.trim();
          
          if (command == "delete_file"){
            SD_MMC.remove(filename);
            // Serial.println("remove file");
            break;
          }else if (command == "keep_file"){
            // do nothing
            // Serial.println("keep file");
            break;
          }
        }
      }

      if (millis() - waittingMillis >= 2000){
        waittingMillis = millis();
        Serial.println("c_file_already_exist");
      }
    }
  }else{
    Serial.println("c_file_created");
    delay(2000);
    Serial.println("c_file_created");
    // delay(2000);
  }
}

bool is_file_exist(const String filename) {
  // Check if the file exists
  if (SD_MMC.exists(filename)) {
    // Serial.println("File exists.");

    // Check if the file extension is ".csv"
    if (filename.endsWith(".csv")) {
      // Serial.println("File is a CSV file.");
      return true;
    } else {
      // Serial.println("File is not a CSV file.");
      return false;
    }
  } else {
    // Serial.println("File does not exist.");
    return false;
  }
}

bool delete_csv_file(const String filename){
  if (SD_MMC.exists(filename)) {
    // Serial.println("File exists.");
    // Attempt to delete the file
    if (SD_MMC.remove(filename)) {
      // Serial.println("File deleted.");
      return true;
    } else {
      // Serial.println("File deletion failed.");
      return false;
    }
  } else {
    // Serial.println("File does not exist.");
    return false;
  }
}

void readAndResendCSVRecords(const char* filename) {
  File file = SD_MMC.open(filename);
  if (!file) {
    // Serial.println("Failed to open file for reading " + String(filename));
    return;
  }

  std::vector<String> lines;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      lines.push_back(line);
    }
  }
  file.close();

  bool modified = false;

  for (auto& line : lines) {
    int index1 = line.indexOf(',');
    int index2 = line.indexOf(',', index1 + 1);
    int index3 = line.indexOf(',', index2 + 1);
    int index4 = line.indexOf(',', index3 + 1);
    int index5 = line.indexOf(',', index4 + 1);
    int index6 = line.length();

    String date = line.substring(0, index1);
    String time = line.substring(index1 + 1, index2);
    String weight = line.substring(index2 + 1, index3);
    String pathPic = line.substring(index3 + 1, index4);
    String lineNotiStatus = line.substring(index4 + 1, index5);
    String ggSheetUploadStatus = line.substring(index5 + 1, index6);

    if (lineNotiStatus == "0" || ggSheetUploadStatus == "0") {
      camera_fb_t* pic = read_image_from_sd(pathPic); // Read image from SD card
      
      if (pic != nullptr) {
        // String notiStr = "\nCurrent weight: " + weight + " ml\n";
        String notiStr = "นี่คือข้อความที่ถูกโหลดมาแจ้งเตือนเนื่องจากก่อนหน้านี้เน็ตมีปัญหา\nวันที่ "+date+" \nเวลา "+time+"\nปัสสาวะที่วัดได้: "+weight + " มล.";
        bool gg_res = false;
        bool line_res = false;

        if (WiFi.status() == WL_CONNECTED) {
          reconnectWiFi();
          line_res = LINE.notifyPicture(notiStr, pic->buf, pic->len);
          // line_res = LINE.notifyPicture(notiStr, pic->buf, pic->len);
          // gg_res = sendToGoogleSheets(date + "," + time + "," + weight + "," + pathPic + "," + lineNotiStatus + "," + ggSheetUploadStatus);
        }

        if (gg_res) {
          if (line_res) {
            line = date + "," + time + "," + weight + "," + pathPic + ",1,1";
          } else {
            line = date + "," + time + "," + weight + "," + pathPic + ",0,1";
          }
        } 
        // else {
        //   if (line_res) {
        //     line = date + "," + time + "," + weight + "," + pathPic + ",1,0";
        //   } else {
        //     line = date + "," + time + "," + weight + "," + pathPic + ",0,0";
        //   }
        // }
        else {
          if (line_res) {
            line = date + "," + time + "," + weight + "," + pathPic + ",1,1";
          } else {
            lineTokenCounter++;
            line = date + "," + time + "," + weight + "," + pathPic + ",0,1";
          }
        }
        free(pic->buf); // Free the allocated buffer
        free(pic);      // Free the pic structure

        modified = true;
      } else {
        Serial.println("Failed to read image from SD card");
      }
    }
  }

  if (modified) {
    // Rewrite the entire CSV file with updated data
    file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }

    for (const auto& line : lines) {
      file.println(line);
    }
    file.close();
  }
}

void add_initial_data(const char* filename) {
  // csv format date,time,weight,path_pic,upload_status
  write_CSV(filename, "17-July-2024,11-36-44,330.5,'/picture5588.jpg,0,0");
  write_CSV(filename, "17-July-2024,11-36-44,330.5,'/picture5588.jpg,0,0");
}