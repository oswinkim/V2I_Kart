#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "HardwareSerial.h"
#include "Adafruit_TCS34725.h"

// WiFi 설정
WiFiUDP udp;
const char* ssid = "a12";
const char* password = "12345678";
unsigned int recvPort = 7001;   //wifi연결 후 자동 설정
unsigned int sendPort = 7000;
IPAddress PC1_IP(192, 168, 0, 7);

int aa = 0;
int bb = 0;
int cc = 0;

// 모터 설정
#define MOTOR_A_IN1 25
#define MOTOR_A_IN2 26
#define MOTOR_B_IN1 32
#define MOTOR_B_IN2 33
const int freq = 1000;
const int resolution = 8;
int MOTOR_A_state = 250;
int MOTOR_B_state = 250;
int MOTOR_A =250;
int MOTOR_B =250;
// AHRS 설정
// #define MAX_LINE_LENGTH 64
// HardwareSerial AHRS_Serial(2);
float Roll = 0, Pitch = 0, Yaw = 0.1, start_yaw = 0;
// char line[MAX_LINE_LENGTH];
// int lineIndex = 0;

// 컬러 센서 설정
// #define MAX_COLORS 6  // 색상 수 고정
// #define ATTR_COUNT 5  // color, lux, r, g, b
// String tuning[MAX_COLORS][ATTR_COUNT];
String tokens[200];          // 1차원 배열로 파싱된 결과
String tuning[8][5]={
{"mdf","44","247","114","76"},
{"red","65485","337","60","50"},
{"blue","153","177","248","246"},
{"green","196","184","203","88"},
{"pink","36","694","275","235"},
{"orange","65500","550","130","86"},
{"sky","240","386","373","305"},
{"white","218","615","409","311"}
};         // 최종 2차원 배열
int tokenCount = 0;
int tuningSize = 8;
#define I2C_SDA 13
#define I2C_SCL 27
uint16_t currentR = 0, currentG = 0, currentB = 0, currentC = 0, currentLux = 0;
String currentColorName = "unknown";

  // int tuningSize = 7;
  // String ttuning[7][5] = {
  //   {"white", "920", "2500", "1700", "1300"},
  //   {"red", "65200", "1700", "300", "250"},
  //   {"pink", "50", "2200", "800", "750"},
  //   {"orange", "65300", "2200", "500", "350"},
  //   {"green", "750", "700", "750", "330"},
  //   {"blue", "550", "600", "900", "900"},
  //   {"sky", "850", "1300", "1300", "1100"}
  // };
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_1X);

// 시간 저장
unsigned long start_time = 0;
unsigned long current_time = 0;

