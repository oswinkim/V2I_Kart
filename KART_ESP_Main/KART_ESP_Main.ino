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

WiFiUDP udp;
const unsigned int recvPort = 4212;  // PC1에서 제어 명령을 받을 포트
const unsigned int sendPort = 4213;  // PC1으로 데이터를 보낼 포트

IPAddress PC1_IP(192, 168, 0, 8);  // PC1의 IP 주소

#define MOTOR_A_IN1 25  // PWM핀
#define MOTOR_A_IN2 26
#define MOTOR_B_IN1 32
#define MOTOR_B_IN2 33
#define I2C_SDA 13
#define I2C_SCL 27

unsigned long currenttime = 0;
int randVal = 0;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

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
    if (millis() - currenttime > 5000){
        randVal = random(0, 255);
        currenttime = millis();
    }

    if (AHRS_Serial.available()) {
        String inString = AHRS_Serial.readStringUntil('\n'); 

        int index1 = inString.indexOf(','); 
        int index2 = inString.indexOf(',', index1 + 1);
        int index3 = inString.indexOf(',', index2 + 1);
        int index4 = inString.length();

        if (index1 > 0 && index2 > index1 && index3 > index2) {
        Roll = inString.substring(index1 + 1, index2).toFloat();
        Pitch = inString.substring(index2 + 1, index3).toFloat();
        Yaw = inString.substring(index3 + 1, index4).toFloat();
        


        }
    }
    if (packetSize) {
        udp.read(packetBuffer, 255);
        packetBuffer[packetSize] = '\0';  // 문자열 종료
        Serial.print("Received: ");
        Serial.println(packetBuffer);

        // 모터 제어
        if (strcmp(packetBuffer, "w") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);

            digitalWrite(MOTOR_A_IN1, HIGH);
            digitalWrite(MOTOR_B_IN1, HIGH);

            Serial.println("FORWARD");
        }
        else if (strcmp(packetBuffer, "a") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);
            
            digitalWrite(MOTOR_A_IN1, HIGH);
            digitalWrite(MOTOR_B_IN2, HIGH); 

            Serial.println("LEFT");
        }
        else if (strcmp(packetBuffer, "d") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);
            
            digitalWrite(MOTOR_A_IN2, HIGH);
            digitalWrite(MOTOR_B_IN1, HIGH);

            Serial.println("RIGHT");
        }
        else if (strcmp(packetBuffer, "s") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);

            digitalWrite(MOTOR_A_IN2, HIGH);
            digitalWrite(MOTOR_B_IN2, HIGH);

            Serial.println("BACKWARD");
        } 
        else if (strcmp(packetBuffer, "i") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);
            digitalWrite(MOTOR_B_IN2, LOW);

            Serial.println("MORTOR OFF");
        }

        // 데이터 전송
        else if (strcmp(packetBuffer, "ahrs") == 0) {
            Serial.println("Request received");

            // [ahrs] 형식의 문자열 만들기
            // String msg = "[ahrs]" + String(Roll, 2) + "," + String(Pitch, 2) + "," + String(Yaw, 2);
            String msg = "[ahrs]" + String(Yaw, 2);

            // String을 C 문자열(char 배열)로 변환
            char msgBuffer[64];
            msg.toCharArray(msgBuffer, sizeof(msgBuffer));

            // UDP로 전송
            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
            udp.endPacket();

            // 시리얼 출력
            Serial.printf("Sending data: %s\n", msgBuffer);
        }

        else if (strcmp(packetBuffer, "Color") == 0) {
            Serial.println("Request received");
        
            uint16_t r, g, b, c;
            tcs.getRawData(&r, &g, &b, &c);      
        
            // [ahrs] 형식의 문자열 만들기
            String msg = "[Color]" + String(r) + "," + String(g) + "," + String(b);
            
           
            // String을 C 문자열(char 배열)로 변환
            char msgBuffer[64];
            msg.toCharArray(msgBuffer, sizeof(msgBuffer));

            // UDP로 전송
            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
            udp.endPacket();

            // 시리얼 출력
            Serial.printf("Sending data: %s\n", msgBuffer);
        }
    }
}

/*
PC1(서버컴): 망우 17번
PC2(이용자): 망우 16번
공유기:사마의 �
*/