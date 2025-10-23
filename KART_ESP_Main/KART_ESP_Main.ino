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

// 리플레이 로그 저장
struct ReplayCommand {
  unsigned long delayMs;  // 첫 번째 값: 시간 간격 (ms)
  int leftMotor;          // 두 번째 값: 좌측 모터 (int16)
  int rightMotor;         // 세 번째 값: 우측 모터 (int16)
  float logValue;         // 네 번째 값: 로깅 값 (float)
};
// 로그 저장 배열 (최대 100)
#define MAX_COMMANDS 100
ReplayCommand commands[MAX_COMMANDS];
int commandCount = 0;
// 상태 변수
bool isReplaying = false;
int currentCommandIndex = 0;
unsigned long lastCommandTime = 0;
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
  String latestLine1 = "";
  String latestLine2 = "";
  currentTime = millis();

  while (ahrsSerial.available()) {
    char c = ahrsSerial.read();
    if (c == '\n') {
      latestLine2 = latestLine1;
      latestLine1 = "";
    }
    else latestLine1 += c;
  }
  
  // yaw 추출
  float parsedYaw = yaw;
  if (latestLine2.startsWith("$EUL")) {
    int firstComma  = latestLine2.indexOf(',');
    int secondComma = latestLine2.indexOf(',', firstComma + 1);
    int thirdComma  = latestLine2.indexOf(',', secondComma + 1);

    if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
      String yawStr = latestLine2.substring(thirdComma + 1);
      parsedYaw = yawStr.toFloat();
      yaw = parsedYaw + 180;
      }
    yawDiff = startYaw - yaw;
  }
  sendMsg("yaw: "+String(yaw));
}

// 데이터 문자열 생성 함수
String data(
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

      return msg;
}

// 컬러센서 보정용 데이터 생성 함수
String sendRawColor(String name){
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
  return msg;
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
            sendMsg(sendRawColor(name));
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

// 색상 이름 업데이트 함수
void colorName(){
  tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
  currentLux = tcs.calculateLux(currentR, currentG, currentB);
  currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);
}

