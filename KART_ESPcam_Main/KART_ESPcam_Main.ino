#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Arduino.h>

#define ONBOARD_LED 4
#define CHUNK_LENGTH 1024  // 최적화된 UDP 패킷 크기 (MTU 크기에 맞게 조정)

// 기본 Wi-Fi 설정
char ssid[32] = "a12";           // 기본 Wi-Fi SSID
char password[32] = "12345678";  // 기본 Wi-Fi 비밀번호

// UDP 서버 설정
char udpAddress[16] = "192.168.137.55";  // PC의 로컬 IP 주소
int udpPort = 4222;                    // UDP 서버 포트

WiFiUDP udp;

// 카메라 설정 초기화
camera_config_t config;

void connecteWIFI() {
  WiFi.mode(WIFI_STA);               // 스테이션 모드 설정
  WiFi.setHostname("ESP32CAM_001");  // 호스트네임 설정
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime >= 10000) {
      Serial.println("WiFi connection failed!");
      return;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectCAM() {
  // 카메라 설정
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
  config.jpeg_quality = 10;            // JPEG 품질 (0: 최고 화질, 63: 최저 화질)
  config.fb_count = 2;

  // 카메라 초기화
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera init failed!");
    return;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ONBOARD_LED, OUTPUT);  // 내장 LED 핀 설정
  connecteWIFI();
  udp.begin(udpPort);  // UDP 시작
  connectCAM();
  
  // 기본 설정 출력
  Serial.println("ESP32CAM Ready.");
  Serial.println("You can modify the following parameters:");
  Serial.println("1. SSID (Wi-Fi)");
  Serial.println("2. Password (Wi-Fi)");
  Serial.println("3. UDP Server Address");
  Serial.println("4. UDP Server Port");
  Serial.println("5. Camera Settings (Quality, Frame Size, etc.)");
}

void loop() {
  // 사용자 입력 대기
  if (Serial.available()) {
    String input = Serial.readString()
    input.trim();
    
    if (input.startsWith("ssid")) {
      input.replace("ssid", "");
      

      input.toCharArray(ssid, input.substring(4).length() + 1);
      Serial.print("New SSID: ");
      Serial.println(ssid);
      WiFi.disconnect();
      WiFi.begin(ssid, password);  // 새로운 Wi-Fi 연결
    }

    if (input.startsWith("pass")) {
      input.replace("pass", "");
      input.trim();
      input.toCharArray(password, input.length() + 1);
      Serial.print("New Password: ");
      Serial.println(password);
      WiFi.disconnect();
      WiFi.begin(ssid, password);  // 새로운 비밀번호로 연결
    }

    if (input.startsWith("ip")) {
      input.replace("ip", "");
      input.trim();
      input.toCharArray(udpAddress, input.length() + 1);
      Serial.print("New UDP Server ip: ");
      Serial.println(udpAddress);
    }

    if (input.startsWith("port")) {
      input.replace("port", "");
      input.trim();
      udpPort = input.toInt();
      Serial.print("New UDP Server Port: ");
      Serial.println(udpPort);
    }

    if (input.startsWith("qual")) {
      input.replace("qual", "");
      input.trim();
      config.jpeg_quality = input.toInt();
      Serial.print("New Camera Quality: ");
      Serial.println(config.jpeg_quality);
    }

    if (input.startsWith("size")) {
      input.replace("size", "");
      input.trim();
      if (input == "QVGA") {
        config.frame_size = FRAMESIZE_QVGA;  // 320x240
      } else if (input == "VGA") {
        config.frame_size = FRAMESIZE_VGA;  // 640x480
      } else {
        Serial.println("Unknown frame size");
        return;
      }
      Serial.print("New Camera Frame Size: ");
      Serial.println(input);
    }

    if (input.startsWith("led")) {
      digitalWrite(ONBOARD_LED, !digitalRead(ONBOARD_LED));  // LED 토글
      Serial.println("Toggled LED");
    }

    // Wi-Fi 재연결
    if (WiFi.status() != WL_CONNECTED) {
      connecteWIFI();
    }
  }

  // 카메라에서 프레임 캡처
  camera_fb_t* fb = esp_camera_fb_get();  // 이미지 캡처
  if (!fb) {
    Serial.println("Camera capture failed!");
    return;
  }

  // 메모리 관리 - 불필요한 메모리 해제
  sendPacketData(fb->buf, fb->len);  // UDP 전송
  esp_camera_fb_return(fb);          // 메모리 반환
}

void sendPacketData(const uint8_t* buf, size_t len) {
  size_t chunks = len / CHUNK_LENGTH;
  size_t remaining = len % CHUNK_LENGTH;

  for (size_t i = 0; i < chunks; i++) {
    // UDP 패킷 전송
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buf + (i * CHUNK_LENGTH), CHUNK_LENGTH);
    udp.endPacket();
  }

  if (remaining > 0) {
    // 남은 데이터 전송
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buf + (len - remaining), remaining);
    udp.endPacket();
  }
}
