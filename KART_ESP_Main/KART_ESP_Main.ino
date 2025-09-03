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
IPAddress pc1Ip(192, 168, 3, 228);

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
float roll = 0, pitch = 0, yaw = 0, startYaw = 0, yawDiff = 0;
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

//-------------------------------------함수---------------------------------------------------------//

// 메시지 전송 함수
void sendMsg(String msg, int condition = 1, IPAddress ip = pc1Ip, unsigned int port = sendPort){
  udp.beginPacket(ip, port);
  udp.print(msg);
  udp.endPacket();
  
  if (condition ==1) Serial.printf("Sending data: %s\n", msg.c_str());
}

// 색상 판단 함수
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

// yaw값 업데이트 함수
void yawAhrs(){
  // 최신 AHRS 한 줄 수신
  String latestLine = "";
  currentTime = millis();
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
  if (latestLine.startsWith("$EUL")) {
      int firstComma  = latestLine.indexOf(',');
      int secondComma = latestLine.indexOf(',', firstComma + 1);
      int thirdComma  = latestLine.indexOf(',', secondComma + 1);

      if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
          String yawStr = latestLine.substring(thirdComma + 1);
          parsedYaw = yawStr.toFloat();
      }
  }

  yaw = parsedYaw + 180;
  yawDiff = startYaw - yaw;
}

// 데이터 전송 함수
void data(
    unsigned long startTime,
    int motorAState,
    int motorBState
    ) {
      //yaw값 측정
      yawAhrs();

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

      sendMsg(msg);
}

// 컬러센서 측정값 전송 함수
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
  sendMsg(msg);
}

// 색상 보정 함수
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
          sendMsg(packetBuffer);
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

// 색상 이름 전송 함수
void colorName(){
  tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
  currentLux = tcs.calculateLux(currentR, currentG, currentB);
  currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);
  if (currentColorName.length() == 0) {
      Serial.println(currentColorName);
      String msg = "[colorName]" + currentColorName;

      sendMsg(msg);
      }
}