// 모터 편차 조정 함수
String motorDeviation(float error, int transmit = 1){
  int leftMotorLeast = 100, rightMotorLeast = 100;
  int leftMotorStraight = 0, rightMotorStraight = 0;
  char weakMotor = 'A';
  unsigned long delayLeastMotor = 3000, delayWeakMotor = 5000, delayStraightMotor = 3000, delayDistance = 2000 ,delayStop = 500;
  String head = "[motorDeviation]";

  while (yaw == 0) {
  yawAhrs();
  Serial.println("yaw value renewal");
  if (transmit) sendMsg(head + "yaw value renewal");
  }

  delay(delayStop);
  yawAhrs();

  // 모터 최소 작동값 찾기
  Serial.println("[Finding least value]");
  Serial.println("right motor finding!");
  if (transmit) sendMsg(head + "[Finding least value]");
  if (transmit) sendMsg(head + "right motor finding!");
  // 오른쪽 모터
  yawAhrs();
  while (1){
    Serial.print("yaw:");
    Serial.println(yaw);
    float beforeYaw = yaw;

    driving(0, rightMotorLeast);

    delay(delayLeastMotor);

    driving(0, 0);

    delay(delayStop);

    yawAhrs();
    Serial.print("yaw:");
    Serial.println(yaw);
    Serial.print("right: ");
    Serial.println(rightMotorLeast);

    if((yaw > beforeYaw + error || yaw < beforeYaw  - error)) {
      driving(0, -rightMotorLeast);
      delay(delayLeastMotor);
      driving(0, 0);
      delay(delayStop);
      break;
    }

    if(rightMotorLeast > 255){
      rightMotorLeast = 0;
      Serial.println("Broken RightMotor");
      if (transmit) sendMsg(head + "Broken RightMotor");
      break;
    }
    rightMotorLeast += 10;
  }
  // 왼쪽 모터
  Serial.println("left motor finding!");
  if (transmit) sendMsg(head + "left motor finding!");
  while (1){
    yawAhrs();
    Serial.print("yaw:");
    Serial.println(yaw);
    float beforeYaw = yaw;

    driving(leftMotorLeast, 0);
    delay(delayLeastMotor);
    driving(0, 0);
    delay(delayStop);
    
    yawAhrs();
    Serial.print("yaw:");
    Serial.println(yaw);
    Serial.print("left: ");
    Serial.println(leftMotorLeast);

    yawAhrs();
    if((yaw) > ( beforeYaw + error) || (yaw) < (beforeYaw - error)){
      driving(-leftMotorLeast, 0);
      delay(delayLeastMotor);
      driving(0, 0);
      delay(delayStop);
      break;
    }
    if(leftMotorLeast > 255){
      leftMotorLeast = 0;
      Serial.println("Broken LeftMotor");
      if (transmit) sendMsg(head + "Broken LeftMotor");
      break;
    }
    leftMotorLeast += 10;
  }
  // 180도 부근으로 방향 보정
  yawAhrs();
  while(yaw < 170 || yaw > 190){
    while(yaw < 170){
      driving(leftMotorLeast, -rightMotorLeast);
      yawAhrs();
    }
    driving(0, 0);
    delay(delayStop);
    while(yaw > 190){
      driving(-leftMotorLeast, rightMotorLeast);
      yawAhrs();
    }
    driving(0, 0);
    delay(delayStop);
  }

  // 약한 모터 찾기
  Serial.println("weak motor finding!");
  if(transmit) sendMsg(head + "weak motor finding!");
  if (1){
    yawAhrs();
    float beforeYaw = yaw;
    Serial.print("yaw:");
    Serial.println(yaw);

    driving(leftMotorLeast, rightMotorLeast);
    delay(delayWeakMotor);
    driving(0, 0);
    delay(delayStop);
    driving(-leftMotorLeast, -rightMotorLeast);
    delay(delayWeakMotor);
    driving(0, 0);
    delay(delayStop);

    yawAhrs();
    Serial.print("yaw:");
    Serial.println(yaw);

    if (yaw > beforeYaw) weakMotor = 'B';
    else weakMotor = 'A';
  
    Serial.printf("weakMotor = %c\n",weakMotor);
    if(transmit) sendMsg(head + "weakMotor = " + weakMotor);
  }

  // 180도 부근으로 방향 보정
  yawAhrs();
  while(yaw < 170 || yaw > 190){
    while(yaw < 170){
      driving(leftMotorLeast, -rightMotorLeast);
      yawAhrs();
    }
    driving(0, 0);
    delay(delayStop);
    while(yaw > 190){
      driving(-leftMotorLeast, rightMotorLeast);
      yawAhrs();
    }
    driving(0, 0);
    delay(delayStop);
  }

  // 모터 직선 값 찾기
  Serial.println("motor straight finding!");
  if(transmit) sendMsg(head + "motor straight finding!");
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
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);
      driving(-varMotorA, -rightMotorLeast);
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);

      yawAhrs();
      if (yaw > beforeYaw){
        varMotorA -= 5;
        break;
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
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);
      driving(-leftMotorLeast, -varMotorB);
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);      

      yawAhrs();
      if (yaw < beforeYaw){
        varMotorB -= 5;
        break;
      }
      varMotorB += 10;
      }
      leftMotorStraight = leftMotorLeast;
      rightMotorStraight = varMotorB;
  }

  // // 20cm 거리 도달 시간 측정
  // Serial.println("during time measuring!");
  // unsigned long arrivalTime;
  // String pointColor = "mdf";

  // delay(delayDistance);

  // while (1){
  //   if (currentColorName == pointColor) break;
  // }
  // unsigned long startTime = millis();

  // driving(leftMotorStraight, rightMotorStraight);
  // delay(delayDistance);
  // while(currentColorName != pointColor){
  //   // 색상 업데이트
  //   colorName();
  // }
  // unsigned long actionTime = millis() - startTime;

  // delay(delayDistance);
  // driving(-leftMotorStraight, -rightMotorStraight);
  // delay(actionTime);

  // test value
  unsigned long actionTime = 0;

  String msg = "[motorDeviation]|" + String(leftMotorLeast) + "|" + String(rightMotorLeast) + "|" +
                String(leftMotorStraight) + "|" + String(rightMotorStraight) + "|" + String(actionTime);

  motorA = leftMotorStraight;
  motorB = rightMotorStraight;
  sendMsg("over calibration!");
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

