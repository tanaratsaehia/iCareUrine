#include <time.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

void date_time_init(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

String* get_date_time() {
  static String result[2]; // Static array to hold the date and time

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    date_time_init();
    if (getLocalTime(&timeinfo)){
      break;
    }
    // return result;
  }

  char dateStringBuff[20]; // Buffer to hold the date
  strftime(dateStringBuff, sizeof(dateStringBuff), "%d-%B-%Y", &timeinfo);
  
  char timeStringBuff[10]; // Buffer to hold the time
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H-%M-%S", &timeinfo);
  
  result[0] = String(dateStringBuff);
  result[1] = String(timeStringBuff);
  
  return result;
}

void splitTimeString(String timeStr, int* timeArr) {
  int firstColon = timeStr.indexOf('-');
  int secondColon = timeStr.indexOf('-', firstColon + 1);

  // Extract hours, minutes, and seconds
  String hours = timeStr.substring(0, firstColon);
  String minutes = timeStr.substring(firstColon + 1, secondColon);
  String seconds = timeStr.substring(secondColon + 1);

  // Convert to integers and store in the array
  timeArr[0] = hours.toInt();
  timeArr[1] = minutes.toInt();
  timeArr[2] = seconds.toInt();
}