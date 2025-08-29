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
IPAddress pc1Ip(192, 168, 0, 7);

int aa = 0;
int bb = 0;
int cc = 0;

// 모터 설정
#define motorAIn1 25
#define motorAIn2 26
#define motorBIn1 32
#define motorBIn2 33
const int freq = 1000;
const int resolution = 8;
int motorAState = 250;
int motorBState = 250;
int motorA =250;
int motorB =250;
// AHRS 설정
#define maxLineLength 64
HardwareSerial ahrsSerial(2);
float roll = 0, pitch = 0, yaw = 0.1, startYaw = 0;
char line[maxLineLength];
int lineIndex = 0;

// 컬러 센서 설정
// #define maxColors 6  // 색상 수 고정
// #define attrCount 5  // color, lux, r, g, b
// String tuning[maxColors][attrCount];
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
#define i2cSda 13
#define i2cScl 27
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
unsigned long startTime = 0;
unsigned long currentTime = 0;

// 색 판단 함수
String colorDefine(uint16_t lux, uint16_t r, uint16_t g, uint16_t b,  int tuningSize) {
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
    unsigned long startTime,
    int motorAState,
    int motorBState
    ) {
    currentTime = millis();

    // 최신 AHRS 한 줄 수신
    String latestLine = "";
    unsigned long timeout = millis() + 100;
    while (millis() < timeout) {
        if (ahrsSerial.available()) {
            char c = ahrsSerial.read();
            if (c == '\n') break;
            else latestLine += c;
        }
    }

    // yaw 추출
    float parsedYaw = yaw;
    int firstComma = latestLine.indexOf(',');
    int secondComma = latestLine.indexOf(',', firstComma + 1);
    if (firstComma != -1 && secondComma != -1) {
        String yawStr = latestLine.substring(secondComma + 1);
        parsedYaw = yawStr.toFloat();
    }
    yaw = parsedYaw;
    float yawDiff = startYaw - yaw;

    // 컬러 측정
    tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
    currentLux = tcs.calculateLux(currentR, currentG, currentB);
    currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);

    // 전송
    String msg = "[record]0|" + String(startTime) + "|" + String(currentTime) + "|" +
                 String(motorAState) + "|" + String(motorBState) + "|" + String(yawDiff, 2) + "|" +
                 currentColorName + "|" + String(currentLux) + "|" +
                 String(currentR) + "|" + String(currentG) + "|" + String(currentB) + "|" +
                 String(yaw, 2);

    char msgBuffer[128];
    msg.toCharArray(msgBuffer, sizeof(msgBuffer));

    udp.beginPacket(pc1Ip, sendPort);
    udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
    udp.endPacket();

    Serial.printf("Sending data: %s\n", msgBuffer);
}

void sendRawColor(String name){
  String Tuning[6][5];
  int luxAvg = 0, rAvg = 0, gAvg = 0, bAvg = 0;
  for(int i=0;i<5;i++){
    tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
    currentLux = tcs.calculateLux(currentR, currentG, currentB);
    Tuning[i][0] = name;
    Tuning[i][1] = String(currentLux);
    Tuning[i][2] = String(currentR);
    Tuning[i][3] = String(currentG);
    Tuning[i][4] = String(currentB);

    luxAvg += currentLux;
    rAvg += currentR;
    gAvg += currentG;
    bAvg += currentB;
  }
  luxAvg = luxAvg/5;
  rAvg = rAvg/5;
  gAvg = gAvg/5;
  bAvg = bAvg/5;

  Tuning[5][0] = name;
  Tuning[5][1] = String(luxAvg);
  Tuning[5][2] = String(rAvg);
  Tuning[5][3] = String(gAvg);
  Tuning[5][4] = String(bAvg);

  String msg = "[raw_color]";
  for (int i = 0 ; i < 6 ; i++){
    for(int j = 0; j<5 ; j++){
    msg += "|" + Tuning[i][j];
    }
  }
    char msgBuffer[512];
    msg.toCharArray(msgBuffer, sizeof(msgBuffer));

    udp.beginPacket(pc1Ip, sendPort);
    udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
    udp.endPacket();
    Serial.printf("Sending data: %s\n", msgBuffer);
    // return luxAvg, rAvg, gAvg, bAvg;
}