// 색 판단 함수
String color_define(uint16_t lux, uint16_t r, uint16_t g, uint16_t b,  int tuningSize) {
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
    int motorBState
    ) {
    current_time = millis();


    // Yaw 추출
    float yaw_diff = 1;

    // 컬러 측정
    tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
    currentLux = tcs.calculateLux(currentR, currentG, currentB);
    currentColorName = color_define(currentLux, currentR, currentG, currentB, tuningSize);

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

void send_raw_color(String name){
  String Tuning[6][5];
  int lux_avg = 0, r_avg = 0, g_avg = 0, b_avg = 0;
  for(int i=0;i<5;i++){
    tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
    currentLux = tcs.calculateLux(currentR, currentG, currentB);
    Tuning[i][0] = name;
    Tuning[i][1] = String(currentLux);
    Tuning[i][2] = String(currentR);
    Tuning[i][3] = String(currentG);
    Tuning[i][4] = String(currentB);

    lux_avg += currentLux;
    r_avg += currentR;
    g_avg += currentG;
    b_avg += currentB;
  }
  lux_avg = lux_avg/5;
  r_avg = r_avg/5;
  g_avg = g_avg/5;
  b_avg = b_avg/5;

  Tuning[5][0] = name;
  Tuning[5][1] = String(lux_avg);
  Tuning[5][2] = String(r_avg);
  Tuning[5][3] = String(g_avg);
  Tuning[5][4] = String(b_avg);

  String msg="[raw_color]";
  for (int i=0 ; i<6 ; i++){
    for(int j=0;j<5;j++){
    msg+="|" + Tuning[i][j];
    }
  }
    char msgBuffer[512];
    msg.toCharArray(msgBuffer, sizeof(msgBuffer));

    udp.beginPacket(PC1_IP, sendPort);
    udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
    udp.endPacket();
    Serial.printf("Sending data: %s\n", msgBuffer);
    // return lux_avg, r_avg, g_avg, b_avg;
}

void color_adjust() {
  while (1){
    char packetBuffer[255];
    int packetSize = udp.parsePacket();
    if (packetSize) {
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        udp.read(packetBuffer, 255);
        packetBuffer[packetSize] = '\0';
        Serial.printf("Received!!!!!!!!!!: ");
        Serial.println(packetBuffer);

        if (bb == 0 && strcmp(packetBuffer, "success") == 0) {
            bb = 1;
            Serial.println("ready to color name...");

        } else if (bb == 0) {
            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)packetBuffer, strlen(packetBuffer));
            udp.endPacket();
        }
        else if (bb=1){
          // 변환!!
          Serial.println("translate");
          Serial.println(packetBuffer);
          String packetStr = String(packetBuffer);  // char[] → String으로 변환

          if (packetStr.startsWith("color=")) {
            Serial.print("color!!=");
            String name = packetStr.substring(String("color=").length());
            Serial.print("name: ");
            Serial.println(name);
            send_raw_color(name);
          } 
          else if (packetStr.startsWith("color_data")) {
            Serial.print("[color_data!!=]");
            String msg = packetStr.substring(String("[color_data]").length());
            Serial.print("operator2esp32: ");
            Serial.println(msg);
            // 여기서 msg 파싱 처리s

            // 1. '|' 기준으로 파싱
            const int MAX_PARTS = 100;
            String parts[MAX_PARTS];
            int count = 0;

            int start = 0;
            int idx = msg.indexOf('|');
            while (idx != -1 && count < MAX_PARTS) {
              parts[count++] = msg.substring(start, idx);
              start = idx + 1;
              idx = msg.indexOf('|', start);
            }
            parts[count++] = msg.substring(start);  // 마지막 항목

            // 2. 8개 색상, 각 색상당 5개 필드(name + clear + rgb)
            int colorIndex = 0;
            for (int i = 0; i + 4 < count && colorIndex < 8; i += 5) {
              tuning[colorIndex][0] = parts[i];     // name
              tuning[colorIndex][1] = parts[i + 1]; // clear
              tuning[colorIndex][2] = parts[i + 2]; // red
              tuning[colorIndex][3] = parts[i + 3]; // green
              tuning[colorIndex][4] = parts[i + 4]; // blue
              colorIndex++;
            }

            // 3. 확인용 출력
            Serial.println("tuning 배열에 저장된 색상:");
            for (int i = 0; i < 8; i++) {
              Serial.print("tuning[" + String(i) + "] = { ");
              for (int j = 0; j < 5; j++) {
                Serial.print("\"" + tuning[i][j] + "\"");
                if (j < 4) Serial.print(", ");
              }
              Serial.println(" }");
            }

            cc++;
            break;
          }
            
          }
          }

        }
    }
  

void color_name(){
  tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
  currentLux = tcs.calculateLux(currentR, currentG, currentB);
  currentColorName = color_define(currentLux, currentR, currentG, currentB, tuningSize);
  if (currentColorName.length() == 0) {
      Serial.println(currentColorName);
      String msg = "[color_name]" + currentColorName;
      Serial.println(msg);
      
      char msgBuffer[64];
      msg.toCharArray(msgBuffer, sizeof(msgBuffer));
      udp.beginPacket(PC1_IP, sendPort);
      udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
      udp.endPacket();
      Serial.printf("Sending data: %s\n", msgBuffer);        }
}

