#include "esp_camera.h"
#include <WiFi.h>
#include <Wire.h>
// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"
#include "io_extension.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "WSTEST";
const char *password = "waveshare0755";

void startCameraServer();
void setupLedFlash();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  
  DEV_I2C_Init();
  IO_EXTENSION_Init();
  IO_EXTENSION_Output(IO_EXTENSION_IO_6, 1);

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

//   // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
//   //                      for larger pre-allocated frame buffer.
//   if (config.pixel_format == PIXFORMAT_JPEG) {
//     if (psramFound()) {
//       config.jpeg_quality = 10;
//       config.fb_count = 2;
//       config.grab_mode = CAMERA_GRAB_LATEST;
//     } else {
//       // Limit the frame size when PSRAM is not available
//       config.frame_size = FRAMESIZE_SVGA;
//       config.fb_location = CAMERA_FB_IN_DRAM;
//     }
//   } else {
//     // Best option for face detection/recognition
//     config.frame_size = FRAMESIZE_240X240;
// #if CONFIG_IDF_TARGET_ESP32S3
//     config.fb_count = 2;
// #endif
//   }


  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    esp_camera_deinit();
    Serial.printf("Camera init failed with error 0x%x", err);
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_RGB565;
    esp_camera_init(&config);
  }

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}