// 모터 편차 조정 함수
String motorDeviation(float error){
  int leftMotorLeast = 100, rightMotorLeast = 100;
  int leftMotorStraight = 0, rightMotorStraight = 0;
  char weakMotor = 'A';
  unsigned long timeout;
  unsigned long delayLeastMotor = 3000, delayWeakMotor = 5000, delayStraightMotor = 3000, delayDistance = 2000 ,delayStop = 500;

  yawAhrs();

  // 모터 최소 작동값 찾기
  // 오른쪽 모터
  Serial.println("[Finding least value]");
  Serial.println("right motor finding!");
  while (1){
    Serial.print("yaw:");
    Serial.println(yaw);
    float beforeYaw = yaw;

    driving(0, rightMotorLeast);

    timeout = millis() + delayLeastMotor;
    while (millis() < timeout) {
      yawAhrs();
    }

    driving(0, 0);

    timeout = millis() + delayStop;
    while (millis() < timeout) {
      yawAhrs();
    }

    Serial.print("yaw:");
    Serial.println(yaw);
    Serial.print("right: ");
    Serial.println(rightMotorLeast);

    yawAhrs();
    if((yaw > beforeYaw * (1 + error) || yaw < beforeYaw * (1 - error))) break;
    if(rightMotorLeast > 255){
      rightMotorLeast = 0;
      break;
    }
    rightMotorLeast += 10;
  }
  // 왼쪽 모터
  Serial.println("left motor finding!");
  while (1){
    Serial.print("yaw:");
    Serial.println(yaw);
    float beforeYaw = yaw;

    driving(leftMotorLeast, 0);
    timeout = millis() + delayLeastMotor;
    while (millis() < timeout) {
      yawAhrs();
    }
    driving(0, 0);
    timeout = millis() + delayStop;
    while (millis() < timeout) {
      yawAhrs();
    }

    Serial.print("yaw:");
    Serial.println(yaw);
    Serial.print("left: ");
    Serial.println(leftMotorLeast);

    yawAhrs();
    if((yaw) > ( beforeYaw * (1 + error)) || (yaw) < (beforeYaw * (1 - error))) break;
    if(leftMotorLeast > 255){
      leftMotorLeast = 0;
      break;
    }

    leftMotorLeast += 10;
  }
  rightMotorLeast += 10;
  leftMotorLeast += 10;

  Serial.print("yaw:");
  Serial.println(yaw);
  // 약한 모터 찾기
  Serial.println("weak motor finding!");
  if (1){
    float beforeYaw = yaw;
    Serial.print("yaw:");
    Serial.println(yaw);

    driving(leftMotorLeast, rightMotorLeast);
    timeout = millis() + delayWeakMotor;
    while (millis() < timeout) {
      yawAhrs();
    }
    driving(0, 0);
    timeout = millis() + delayStop;
    while (millis() < timeout) {
      yawAhrs();
    }
    driving(-leftMotorLeast, -rightMotorLeast);
    timeout = millis() + delayWeakMotor;
    while (millis() < timeout) {
      yawAhrs();
    }
    driving(0, 0);
    timeout = millis() + delayStop;
    while (millis() < timeout) {
      yawAhrs();
    }
    Serial.print("yaw:");
    Serial.println(yaw);
    yawAhrs();
  if (abs(yaw + beforeYaw) > 360){
    if (yaw > 0) weakMotor = 'B';
    else weakMotor = 'A';
  }
  else{
    if (yaw > beforeYaw) weakMotor = 'A';
    else weakMotor = 'B';
  }
  }

  Serial.println("motor straight finding!");
  // 모터 직선 값 찾기
  int varMotorA = 0;
  int varMotorB = 0;
  
  Serial.print("yaw:");
  Serial.println(yaw);

  Serial.print("weakMotor: ");
  Serial.println(weakMotor);

  if (weakMotor == 'A'){
    varMotorA = leftMotorLeast;
    while (1){
      float beforeYaw = yaw;
      Serial.print("left: ");
      Serial.println(varMotorA);
      Serial.print("right: ");
      Serial.println(rightMotorLeast);

      driving(varMotorA, rightMotorLeast);
      timeout = millis() + delayStraightMotor;
      while (millis() < timeout) {
        yawAhrs();
      }
      driving(0, 0);
      timeout = millis() + delayStraightMotor;
      while (millis() < timeout) {
        yawAhrs();
      }
      driving(-varMotorA, -rightMotorLeast);
      timeout = millis() + delayStraightMotor;
      while (millis() < timeout) {
        yawAhrs();
      }
      driving(0, 0);
      timeout = millis() + delayStop;
      while (millis() < timeout) {
        yawAhrs();
      }

      yawAhrs();
      if (abs(yaw - beforeYaw) >400){
        if (yaw > 0){ 
          varMotorA -= 5;
          break;
        }
      }
      else{
        if (yaw < beforeYaw){
          varMotorA -= 5;
          break;
        }
      }
      varMotorA += 10;
      }
      leftMotorStraight = varMotorA;
      rightMotorStraight = rightMotorLeast;
  }
  else{
    varMotorB = rightMotorLeast;
    while (1){
      float beforeYaw = yaw;
      Serial.print("left: ");
      Serial.println(leftMotorLeast);
      Serial.print("right: ");
      Serial.println(varMotorB);

      driving(leftMotorLeast, varMotorB);
      timeout = millis() + delayStraightMotor;
      while (millis() < timeout) {
        yawAhrs();
      }
      driving(0, 0);
      timeout = millis() + delayStop;
      while (millis() < timeout) {
        yawAhrs();
      }
      driving(-leftMotorLeast, -varMotorB);
      timeout = millis() + delayStraightMotor;
      while (millis() < timeout) {
        yawAhrs();
      }
      driving(0, 0);
      timeout = millis() + delayStop;
      while (millis() < timeout) {
        yawAhrs();
      }
      
      yawAhrs();
      if (abs(yaw - beforeYaw) > 200){
        if (yaw > 0){ 
          varMotorB -= 5;
          break;
        }
      }
      else{
        if (yaw < beforeYaw){
          varMotorB -= 5;
          break;
        }
      }
      varMotorB += 10;
      }
      leftMotorStraight = leftMotorLeast;
      rightMotorStraight = varMotorB;
  }

  // 20cm 거리 도달 시간 측정
  unsigned long arrivalTime;
  String pointColor = "mdf"

  delay(delayDistance);

  while (1){
    if (currentColorName == pointColor) break;
  }
  unsigned long startTime = millis();

  driving(leftMotorStraight, rightMotorStraight);
  delay(delayDistance);
  while(currentColorName != pointColor){
    // 색상 업데이트
    colorName();
  }
  unsigned long actionTime = millis() - startTime;

  delay(delayDistance);
  driving(-leftMotorStraight, -rightMotorStraight);
  delay(actionTime);

  String msg = "[motorDeviation]|" + String(leftMotorLeast) + "|" + String(rightMotorLeast) + "|" +
                String(leftMotorStraight) + "|" + String(rightMotorStraight) + "|" + String(actionTime);
  return msg;
}