void setup() {


    Serial.begin(9600);
    // AHRS_Serial.begin(115200, SERIAL_8N1, 34, -1);
    WiFi.begin(ssid, password);

    Serial.println("Connecting to WiFi...start");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi!");
    Serial.println("<esp32_ip>");
    Serial.println(WiFi.localIP());

    if(WiFi.localIP()=="192.168.0.14"){
        sendPort = 4213;
        recvPort = 4212;
        MOTOR_A = 250;
        MOTOR_B = 250;
    }

    else if(WiFi.localIP()=="192.168.0.18"){
        sendPort = 7000;
        recvPort = 7001;
        MOTOR_A = 250;
        MOTOR_B = 250;

    }
    Serial.print("recvport: ");
    Serial.println(recvPort);
    Serial.print("sendport: ");
    Serial.println(sendPort);


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
        // color_name();

        udp.read(packetBuffer, 255);
        packetBuffer[packetSize] = '\0';
        Serial.print("Received: ");
        Serial.println(packetBuffer);

//         if (aa == 0 && strcmp(packetBuffer, "success") == 0) {
//             aa = 1;
//             start_yaw = Yaw;
//             start_time = millis();
//             Serial.println("connecting success:");
//         } else if (aa == 0) {
// //            data(start_time, MOTOR_A_state, MOTOR_B_state);
//             udp.beginPacket(PC1_IP, sendPort);
//             udp.write((const uint8_t*)packetBuffer, strlen(packetBuffer));
//             udp.endPacket();
//         }

        if (aa == 0) {
            MOTOR_A_state = MOTOR_A;
            MOTOR_B_state = MOTOR_B;
            if (strcmp(packetBuffer, "w") == 0) {
                ledcWrite(MOTOR_A_IN1, MOTOR_A_state);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, MOTOR_B_state);
                ledcWrite(MOTOR_B_IN2, 0);
                // data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "a") == 0) {
                MOTOR_B_state = 200  ;
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, MOTOR_B_state);
                ledcWrite(MOTOR_B_IN2, 0);
                // data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "d") == 0) {
                MOTOR_A_state = 200;
                ledcWrite(MOTOR_A_IN1, MOTOR_A_state);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, 0);
                // data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "s") == 0) {
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, MOTOR_A_state);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, MOTOR_B_state);
                // data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "i") == 0) {
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, 0);
                // data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
                
            } else if (strcmp(packetBuffer, "[color_adjust]") == 0){
              color_adjust();
            } 

            else if (strcmp(packetBuffer, "ahrs") == 0) {
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
            else if(strcmp(packetBuffer, "stop") == 0) {
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, 0);  
                Serial.println("stop!!!!!!!!!!!!!!!!!!!");
                delay(5000);   
            }
            else if(strcmp(packetBuffer, "[die]") == 0) {
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, 0);
                ledcWrite(MOTOR_B_IN2, 0);     
                delay(10000000);
            }
            else if(strcmp(packetBuffer, "[name]") == 0) {
                          tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
            currentLux = tcs.calculateLux(currentR, currentG, currentB);
            currentColorName = color_define(currentLux, currentR, currentG, currentB, tuningSize);
            Serial.println(currentColorName);
            char msgBuffer[64];
            currentColorName.toCharArray(msgBuffer, sizeof(msgBuffer));
            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
            udp.endPacket();
            Serial.printf("[name] Sending data : %s\n", msgBuffer);    
            }

            if(cc>0 && cc<30){
                  String msg = "[save]";
                  char msgBuffer[512];
                  msg.toCharArray(msgBuffer, sizeof(msgBuffer));

                  udp.beginPacket(PC1_IP, sendPort);
                  udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
                  udp.endPacket();
                  Serial.printf("Sending data: %s\n", msgBuffer);
            }
    
 

        }
    }
    // if(aa==1){
    //   color_name();
    // }
}