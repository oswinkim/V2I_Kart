#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "HardwareSerial.h"
#include "Adafruit_TCS34725.h"

// WiFi 설정
WiFiUDP udp;
const char* ssid = "a12";
const char* password = "12345678";
const unsigned int recvPort = 4212;
const unsigned int sendPort = 4213;
IPAddress PC1_IP(192, 168, 0, 7);
int aa = 0;

// 모터 설정
#define MOTOR_A_IN1 25
#define MOTOR_A_IN2 26
#define MOTOR_B_IN1 32
#define MOTOR_B_IN2 33
const int freq = 1000;
const int resolution = 8;
int MOTOR_A_state = 250;
int MOTOR_B_state = 250;

// AHRS 설정
#define MAX_LINE_LENGTH 64
HardwareSerial AHRS_Serial(2);
float Roll = 0, Pitch = 0, Yaw = 0, start_yaw = 0;
char line[MAX_LINE_LENGTH];
int lineIndex = 0;

// 컬러 센서 설정
#define I2C_SDA 13
#define I2C_SCL 27
uint16_t currentR = 0, currentG = 0, currentB = 0, currentC = 0, currentLux = 0;
String currentColorName = "unknown";
int tuningSize = 7;
String tuning[7][5] = {
  {"white", "920", "2500", "1700", "1300"},
  {"red", "65200", "1700", "300", "250"},
  {"pink", "50", "2200", "800", "750"},
  {"orange", "65300", "2200", "500", "350"},
  {"green", "750", "700", "750", "330"},
  {"blue", "550", "600", "900", "900"},
  {"sky", "850", "1300", "1300", "1100"}
};
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

// 시간 저장
unsigned long start_time = 0;
unsigned long current_time = 0;

// 색 판단 함수
String color_define(uint16_t lux, uint16_t r, uint16_t g, uint16_t b, String tuning[][5], int tuningSize) {
  int raw[4] = {(int)lux, (int)r, (int)g, (int)b};
  long deviation[tuningSize];
  for (int i = 0; i < tuningSize; i++) {
    long temp = 0;
    for (int j = 0; j < 4; j++) {
      int tuningVal = tuning[i][j + 1].toInt();
      temp += pow((tuningVal - raw[j]), 2);
    }
    deviation[i] = temp;
  }
  int minIndex = 0;
  long minValue = deviation[0];
  for (int i = 1; i < tuningSize; i++) {
    if (deviation[i] < minValue) {
      minValue = deviation[i];
      minIndex = i;
    }
  }
  return tuning[minIndex][0];
}

// 데이터 전송 함수
void data(
    unsigned long start_time,
    int motorAState,
    int motorBState,
    float start_yaw
) {
    current_time = millis();

    // 최신 AHRS 한 줄 수신
    String latestLine = "";
    unsigned long timeout = millis() + 100;
    while (millis() < timeout) {
        if (AHRS_Serial.available()) {
            char c = AHRS_Serial.read();
            if (c == '\n') break;
            else latestLine += c;
        }
    }

    // Yaw 추출
    float parsedYaw = Yaw;
    int firstComma = latestLine.indexOf(',');
    int secondComma = latestLine.indexOf(',', firstComma + 1);
    if (firstComma != -1 && secondComma != -1) {
        String yawStr = latestLine.substring(secondComma + 1);
        parsedYaw = yawStr.toFloat();
    }
    Yaw = parsedYaw;
    float yaw_diff = start_yaw - Yaw;

    // 컬러 측정
    tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
    currentLux = tcs.calculateLux(currentR, currentG, currentB);
    currentColorName = color_define(currentLux, currentR, currentG, currentB, tuning, tuningSize);

    // 전송
    String msg = "[record]0|" + String(start_time) + "|" + String(current_time) + "|" +
                 String(motorAState) + "|" + String(motorBState) + "|" + String(yaw_diff, 2) + "|" +
                 currentColorName + "|" + String(currentLux) + "|" +
                 String(currentR) + "|" + String(currentG) + "|" + String(currentB) + "|" +
                 String(Yaw, 2);

    char msgBuffer[128];
    msg.toCharArray(msgBuffer, sizeof(msgBuffer));

    udp.beginPacket(PC1_IP, sendPort);
    udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
    udp.endPacket();

    Serial.printf("Sending data: %s\n", msgBuffer);
}

void setup() {
    Serial.begin(9600);
    AHRS_Serial.begin(115200, SERIAL_8N1, 34, -1);
    WiFi.begin(ssid, password);

    Serial.println("Connecting to WiFi...start");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi!");
    Serial.println("<esp32_ip>");
    Serial.println(WiFi.localIP());
    udp.begin(recvPort);

    ledcAttach(MOTOR_A_IN1, freq, resolution);
    ledcAttach(MOTOR_A_IN2, freq, resolution);
    ledcAttach(MOTOR_B_IN1, freq, resolution);
    ledcAttach(MOTOR_B_IN2, freq, resolution);

    ledcWrite(MOTOR_A_IN1, 0);
    ledcWrite(MOTOR_A_IN2, 0);
    ledcWrite(MOTOR_B_IN1, 0);
    ledcWrite(MOTOR_B_IN2, 0);

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!tcs.begin()) {
        Serial.println("No TCS34725 found ... check your connections");
        while (1);
    }
}

void loop() {
    char packetBuffer[255];
    int packetSize = udp.parsePacket();
    //data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
    if (packetSize) {
        udp.read(packetBuffer, 255);
        packetBuffer[packetSize] = '\0';
        Serial.print("Received: ");
        Serial.println(packetBuffer);

        if (aa == 0 && strcmp(packetBuffer, "success") == 0) {
            aa = 1;
            start_yaw = Yaw;
            start_time = millis();
            Serial.println("connecting success:");
        } else if (aa == 0) {
            data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)packetBuffer, strlen(packetBuffer));
            udp.endPacket();
        }

        if (aa == 1) {
            if (strcmp(packetBuffer, "w") == 0) {
                ledcWrite(MOTOR_A_IN1, MOTOR_A_state);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, MOTOR_B_state);
                ledcWrite(MOTOR_B_IN2, 0);
                data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "a") == 0) {
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, MOTOR_A_state);
                ledcWrite(MOTOR_B_IN1, MOTOR_B_state);
                ledcWrite(MOTOR_B_IN2, 0);
                data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "d") == 0) {
                ledcWrite(MOTOR_A_IN1, MOTOR_A_state);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, MOTOR_B_state);
                data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "s") == 0) {
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, MOTOR_A_state);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, MOTOR_B_state);
                data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "i") == 0) {
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, 0);
                data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "ahrs") == 0) {
                String msg = "[ahrs]" + String(Yaw, 2);
                char msgBuffer[64];
                msg.toCharArray(msgBuffer, sizeof(msgBuffer));
                udp.beginPacket(PC1_IP, sendPort);
                udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
                udp.endPacket();
                Serial.printf("Sending data: %s\n", msgBuffer);
            } else if (strcmp(packetBuffer, "Color") == 0) {
                tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
                currentLux = tcs.calculateLux(currentR, currentG, currentB);
                String msg = "[Color]" + String(currentLux) + "," + String(currentR) + "," + String(currentG) + "," + String(currentB);
                char msgBuffer[64];
                msg.toCharArray(msgBuffer, sizeof(msgBuffer));
                udp.beginPacket(PC1_IP, sendPort);
                udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
                udp.endPacket();
                Serial.printf("Sending data: %s\n", msgBuffer);
            }
        }
    }
}
