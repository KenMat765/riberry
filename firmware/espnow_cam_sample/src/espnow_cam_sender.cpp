/**************************************************
 * ESPNowCam video Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
 - AtomS3RCam
 https://github.com/m5stack/M5AtomS3/blob/main/examples/Basics/camera/camera.ino
 https://github.com/m5stack/M5AtomS3/blob/main/examples/Basics/camera/camera_pins.h
**************************************************/

#include <Arduino.h>
#include <esp_camera.h>
#include <ESPNowCam.h>
#include <Utils.h>

ESPNowCam radio;
camera_fb_t* fb;

bool has_psram = false;

#define POWER_GPIO_NUM 18

// Please change this to your Camera pins:
camera_config_t camera_config = {
    .pin_pwdn     = -1,
    .pin_reset    = -1,
    .pin_xclk     = 21,
    .pin_sscb_sda = 12,
    .pin_sscb_scl = 9,
    .pin_d7       = 13,
    .pin_d6       = 11,
    .pin_d5       = 17,
    .pin_d4       = 4,
    .pin_d3       = 48,
    .pin_d2       = 46,
    .pin_d1       = 42,
    .pin_d0       = 3,

    .pin_vsync = 10,
    .pin_href  = 14,
    .pin_pclk  = 40,
    
    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format  = PIXFORMAT_RGB565,
    .frame_size    = FRAMESIZE_QVGA,
    // FRAMESIZE_96X96,    // 96x96 - OK
    // FRAMESIZE_QQVGA,    // 160x120 - OK
    // FRAMESIZE_QCIF,     // 176x144 - OK
    // FRAMESIZE_HQVGA,    // 240x176 - OK
    // FRAMESIZE_240X240,  // 240x240 - OK
    // FRAMESIZE_QVGA,     // 320x240 - OK

    .jpeg_quality  = 0,
    .fb_count      = 2,
    .fb_location   = CAMERA_FB_IN_PSRAM,
    .grab_mode     = CAMERA_GRAB_LATEST,
    .sccb_i2c_port = 0,
};

bool CameraBegin() {
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    return false;
  }

  // Add
  sensor_t *s = esp_camera_sensor_get();
  s->set_hmirror(s, 1);        // 左右反転
  s->set_vflip(s, 0); //上下反転 0無効 1有効

  //カメラ追加設定
  // sensor_t * s = esp_camera_sensor_get();
  // s->set_hmirror(s, 1); //左右反転 0無効 1有効
  // s->set_vflip(s, 1); //上下反転 0無効 1有効
  // s->set_colorbar(s, 1); //カラーバー 0無効 1有効
  // s->set_brightness(s, 1);  // up the brightness just a bit
  // s->set_saturation(s, 0);  // lower the saturation

  return true;
}

bool CameraGet() {
  fb = esp_camera_fb_get();
  if (!fb) {
    return false;
  }
  return true;
}

bool CameraFree() {
  if (fb) {
    esp_camera_fb_return(fb);
    return true;
  }
  return false;
}

void processFrame() {
  if (CameraGet()) {
    if (has_psram) {
      uint8_t *out_jpg = NULL;
      size_t out_jpg_len = 0;
      frame2jpg(fb, 12, &out_jpg, &out_jpg_len);
      radio.sendData(out_jpg, out_jpg_len);
      free(out_jpg);
    }
    else{
      radio.sendData(fb->buf, fb->len);
      delay(30); // ==> weird delay for cameras without PSRAM
    }
    printFPS("CAM:");
    CameraFree();
  }
}

void setup() {
  USBSerial.begin(115200);

  // add - これが無いと動かなかった
  pinMode(POWER_GPIO_NUM, OUTPUT);
  digitalWrite(POWER_GPIO_NUM, LOW);
  delay(500);  

  delay(1000); // only for debugging 

  if(psramFound()){
    has_psram = true;
    size_t psram_size = esp_spiram_get_size() / 1048576;
    USBSerial.printf("PSRAM size: %dMb\r\n", psram_size);
    // suggested config with PSRAM
    camera_config.pixel_format = PIXFORMAT_RGB565;
    camera_config.fb_location = CAMERA_FB_IN_PSRAM;
    camera_config.fb_count = 2;
    digitalWrite(POWER_GPIO_NUM, LOW);   // LED ON
  }
  else{
    USBSerial.println("PSRAM not found! Basic framebuffer setup.");
    digitalWrite(POWER_GPIO_NUM, HIGH);  // LED OFF
  }
  
  radio.init();

  if (!CameraBegin()) {
    USBSerial.println("Camera Init Fail");
    delay(1000);
    ESP.restart();
  }
  delay(500);

  USBSerial.println("Sender setup Complete!!");
}

void loop() {
  processFrame();
}
