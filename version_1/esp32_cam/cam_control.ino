#include <FS.h>
#include <SD_MMC.h>
#include "esp_camera.h"
#include "img_converters.h"

// Camera configuration
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_GPIO_NUM    4

void esp_cam_init(){
  // Initialize the camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  // config.frame_size = FRAMESIZE_SVGA;  // 800x600 
  config.frame_size = FRAMESIZE_UXGA;  // 1600x1200
  config.jpeg_quality = 35;
  config.fb_count = 1;

  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();
  // s->set_vflip(s, 1);  // 1 to flip the image vertically
  // s->set_hmirror(s, 1);  // 1 to flip the image horizontally
}

camera_fb_t* get_picture(bool flash){
  camera_fb_t * fb; 
  if (flash){
    on_flash();
  }
    
  for (int i=0; i<=1; i++){
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
  }

  if (!fb) {
    Serial.println("Camera capture failed");
    off_flash();
    return fb;
  }
  off_flash();
  return fb;
}

bool save_image_to_sd(camera_fb_t * fb, const char * path) {
  fs::FS &fs = SD_MMC; 
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
      // Serial.println("Failed to open file for writing");
      return false;
  }else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    // Serial.printf("Saved file to path: %s\n", path);
  }
  file.close();
  // Serial.println("File written");
  return true;
}

camera_fb_t* read_image_from_sd(String path) {
  File file = SD_MMC.open(path);
  if (!file) {
    Serial.println("Failed to open image file");
    return nullptr;
  }

  size_t fileSize = file.size();
  uint8_t* buffer = (uint8_t*)malloc(fileSize);
  if (!buffer) {
    Serial.println("Failed to allocate memory");
    file.close();
    return nullptr;
  }

  file.read(buffer, fileSize);
  file.close();

  camera_fb_t* fb = (camera_fb_t*)malloc(sizeof(camera_fb_t));
  if (!fb) {
    Serial.println("Failed to allocate memory for fb");
    free(buffer);
    return nullptr;
  }

  fb->width = 1600; // You may need to set appropriate values based on your image format
  fb->height = 1200;
  fb->buf = buffer;
  fb->len = fileSize;
  fb->format = PIXFORMAT_JPEG; // Adjust if necessary

  return fb;
}

void on_flash(){
  digitalWrite(FLASH_GPIO_NUM, HIGH);
}

void off_flash(){
  digitalWrite(FLASH_GPIO_NUM, LOW);
}