void colorAdjust() {
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
            udp.beginPacket(pc1Ip, sendPort);
            udp.write((const uint8_t*)packetBuffer, strlen(packetBuffer));
            udp.endPacket();
        }
        else if (bb = 1){
          // 변환!!
          Serial.println("translate");
          Serial.println(packetBuffer);
          String packetStr = String(packetBuffer);  // char[] → String으로 변환

          if (packetStr.startsWith("color=")) {
            Serial.print("color!!=");
            String name = packetStr.substring(String("color=").length());
            Serial.print("name: ");
            Serial.println(name);
            sendRawColor(name);
          } 
          else if (packetStr.startsWith("colorData")) {
            Serial.print("[colorData!!=]");
            String msg = packetStr.substring(String("[colorData]").length());
            Serial.print("operator2esp32: ");
            Serial.println(msg);
            // 여기서 msg 파싱 처리s

            // 1. '|' 기준으로 파싱
            const int maxParts = 100;
            String parts[maxParts];
            int count = 0;

            int start = 0;
            int idx = msg.indexOf('|');
            while (idx != -1 && count < maxParts) {
              parts[count++] = msg.substring(start, idx);
              start = idx + 1;
              idx = msg.indexOf('|', start);
            }
            parts[count++] = msg.substring(start);  // 마지막 항목

            // 2. 8개 색상, 각 색상당 5개 필드(name + clear + rgb)
            int colorIndex = 0;
            for (int i = 0 ; i + 4 < count && colorIndex < 8 ; i += 5) {
              tuning[colorIndex][0] = parts[i];     // name
              tuning[colorIndex][1] = parts[i + 1]; // clear
              tuning[colorIndex][2] = parts[i + 2]; // red
              tuning[colorIndex][3] = parts[i + 3]; // green
              tuning[colorIndex][4] = parts[i + 4]; // blue
              colorIndex++;
            }

            // 3. 확인용 출력
            Serial.println("tuning 배열에 저장된 색상:");
            for (int i = 0 ; i < 8 ; i++) {
              Serial.print("tuning[" + String(i) + "] = { ");
              for (int j = 0 ; j < 5 ; j++) {
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
  

void colorName(){
  tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
  currentLux = tcs.calculateLux(currentR, currentG, currentB);
  currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);
  if (currentColorName.length() == 0) {
      Serial.println(currentColorName);
      String msg = "[colorName]" + currentColorName;
      Serial.println(msg);
      
      char msgBuffer[64];
      msg.toCharArray(msgBuffer, sizeof(msgBuffer));
      udp.beginPacket(pc1Ip, sendPort);
      udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
      udp.endPacket();
      Serial.printf("Sending data: %s\n", msgBuffer);        }
}

void motorDeviation(){

}

void setup() {


    Serial.begin(9600);
    ahrsSerial.begin(115200, SERIAL_8N1, 34, -1);
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
        motorA = 250;
        motorB = 250;
    }

    else if(WiFi.localIP()=="192.168.0.18"){
        sendPort = 7000;
        recvPort = 7001;
        motorA = 250;
        motorB = 250;

    }
    Serial.print("recvPort: ");
    Serial.println(recvPort);
    Serial.print("sendport: ");
    Serial.println(sendPort);


    udp.begin(recvPort);

    ledcAttach(motorAIn1, freq, resolution);
    ledcAttach(motorAIn2, freq, resolution);
    ledcAttach(motorBIn1, freq, resolution);
    ledcAttach(motorBIn2, freq, resolution);

    ledcWrite(motorAIn1, 0);
    ledcWrite(motorAIn2, 0);
    ledcWrite(motorBIn1, 0);
    ledcWrite(motorBIn2, 0);

    Wire.begin(i2cSda, i2cScl);
    if (!tcs.begin()) {
        Serial.println("No TCS34725 found ... check your connections");
        while (1);
    }
}

void loop() {
  
    char packetBuffer[255];
    int packetSize = udp.parsePacket();
    //data(startTime, motorAState, motorBState, startYaw);
    if (packetSize) {
        // colorName();

        udp.read(packetBuffer, 255);
        packetBuffer[packetSize] = '\0';
        Serial.print("Received: ");
        Serial.println(packetBuffer);

        if (aa == 0 && strcmp(packetBuffer, "success") == 0) {
            aa = 1;
            startYaw = yaw;
            startTime = millis();
            Serial.println("connecting success:");
        } else if (aa == 0) {
//            data(startTime, motorAState, motorBState);
            udp.beginPacket(pc1Ip, sendPort);
            udp.write((const uint8_t*)packetBuffer, strlen(packetBuffer));
            udp.endPacket();
        }

        if (aa == 1) {
            motorAState = motorA;
            motorBState = motorB;
            if (strcmp(packetBuffer, "w") == 0) {
                ledcWrite(motorAIn1, motorAState);
                ledcWrite(motorAIn2, 0);
                ledcWrite(motorBIn1, motorBState);
                ledcWrite(motorBIn2, 0);
                // data(startTime, motorAState, motorBState, startYaw);
            } else if (strcmp(packetBuffer, "a") == 0) {
                motorBState = 200  ;
                ledcWrite(motorAIn1, 0);
                ledcWrite(motorAIn2, 0);
                ledcWrite(motorBIn1, motorBState);
                ledcWrite(motorBIn2, 0);
                // data(startTime, motorAState, motorBState, startYaw);
            } else if (strcmp(packetBuffer, "d") == 0) {
                motorAState = 200;
                ledcWrite(motorAIn1, motorAState);
                ledcWrite(motorAIn2, 0);
                ledcWrite(motorBIn1, 0);
                ledcWrite(motorBIn2, 0);
                // data(startTime, motorAState, motorBState, startYaw);
            } else if (strcmp(packetBuffer, "s") == 0) {
                ledcWrite(motorAIn1, 0);
                ledcWrite(motorAIn2, motorAState);
                ledcWrite(motorBIn1, 0);
                ledcWrite(motorBIn2, motorBState);
                // data(startTime, motorAState, motorBState, startYaw);
            } else if (strcmp(packetBuffer, "i") == 0) {
                ledcWrite(motorAIn1, 0);
                ledcWrite(motorAIn2, 0);
                ledcWrite(motorBIn1, 0);
                ledcWrite(motorBIn2, 0);
                // data(startTime, motorAState, motorBState, startYaw);
                
            } else if (strcmp(packetBuffer, "[colorAdjust]") == 0){
              colorAdjust();
            } else if (strcmp(packetBuffer, "ahrs") == 0) {
                String msg = "[ahrs]" + String(yaw, 2);
                char msgBuffer[64];
                msg.toCharArray(msgBuffer, sizeof(msgBuffer));
                udp.beginPacket(pc1Ip, sendPort);
                udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
                udp.endPacket();
                Serial.printf("Sending data: %s\n", msgBuffer);

            } else if (strcmp(packetBuffer, "color") == 0) {
                tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
                currentLux = tcs.calculateLux(currentR, currentG, currentB);
                String msg = "[color]" + String(currentLux) + "," + String(currentR) + "," + String(currentG) + "," + String(currentB);
                char msgBuffer[64];
                msg.toCharArray(msgBuffer, sizeof(msgBuffer));
                udp.beginPacket(pc1Ip, sendPort);
                udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
                udp.endPacket();
                Serial.printf("Sending data: %s\n", msgBuffer);
            }
            else if(strcmp(packetBuffer, "stop") == 0) {
                ledcWrite(motorAIn1, 0);
                ledcWrite(motorAIn2, 0);
                ledcWrite(motorBIn1, 0);
                ledcWrite(motorBIn2, 0);  
                Serial.println("stop!!!!!!!!!!!!!!!!!!!");
                delay(5000);   
            }
            else if(strcmp(packetBuffer, "[die]") == 0) {
                ledcWrite(motorAIn1, 0);
                ledcWrite(motorAIn2, 0);
                ledcWrite(motorBIn1, 0);
                ledcWrite(motorBIn2, 0);     
                delay(10000000);
            }
            else if(strcmp(packetBuffer, "[name]") == 0) {
              tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
              currentLux = tcs.calculateLux(currentR, currentG, currentB);
              currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);
              Serial.println(currentColorName);
              char msgBuffer[64];
              currentColorName.toCharArray(msgBuffer, sizeof(msgBuffer));
              udp.beginPacket(pc1Ip, sendPort);
              udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
              udp.endPacket();
              Serial.printf("[name] Sending data : %s\n", msgBuffer);    
            }

            if(cc>0 && cc<30){
                  String msg = "[save]";
                  char msgBuffer[512];
                  msg.toCharArray(msgBuffer, sizeof(msgBuffer));

                  udp.beginPacket(pc1Ip, sendPort);
                  udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
                  udp.endPacket();
                  Serial.printf("Sending data: %s\n", msgBuffer);
            }
    
 

        }
    }
    // if(aa==1){
    //   colorName();
    // }
}