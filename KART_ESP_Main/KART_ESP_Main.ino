#include <WiFi.h>
#include <WiFiUdp.h>
#include "HardwareSerial.h"     
#include "Adafruit_TCS34725.h" 

//udp설정 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WiFiUDP udp;

const char* ssid = "a12";      // WiFi 이름
const char* password = "12345678";  // WiFi 비밀번호
const unsigned int recvPort = 4212;  // PC1에서 제어 명령을 받을 포트
const unsigned int sendPort = 4213;  // PC1으로 데이터를 보낼 포트

IPAddress PC1_IP(192, 168, 0, 10);  // PC1의 IP 주소
int aa = 0;     //연산컴퓨터와 최초연결시 1로 갱신

//모터설정 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MOTOR_A_IN1 25  // PWM핀
#define MOTOR_A_IN2 26
#define MOTOR_B_IN1 32
#define MOTOR_B_IN2 33

int MOTOR_A_state = 0;
int MOTOR_B_state = 0;

//ahrs설정 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_LINE_LENGTH 64

float Roll = 0;     
float Pitch = 0;    
float start_yaw = 0;
float Yaw = 0;     
char line[MAX_LINE_LENGTH];
int lineIndex = 0;

HardwareSerial AHRS_Serial(2);  // UART2 객체 생성  ahrs

//컬러센서 설정 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define I2C_SDA 13
#define I2C_SCL 27

uint16_t currentR = 0, currentG = 0, currentB = 0, currentC = 0, currentLux = 0;
int tuningSize = 7;     //tuning배열의 색깔 개수임
String currentColorName = "unknown";

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

//데이터 저장 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long start_time = 0;
unsigned long current_time = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//색깔 변환 함수
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

//recoding데이터 전송 함수
void data(
    unsigned long start_time,
    int motorAState,
    int motorBState,
    float start_yaw,
    float current_yaw,
    String colorName,
    uint16_t lux,
    uint16_t r,
    uint16_t g,
    uint16_t b
    ) {
        current_time = millis();
        float yaw_diff = start_yaw - current_yaw;

        String msg = "0," +                         // 현재 구간
                    String(start_time) + "|" +     // 최초 연결 시간
                    String(current_time) + "|" +   // 현재 시간
                    String(motorAState) + "|" +    // 왼쪽 모터 상태
                    String(motorBState) + "|" +    // 오른쪽 모터 상태
                    String(yaw_diff, 2) + "|" +    // 방향변환값
                    colorName + "|" +              // 변환된 컬러값
                    String(lux) + "|" +            // LUX
                    String(r) + "|" +              // 컬러 R
                    String(g) + "|" +              // 컬러 G
                    String(b) + "|" +              // 컬러 B
                    String(current_yaw, 2);       // raw방향값

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

    pinMode(MOTOR_A_IN1, OUTPUT);
    pinMode(MOTOR_A_IN2, OUTPUT);
    pinMode(MOTOR_B_IN1, OUTPUT);
    pinMode(MOTOR_B_IN2, OUTPUT);

    digitalWrite(MOTOR_A_IN1, LOW);
    digitalWrite(MOTOR_A_IN2, LOW);
    digitalWrite(MOTOR_B_IN1, LOW);
    digitalWrite(MOTOR_B_IN2, LOW);

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

    // 컬러센서 값 갱신
    tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
    currentLux = tcs.calculateLux(currentR, currentG, currentB);
    //색 판단
    currentColorName = color_define(currentLux, currentR, currentG, currentB, tuning, tuningSize);

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
                start_yaw = Yaw;        // 최초 방향 저장
                start_time = millis();    //시작 시간 설정
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
            String msg = "[Color]" + String(currentLux) + "," + String(currentR) + "," + String(currentG) + "," + String(currentB);

            char msgBuffer[64];
            msg.toCharArray(msgBuffer, sizeof(msgBuffer));

            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
            udp.endPacket();

            Serial.printf("Sending data: %s\n", msgBuffer);
            
        }
        data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw, Yaw, currentColorName, currentLux, currentR, currentG, currentB);

    }
}

/*
PC1(서버컴): 망우 17번
PC2(이용자): 망우 16번
공유기:사마의 �
*/