void replayLogSave(char packetBuffer[255]) {
  String receivedString(packetBuffer);

  // 포맷으로 시작하는지
  if (!receivedString.startsWith("[replay]")) {
    Serial.println("알 수 없는 포맷");
    return;
  }
  // ACK 메시지 전송
  sendMsg(receivedString);
  // [포맷] 접두사 제거
  String dataString = receivedString.substring(receivedString.indexOf("[replay]") + 8);

  // |로 행 분리
  int rowStartIndex = 0;
  int rowEndIndex = dataString.indexOf('|');
  commandCount = 0;

  while (rowEndIndex != -1 && commandCount < MAX_COMMANDS) {
    String rowString = dataString.substring(rowStartIndex, rowEndIndex);
    // ,로 열 분리
    int commaIndex1 = rowString.indexOf(',');
    int commaIndex2 = rowString.indexOf(',', commaIndex1 + 1);
    int commaIndex3 = rowString.indexOf(',', commaIndex2 + 1);

    if (commaIndex1 != -1 && commaIndex2 != -1 && commaIndex3 != -1) {
      unsigned long delayMs = rowString.substring(0, commaIndex1).toInt();
      int leftMotor = rowString.substring(commaIndex1 + 1, commaIndex2).toInt();
      int rightMotor = rowString.substring(commaIndex2 + 1, commaIndex3).toInt();
      float logValue = rowString.substring(commaIndex3 + 1).toFloat();
      //저징
      commands[commandCount] = {delayMs, leftMotor, rightMotor, logValue};
      commandCount++;
    }
    // 다음 행
    rowStartIndex = rowEndIndex + 1;
    rowEndIndex = dataString.indexOf('|', rowStartIndex);
  }
  if (rowStartIndex < dataString.length() && commandCount < MAX_COMMANDS) {
    String lastRowString = dataString.substring(rowStartIndex);

    int commaIndex1 = lastRowString.indexOf(',');
    int commaIndex2 = lastRowString.indexOf(',', commaIndex1 + 1);
    int commaIndex3 = lastRowString.indexOf(',', commaIndex2 + 1);

    if (commaIndex1 != -1 && commaIndex2 != -1 && commaIndex3 != -1) {
      unsigned long delayMs = lastRowString.substring(0, commaIndex1).toInt();
      int leftMotor = lastRowString.substring(commaIndex1 + 1, commaIndex2).toInt();
      int rightMotor = lastRowString.substring(commaIndex2 + 1, commaIndex3).toInt();
      float logValue = lastRowString.substring(commaIndex3 + 1).toFloat();

      commands[commandCount] = {delayMs, leftMotor, rightMotor, logValue};
      commandCount++;
    }
  }

  for (int i = 0; i < commandCount; i++) {
    Serial.printf("번호 %d: 대기=%lu, 좌=%d, 우=%d, ahrs=%.2f\n",
                  i, commands[i].delayMs, commands[i].leftMotor, commands[i].rightMotor, commands[i].logValue);
  }
  isReplaying = true; // 데이터 파싱 후 리플레이
  currentCommandIndex = 0;
  lastCommandTime = millis();
}

void pathReproduction(){
    //  현재 인덱스가 유효하다면
    if (currentCommandIndex < commandCount) {
      ReplayCommand currentCommand = commands[currentCommandIndex];
      // 시간차 계산
      if (millis() - lastCommandTime >= currentCommand.delayMs) {
        Serial.printf("현재명령(%d): 대기=%lu, 좌=%d, 우=%d\n",
                      currentCommandIndex, currentCommand.delayMs, currentCommand.leftMotor, currentCommand.rightMotor);
        // 모터 업데이트
        driving(currentCommand.leftMotor, currentCommand.rightMotor);
        // 인덱스 업데이트
        lastCommandTime = millis();
        currentCommandIndex++;
      }
    } else {
      isReplaying = false;
      driving(0, 0);
      Serial.println("모든 명령 실행 완료");
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
        yawAhrs();

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
            // sendMsg(data(startTime, motorAState, motorBState));
            sendMsg(packetBuffer);
        }

        if (aa == 1) {
            motorAState = motorA;
            motorBState = motorB;
            if (strcmp(packetBuffer, "w") == 0) {
                Serial.println("advance");
                driving(motorAState, motorBState);
                sendMsg(data(startTime, motorAState, motorBState));
            } else if (strcmp(packetBuffer, "a") == 0) {
                driving(0, motorBState);
                sendMsg(data(startTime, motorAState, motorBState));
            } else if (strcmp(packetBuffer, "d") == 0) {
                driving(motorAState, 0);
                sendMsg(data(startTime, motorAState, motorBState));
            } else if (strcmp(packetBuffer, "s") == 0) {
                driving(-motorAState, -motorBState);
                sendMsg(data(startTime, motorAState, motorBState));
            } else if (strcmp(packetBuffer, "i") == 0) {
                driving(0, 0);
                sendMsg(data(startTime, motorAState, motorBState));
                
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
              motorDeviation(5);
            }
            if(cc>0 && cc<30){
                  String msg = "[save]";
                  sendMsg(msg);
            }
    
 

        }

        if (strcmp(packetBuffer, "i") == 0) {
          isReplaying = false;
          driving(0, 0);
          Serial.println("강제정지");
        } else if (strncmp(packetBuffer, "[replay]", 8) == 0) {
            replayLogSave(packetBuffer);
            isReplaying = true;
        }

    }
    // if(aa==1){
    //   colorName();
    // }

    if (isReplaying) {
      pathReproduction();
    }
}