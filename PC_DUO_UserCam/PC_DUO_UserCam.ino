#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#define CHUNK_LENGTH 1460  // UDP 패킷 크기 (최대 MTU 제한)

// Wi-Fi 설정
const char* ssid = "a12";      // Wi-Fi SSID
const char* password = "12345678";  // Wi-Fi 비밀번호

// UDP 서버 설정
const char* udpAddress = "192.168.55.41"; // PC의 로컬 IP 주소
const int udpPort = 4222;                 // UDP 서버 포트

WiFiUDP udp;

void setup() {
  Serial.begin(115200);

  // Wi-Fi 연결
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  udp.begin(udpPort);  // UDP 시작

  // 카메라 설정
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;

  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;

  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;  // 해상도: 320x240
  config.jpeg_quality = 10;  // JPEG 품질 (0: 최고 화질, 63: 최저 화질)
  config.fb_count = 2;

  // 카메라 초기화
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    return;
  }
}

void loop() {
  camera_fb_t* fb = esp_camera_fb_get(); // 이미지 캡처

  if (!fb) {
    Serial.println("Camera capture failed!");
    return;
  }

  sendPacketData(fb->buf, fb->len); // UDP 전송
  esp_camera_fb_return(fb); // 메모리 반환
}

void sendPacketData(const uint8_t* buf, size_t len) {
  size_t chunks = len / CHUNK_LENGTH;
  size_t remaining = len % CHUNK_LENGTH;

  for (size_t i = 0; i < chunks; i++) {
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buf + (i * CHUNK_LENGTH), CHUNK_LENGTH);
    udp.endPacket();
  }

  if (remaining > 0) {
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buf + (len - remaining), remaining);
    udp.endPacket();
  }
}
