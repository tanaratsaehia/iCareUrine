#include <HTTPClient.h>

bool sendToGoogleSheets(String data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(googleScriptID);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String httpRequestData = "data=" + data;
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode); // Print return code
      Serial.println(response);         // Print request answer
      http.end();
      return true;
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
      http.end();
      return false;
    }
  } else {
    // Serial.println("Error in WiFi connection");
    return false;
  }
}