// 모터 작동 함수
void driving(int leftMotorValue, int rightMotorValue){
  // 왼쪽 모터 출력
  if (leftMotorValue >= 0) {
    ledcWrite(motorAIn1, leftMotorValue);
    ledcWrite(motorAIn2, 0);
  }
  else {
    ledcWrite(motorAIn1, 0);
    ledcWrite(motorAIn2, -leftMotorValue);
  } 

  // 오른쪽 모터 출력
  if (rightMotorValue >= 0) {
    ledcWrite(motorBIn1, rightMotorValue);
    ledcWrite(motorBIn2, 0);
  }
  else {
    ledcWrite(motorBIn1, 0);
    ledcWrite(motorBIn2, -rightMotorValue);
  } 
}

//-------------------------------------실행---------------------------------------------------------//


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

    driving(0, 0);

    Wire.begin(i2cSda, i2cScl);
    if (!tcs.begin()) {
        Serial.println("No TCS34725 found ... check your connections");
        while (1);
    }
}

void loop() {
  
    char packetBuffer[255];
    int packetSize = udp.parsePacket();
    
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
            // data(startTime, motorAState, motorBState);
            sendMsg(packetBuffer);
        }

        if (aa == 1) {
            motorAState = motorA;
            motorBState = motorB;
            if (strcmp(packetBuffer, "w") == 0) {
                Serial.println("advance");
                driving(motorAState, motorBState);
                data(startTime, motorAState, motorBState);
            } else if (strcmp(packetBuffer, "a") == 0) {
                motorBState = 200;
                driving(0, motorBState);
                data(startTime, motorAState, motorBState);
            } else if (strcmp(packetBuffer, "d") == 0) {
                motorAState = 200;
                driving(motorAState, 0);
                data(startTime, motorAState, motorBState);
            } else if (strcmp(packetBuffer, "s") == 0) {
                driving(-motorAState, -motorBState);
                data(startTime, motorAState, motorBState);
            } else if (strcmp(packetBuffer, "i") == 0) {
                driving(0, 0);
                data(startTime, motorAState, motorBState);
                
            } else if (strcmp(packetBuffer, "[colorAdjust]") == 0){
              colorAdjust();
            } else if (strcmp(packetBuffer, "ahrs") == 0) {
                yawAhrs();
                String msg = "[ahrs]" + String(yaw, 2);
                sendMsg(msg);
            } else if (strcmp(packetBuffer, "color") == 0) {
                tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
                currentLux = tcs.calculateLux(currentR, currentG, currentB);
                String msg = "[color]" + String(currentLux) + "," + String(currentR) + "," + String(currentG) + "," + String(currentB);
                sendMsg(msg);
            } else if(strcmp(packetBuffer, "stop") == 0) {
                driving(0, 0);
                Serial.println("stop!!!!!!!!!!!!!!!!!!!");
                delay(5000);   
            } else if(strcmp(packetBuffer, "[die]") == 0) {
                driving(0, 0);
                delay(1000);
            } else if(strcmp(packetBuffer, "[name]") == 0) {
              tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
              currentLux = tcs.calculateLux(currentR, currentG, currentB);
              currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);
              sendMsg(currentColorName);
            } else if(strcmp(packetBuffer,  "=") == 0){
              motorDeviation(0.2);
            }
            if(cc>0 && cc<30){
                  String msg = "[save]";
                  sendMsg(msg);
            }
    
 

        }
    }
    // if(aa==1){
    //   colorName();
    // }
}