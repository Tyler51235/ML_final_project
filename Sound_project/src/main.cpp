#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "camera_pins.h"   

// ------------------------------------------------------
// USER SETTINGS  (EDIT THESE ONLY)
// ------------------------------------------------------
const char* ssid       = "YOUR_WIFI_SSID";
const char* password   = "YOUR_WIFI_PASSWORD";
const char* BOT_TOKEN  = "8539385739:AAH0WWQpQA7L9CW6SLbFU2p3dDgNUNSVDqs";    // from BotFather
const char* CHAT_ID    = "7318781359";                  // your chat ID

// Placeholder for ML logic 
bool dangerDetected = true;    // <--- Replace with ML result

bool sent = false;

// ------------------------------------------------------
// WiFi Setup
// ------------------------------------------------------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection FAILED!");
  }
}

// ------------------------------------------------------
// Camera Setup (XIAO ESP32-S3)
// ------------------------------------------------------
void initCamera() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  Serial.println("Initializing camera...");
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("Camera init FAILED: 0x%x\n", err);
  } else {
    Serial.println("Camera init OK!");
  }
}

// ------------------------------------------------------
// Send Photo to Telegram
// ------------------------------------------------------
bool sendTelegramPhoto(const char* caption) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    return false;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed.");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  String boundary = "----ESP32CamBoundary";
  String startReq =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
    String(CHAT_ID) + "\r\n--" + boundary + "\r\n" +
    "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" +
    String(caption) + "\r\n--" + boundary + "\r\n" +
    "Content-Disposition: form-data; name=\"photo\"; filename=\"alert.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n\r\n";

  String endReq = "\r\n--" + boundary + "--\r\n";

  String header =
    "POST /bot" + String(BOT_TOKEN) + "/sendPhoto HTTP/1.1\r\n" +
    "Host: api.telegram.org\r\n" +
    "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n" +
    "Content-Length: " + String(startReq.length() + fb->len + endReq.length()) + "\r\n\r\n";

  Serial.println("Connecting to Telegram...");
  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Connection to Telegram FAILED.");
    esp_camera_fb_return(fb);
    return false;
  }

  client.print(header);
  client.print(startReq);
  client.write(fb->buf, fb->len);
  client.print(endReq);

  esp_camera_fb_return(fb);

  unsigned long timeout = millis();
  String response = "";
  while (millis() - timeout < 5000) {
    while (client.available()) response += (char)client.read();
  }

  Serial.println("Telegram response:");
  Serial.println(response);

  return response.indexOf("\"ok\":true") != -1;
}

// ------------------------------------------------------
// MAIN
// ------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Booting Security Camera System...");
  connectWiFi();
  initCamera();
}

void loop() {
  // dangerDetected = (ML model result);

  if (dangerDetected && !sent) {
    Serial.println("DANGER DETECTED — sending alert to user...");
    sendTelegramPhoto("⚠️ ALERT: Dangerous sound detected!");
    sent = true;
  }

  delay(1000);
}
