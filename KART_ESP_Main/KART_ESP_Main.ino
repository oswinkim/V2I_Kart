#include <WiFi.h>
#include <WiFiUdp.h>
#include "HardwareSerial.h"     //ahrs
#include "Adafruit_TCS34725.h" 

HardwareSerial AHRS_Serial(2);  // UART2 객체 생성  ahrs

float Roll = 0;     //ahrs
float Pitch = 0;     //ahrs
float Yaw = 0;     //ahrs


const char* ssid = "a12";      // WiFi 이름
const char* password = "12345678";  // WiFi 비밀번호

#define MAX_LINE_LENGTH 64
char line[MAX_LINE_LENGTH];
int lineIndex = 0;

WiFiUDP udp;
const unsigned int recvPort = 4212;  // PC1에서 제어 명령을 받을 포트
const unsigned int sendPort = 4213;  // PC1으로 데이터를 보낼 포트

IPAddress PC1_IP(192, 168, 0, 10);  // PC1의 IP 주소

#define MOTOR_A_IN1 25  // PWM핀
#define MOTOR_A_IN2 26
#define MOTOR_B_IN1 32
#define MOTOR_B_IN2 33
#define I2C_SDA 13
#define I2C_SCL 27

unsigned long currenttime = 0;
int randVal = 0;


Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

int aa = 0;
int lux, r, g, b = 0;


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

    pinMode(MOTOR_A_IN1, OUTPUT);
    pinMode(MOTOR_A_IN2, OUTPUT);
    pinMode(MOTOR_B_IN1, OUTPUT);
    pinMode(MOTOR_B_IN2, OUTPUT);

    digitalWrite(MOTOR_A_IN1, LOW);
    digitalWrite(MOTOR_A_IN2, LOW);
    digitalWrite(MOTOR_B_IN1, LOW);
    digitalWrite(MOTOR_B_IN2, LOW);

    randomSeed(millis() % 255);
    currenttime = millis();
    randVal = random(0, 255);



    Wire.begin(I2C_SDA, I2C_SCL);

    if (tcs.begin()) {
        Serial.println("Found sensor");
    } else {
        Serial.println("No TCS34725 found ... check your connections");
        while (1);
    }
}

void loop() {
    char packetBuffer[255];
    int packetSize = udp.parsePacket();

    // AHRS Yaw 값만 추출
    while (AHRS_Serial.available()) {
        char c = AHRS_Serial.read();
        if (c == '\n') {
            line[lineIndex] = '\0';

            // Yaw 파싱
            char* p1 = strchr(line, ',');
            char* p2 = p1 ? strchr(p1 + 1, ',') : nullptr;
            char* p3 = p2 ? strchr(p2 + 1, ',') : nullptr;

            if (p1 && p2 && p3) {
                Yaw = atof(p3 + 1);
            }

            lineIndex = 0;
        } else if (lineIndex < MAX_LINE_LENGTH - 1) {
            line[lineIndex++] = c;
        } else {
            // 버퍼 오버플로 방지
            lineIndex = 0;
        }
    }

    // UDP 패킷 처리
    if (packetSize) {
        udp.read(packetBuffer, 255);
        packetBuffer[packetSize] = '\0';
        Serial.print("Received: ");
        Serial.println(packetBuffer);

        if (aa == 0) {
            String msg = packetBuffer;

            if (strcmp(packetBuffer, "success") == 0) {
                aa = 1;
                Serial.println("connecting success:");
            } else {
                char msgBuffer[64];
                msg.toCharArray(msgBuffer, sizeof(msgBuffer));

                udp.beginPacket(PC1_IP, sendPort);
                udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
                udp.endPacket();

                Serial.printf("Sending data: %s\n", msgBuffer);
            }
        }

        // 모터 제어
        else if (strcmp(packetBuffer, "w") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);

            digitalWrite(MOTOR_A_IN1, HIGH);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, HIGH);
            digitalWrite(MOTOR_B_IN2, LOW);
            Serial.println("FORWARD");
        } else if (strcmp(packetBuffer, "a") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);

            digitalWrite(MOTOR_A_IN1, HIGH);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, HIGH);
            Serial.println("LEFT");
        } else if (strcmp(packetBuffer, "d") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);

            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, HIGH);
            digitalWrite(MOTOR_B_IN1, HIGH);
            digitalWrite(MOTOR_B_IN2, LOW);
            Serial.println("RIGHT");
        } else if (strcmp(packetBuffer, "s") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);

            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, HIGH);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, HIGH);
            Serial.println("BACKWARD");
        } else if (strcmp(packetBuffer, "i") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);
            Serial.println("MOTOR OFF");

             uint16_t r, g, b, c;
            tcs.getRawData(&r, &g, &b, &c);
            lux = tcs.calculateLux(r, g, b);


            //원상형태["구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "AHRS", "LUX","컬러R", "컬러G", "컬러B"]
            String msg = String("[") + "none color,0s,0s,0,0," +
             String(Yaw, 2) + "," +
             String(lux, 2) + "," +
             String(r) + "," +
             String(g) + "," +
             String(b) + "]";

            char msgBuffer[64];
            msg.toCharArray(msgBuffer, sizeof(msgBuffer));

            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
            udp.endPacket();

            Serial.printf("Sending data: %s\n", msgBuffer);
        }

        // Yaw 값 요청 처리
        else if (strcmp(packetBuffer, "ahrs") == 0) {
            Serial.println("Request received");


            String msg = "[ahrs]" + String(Yaw, 2);
            char msgBuffer[64];
            msg.toCharArray(msgBuffer, sizeof(msgBuffer));

            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
            udp.endPacket();

            Serial.printf("Sending data: %s\n", msgBuffer);
        }

        else if (strcmp(packetBuffer, "Color") == 0) {
            uint16_t r, g, b, c;
            tcs.getRawData(&r, &g, &b, &c);
            lux = tcs.calculateLux(r, g, b);

            String msg = "[Color]" + String(lux) + "," + String(r) + "," + String(g) + ","+ String(b);
            char msgBuffer[64];
            msg.toCharArray(msgBuffer, sizeof(msgBuffer));

            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
            udp.endPacket();

            Serial.printf("Sending data: %s\n", msgBuffer);
            
        }

    }
}

/*
PC1(서버컴): 망우 17번
PC2(이용자): 망우 16번
공유기:사마의 �
*/