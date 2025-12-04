#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "HardwareSerial.h"
#include "Adafruit_TCS34725.h"
#include <math.h>

// WiFi 설정
WiFiUDP udp;
const char* ssid = "a12";
const char* password = "12345678";
unsigned int recvPort = 7000;   //wifi연결 후 자동 설정
unsigned int sendPort = 7000;
IPAddress pc1Ip(192, 168, 241, 228);

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
int motorAState = 0;
int motorBState = 0;
int motorALeast =190;
int motorBLeast =195;
int motorAMax = 250;
int motorBMax = 255;
int targetMotorA = 0;
int targetMotorB = 0;
int stepUnitA = 14;
int stepUnitB = 14;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long motorDelay = 50000000;
unsigned long Time2ChangeMotorPrevious = 0;
unsigned long Time2ChangeMotorCurrent = 0;
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
String tokens[200];  // 1차원 배열로 파싱된 결과
String tuning[8][5] = {
  { "mdf", "44", "247", "114", "76" },
  { "red", "65485", "337", "60", "50" },
  { "blue", "153", "177", "248", "246" },
  { "green", "196", "184", "203", "88" },
  { "pink", "36", "694", "275", "235" },
  { "orange", "65500", "550", "130", "86" },
  { "sky", "240", "386", "373", "305" },
  { "white", "218", "615", "409", "311" }
};  // 최종 2차원 배열

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
  char key;
};
// 로그 저장 배열 (최대 100)
#define MAX_COMMANDS 300
ReplayCommand commands[MAX_COMMANDS];
int commandCount = 0;
// 상태 변수
bool isReplaying = false;
int currentCommandIndex = 0;
unsigned long lastCommandTime = 0;
String replayBuffer = "";
String preData = "";
bool receivingReplay = false;
String currentState = "i";
//-------------------------------------함수---------------------------------------------------------//

// 메시지 전송 함수
void sendMsg(String msg, int condition = 1, IPAddress ip = pc1Ip, unsigned int port = sendPort) {
  udp.beginPacket(ip, port);
  udp.print(msg);
  udp.endPacket();

  if (condition == 1) Serial.printf("Sending data: %s\n", msg.c_str());
}

// 색상 판단 함수
String colorDefine(uint16_t lux, uint16_t r, uint16_t g, uint16_t b, int tuningSize) {
  int raw[4] = { (int)lux, (int)r, (int)g, (int)b };
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
float yawAhrs() {
  // 최신 AHRS 한 줄 수신
  String latestLine1 = "";
  String latestLine2 = "";
  currentTime = millis();

  while (ahrsSerial.available()) {
    char c = ahrsSerial.read();
    if (c == '\n') {
      latestLine2 = latestLine1;
      latestLine1 = "";
    } else latestLine1 += c;
  }

  // yaw 추출
  float parsedYaw = yaw;
  if (latestLine2.startsWith("$EUL")) {
    int firstComma = latestLine2.indexOf(',');
    int secondComma = latestLine2.indexOf(',', firstComma + 1);
    int thirdComma = latestLine2.indexOf(',', secondComma + 1);

    if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
      String yawStr = latestLine2.substring(thirdComma + 1);
      parsedYaw = yawStr.toFloat();
      yaw = parsedYaw + 180;
    }
    yawDiff = startYaw - yaw;
  }
  // sendMsg("yaw: "+String(yaw));
  return yaw;
}

// 데이터 문자열 생성 함수
String data(
  unsigned long startTime,
  int motorAState,
  int motorBState) {
  //yaw값 측정
  yawAhrs();

  // 컬러 측정
  tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
  currentLux = tcs.calculateLux(currentR, currentG, currentB);
  currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);

  // 전송
      String msg = "[record]"+ currentState + "|" + String(startTime) + "|" + String(currentTime) + "|" +
                  String(motorAState) + "|" + String(motorBState) + "|" + String(yawDiff, 2) + "|" +
                  currentColorName + "|" + String(currentLux) + "|" +
                  String(currentR) + "|" + String(currentG) + "|" + String(currentB) + "|" +
                  String(yaw, 2);

  return msg;
}

// 컬러센서 보정용 데이터 생성 함수
String sendRawColor(String name) {
  String Tuning[6][5];
  int luxAvg = 0, rAvg = 0, gAvg = 0, bAvg = 0;
  for (int i = 0; i < 5; i++) {
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
  luxAvg = luxAvg / 5;
  rAvg = rAvg / 5;
  gAvg = gAvg / 5;
  bAvg = bAvg / 5;

  Tuning[5][0] = name;
  Tuning[5][1] = String(luxAvg);
  Tuning[5][2] = String(rAvg);
  Tuning[5][3] = String(gAvg);
  Tuning[5][4] = String(bAvg);

  String msg = "[raw_color]";
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 5; j++) {
      msg += "|" + Tuning[i][j];
    }
  }
  return msg;
}

// 색상 보정 함수
void colorAdjust() {
  while (1) {
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
      } else if (bb == 1) {
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
        } else if (packetStr.startsWith("colorData")) {
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
          for (int i = 0; i + 4 < count && colorIndex < 8; i += 5) {
            tuning[colorIndex][0] = parts[i];      // name
            tuning[colorIndex][1] = parts[i + 1];  // clear
            tuning[colorIndex][2] = parts[i + 2];  // red
            tuning[colorIndex][3] = parts[i + 3];  // green
            tuning[colorIndex][4] = parts[i + 4];  // blue
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
  bb = 0;
}

// 색상 이름 업데이트 함수
void colorName() {
  tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
  currentLux = tcs.calculateLux(currentR, currentG, currentB);
  currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);
}


// 모터 작동 함수
void driving(int leftMotorValue, int rightMotorValue) {
  // 왼쪽 모터 출력
  if (leftMotorValue >= 0) {
    ledcWrite(motorAIn1, leftMotorValue);
    ledcWrite(motorAIn2, 0);
  } else {
    ledcWrite(motorAIn1, 0);
    ledcWrite(motorAIn2, -leftMotorValue);
  }

  // 오른쪽 모터 출력
  if (rightMotorValue >= 0) {
    ledcWrite(motorBIn1, rightMotorValue);
    ledcWrite(motorBIn2, 0);
  } else {
    ledcWrite(motorBIn1, 0);
    ledcWrite(motorBIn2, -rightMotorValue);
  }
}

// 모터 편차 조정 함수
String motorDeviation(float error, int transmit = 1) {
  int leftMotorLeast = 100, rightMotorLeast = 100;
  int leftMotorStraight = 0, rightMotorStraight = 0;
  char weakMotor = 'A';
  unsigned long delayLeastMotor = 1500, delayWeakMotor = 3000, delayStraightMotor = 3000, delayDistance = 2000, delayStop = 500;
  String head = "[motorDeviation]";

  // AHRS 의 값이 측정 되지 않았을 경우
  while (yaw == 0) {
    yawAhrs();
    Serial.println("yaw value renewal");
    if (transmit) sendMsg(head + "yaw value renewal");
  }

  driving(0, 0);
  delay(delayStop);

  // 모터 최소 작동값 찾기
  Serial.println("[Finding least value]");
  Serial.println("right motor finding!");
  if (transmit) sendMsg(head + "[Finding least value]");
  if (transmit) sendMsg(head + "right motor finding!");
  // 오른쪽 모터
  yawAhrs();
  while (1) {
    Serial.print("yaw:");
    Serial.println(yaw);
    float beforeYaw = yaw;

    driving(0, rightMotorLeast);

    delay(delayLeastMotor);

    driving(0, 0);

    delay(delayStop);
    yawAhrs();
    yawAhrs();
    Serial.print("yaw:");
    Serial.println(yaw);
    Serial.print("right: ");
    Serial.println(rightMotorLeast);
    yawAhrs();
    if ((yaw > beforeYaw + error || yaw < beforeYaw - error)) {
      driving(0, -rightMotorLeast);
      delay(delayLeastMotor);
      driving(0, 0);
      delay(delayStop);
      break;
    }

    if (rightMotorLeast > 255) {
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
  while (1) {
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
    yawAhrs();
    if ((yaw) > (beforeYaw + error) || (yaw) < (beforeYaw - error)) {
      driving(-leftMotorLeast, 0);
      delay(delayLeastMotor);
      driving(0, 0);
      delay(delayStop);
      break;
    }
    if (leftMotorLeast > 255) {
      leftMotorLeast = 0;
      Serial.println("Broken LeftMotor");
      if (transmit) sendMsg(head + "Broken LeftMotor");
      break;
    }
    leftMotorLeast += 10;
  }
  stepUnitA = leftMotorLeast/33.0 + 0.5;
  stepUnitB = rightMotorLeast/33.0 + 0.5;
  Serial.printf("stepUnitA: %d\n", stepUnitA);
  Serial.printf("stepUnitB: %d\n", stepUnitB);  
  Serial.printf("[origin]leftMotorLeast: %d\n", leftMotorLeast);
  Serial.printf("[origin]rightMotorLeast: %d\n", rightMotorLeast);
  leftMotorLeast = leftMotorLeast*(180.0/165.0) + 0.5;
  rightMotorLeast = rightMotorLeast*(180.0/165.0) + 0.5;
  Serial.printf("[after]leftMotorLeast: %d\n", leftMotorLeast);
  Serial.printf("[after]rightMotorLeast: %d\n", rightMotorLeast);


  // 180도 부근으로 방향 보정
  Serial.println("locating");
  if (transmit) sendMsg(head + "locating");
  if (transmit) sendMsg(head + "yaw" + String(yaw));
  yawAhrs();
  while (yaw < 170 || yaw > 190) {
      driving(leftMotorLeast, -rightMotorLeast);
      yawAhrs();
      Serial.printf("yaw: %f\n",yaw);
  }
  driving(0,0);
  delay(delayWeakMotor);

  // 약한 모터 찾기
  Serial.println("weak motor finding!");
  if (transmit) sendMsg(head + "weak motor finding!");
  if (1) {
    yawAhrs();
    float beforeYaw = yaw;
    Serial.print("yaw:");
    Serial.println(yaw);

    driving(leftMotorLeast, rightMotorLeast);
    delay(delayWeakMotor);
    driving(0, 0);
    delay(delayStop);
    yawAhrs();
    driving(-leftMotorLeast, -rightMotorLeast);
    delay(delayWeakMotor);
    driving(0, 0);
    delay(delayStop);

    Serial.print("yaw:");
    Serial.println(yaw);

    if (yaw > beforeYaw) weakMotor = 'B';
    else weakMotor = 'A';

    Serial.printf("weakMotor = %c\n", weakMotor);
    if (transmit) sendMsg(head + "weakMotor = " + weakMotor);
  }


  // 180도 부근으로 방향 보정
  Serial.println("locating");
  if (transmit) sendMsg(head + "locating");
  if (transmit) sendMsg(head + "yaw" + String(yaw));
  yawAhrs();
  while (yaw < 170 || yaw > 190) {
    while (yaw < 170) {
      driving(leftMotorLeast, -rightMotorLeast);
      yawAhrs();
    }
    driving(0, 0);
    delay(delayStop);
    while (yaw > 190) {
      driving(-leftMotorLeast, rightMotorLeast);
      yawAhrs();
    }
    driving(0, 0);
    delay(delayStop);
  }

  // 모터 직선 값 찾기
  Serial.println("motor straight finding!");
  if (transmit) sendMsg(head + "motor straight finding!");
  int varMotorA = 0;
  int varMotorB = 0;

  Serial.print("yaw:");
  Serial.println(yaw);

  Serial.print("weakMotor: ");
  Serial.println(weakMotor);

  if (weakMotor == 'A') {
    varMotorA = leftMotorLeast;
    while (1) {
      yawAhrs();
      float beforeYaw = yaw;
      Serial.print("left: ");
      Serial.println(varMotorA);
      Serial.print("right: ");
      Serial.println(rightMotorLeast);

      driving(varMotorA, rightMotorLeast);
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);

      yawAhrs();
      if (yaw > beforeYaw) {
        varMotorA -= 5;
        break;
      }
      driving(-varMotorA, -rightMotorLeast);
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);

      varMotorA += 10;
    }
    leftMotorStraight = varMotorA;
    rightMotorStraight = rightMotorLeast;
  } else {
    varMotorB = rightMotorLeast;
    while (1) {
      yawAhrs();
      float beforeYaw = yaw;
      Serial.print("left: ");
      Serial.println(leftMotorLeast);
      Serial.print("right: ");
      Serial.println(varMotorB);

      driving(leftMotorLeast, varMotorB);
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);

      yawAhrs();
      if (yaw < beforeYaw) {
        varMotorB -= 5;
        break;
      }
      driving(-leftMotorLeast, -varMotorB);
      delay(delayStraightMotor);
      driving(0, 0);
      delay(delayStop);

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

  String msg = "[motorDeviation]|" + String(stepUnitA) + "|" + String(stepUnitB) + "|" +
                String(leftMotorStraight) + "|" + String(rightMotorStraight) + "|" + String(actionTime)+ 
                "|" + String(stepUnitA)+ "|" + String(stepUnitB);

  motorALeast = leftMotorStraight;
  motorBLeast = rightMotorStraight;
  sendMsg("over calibration!");
  return msg;
}

void parseReplayBuffer() {
  Serial.println("=== 전체 데이터 파싱 시작 ===");
  Serial.println(replayBuffer);

  commandCount = 0;

  int rowStart = 0;
  int rowEnd = replayBuffer.indexOf('|');

  while (rowEnd != -1 && commandCount < MAX_COMMANDS) {
    String row = replayBuffer.substring(rowStart, rowEnd);

    int c1 = row.indexOf(',');
    int c2 = row.indexOf(',', c1 + 1);
    int c3 = row.indexOf(',', c2 + 1);
    int c4 = row.indexOf(',', c3 + 1);

    if (c1 != -1 && c2 != -1 && c3 != -1 && c4 != -1) {
      commands[commandCount].delayMs = row.substring(0, c1).toInt();
      commands[commandCount].leftMotor = row.substring(c1 + 1, c2).toInt();
      commands[commandCount].rightMotor = row.substring(c2 + 1, c3).toInt();
      commands[commandCount].logValue = row.substring(c3 + 1).toFloat();
      commands[commandCount].key = row.substring(c4 + 1)[0];
      commandCount++;
    }

    rowStart = rowEnd + 1;
    rowEnd = replayBuffer.indexOf('|', rowStart);
  }

  Serial.printf("총 %d개 명령 파싱 완료\n", commandCount);

  // Replay 시작
  currentCommandIndex = 0;
  isReplaying = true;
  lastCommandTime = millis();
}

void replayLogSave(char packetBuffer[255]) {
  String received = String(packetBuffer);
  received.trim();

  Serial.print("replayLogSave(received): ");
  Serial.println(received);
  if (received.startsWith("[replay_start]")) {
    replayBuffer = "";
    receivingReplay = true;
    sendMsg("[replay_start]");
    Serial.println("리플레이 시작 신호 수신 → 버퍼 초기화");
    return;
  }

  if (received.startsWith("[replay_end]")) {
    receivingReplay = false;
    sendMsg("[replay_end]");
    Serial.println("리플레이 종료 신호 수신 → 파싱 시작");
    parseReplayBuffer();
    return;
  }

  if (received.startsWith("[replay_data]")) {
    String dataPart = received.substring(strlen("[replay_data]"));
    if (preData == dataPart) return;
    preData = dataPart;
    Serial.print("preData: ");
    Serial.println(preData);
    Serial.print("dataPart: ");
    Serial.println(dataPart);
    replayBuffer += dataPart + "|";  // 안전하게 구분자 추가
    sendMsg(received);
    Serial.println("데이터 청크 수신 및 버퍼에 추가됨");
    return;
  }

  Serial.print("알 수 없는 패킷: ");
  Serial.println(received);
}
float replayTargetAHRS(float startAHRS, float addAHRS, char keyState = 'f') {
  float finalAHRS;
  if (startAHRS + addAHRS >= 0 && startAHRS + addAHRS <= 360) {
    finalAHRS = startAHRS + addAHRS;
  }

  if (keyState == 'd') {
    if (startAHRS + addAHRS >= 360) {
      finalAHRS = startAHRS + addAHRS - 360;
    }
  }
  else if (keyState == 'a') {
    if (startAHRS + addAHRS < 0) {
      finalAHRS = 360 + startAHRS + addAHRS;
    }
  }
  else {
    Serial.printf("replayTargetAHRS에서 키 입력 오류, 키 값: %c\n", keyState);
  }

  return finalAHRS;
}


void pathReproduction() {
  if (!isReplaying) return;
  Serial.println("ready to pathReproduction");
  Serial.printf("currentCommandIndex: %d\n", currentCommandIndex);
  Serial.printf("commandCount: %d\n", commandCount);
  while (currentCommandIndex < commandCount) {
    ReplayCommand cmd = commands[currentCommandIndex];

    Serial.printf("[REPLAY] #%d: delay=%lu, L=%d, R=%d, log=%.2f, key=%c\n",
    currentCommandIndex, cmd.delayMs, cmd.leftMotor, cmd.rightMotor, cmd.logValue, cmd.key);
    
    if (cmd.key == 'a' || cmd.key == 'd') {
      float targetYaw = replayTargetAHRS(yaw, cmd.logValue, cmd.key);
      while ((yawAhrs() - targetYaw) > 10) {
        if (cmd.key == 'a') {
          driving(0, motorBLeast);
        }
        else if (cmd.key == 'd') {
          driving(motorALeast, 0);
        }
      }
      driving(0, 0);
    }
    else {
      driving(cmd.leftMotor, cmd.rightMotor);
      // if (cmd.key == 'a' || cmd.key =='d') delay(cmd.delayMs+500);
      // else if ((cmd.key == 'w' || cmd.key =='s')&&(cmd.delayMs>150)) delay(cmd.delayMs-150);
      // else delay(cmd.delayMs);
      delay(cmd.delayMs);
    }    
      currentCommandIndex++;
  }
  Serial.println("=== 모든 명령 재생 완료 ===");
  isReplaying = false;

  driving(0, 0);  // 정지

  // 명령 초기화
  for (int i = 0; i < MAX_COMMANDS; i++) {
    commands[i] = {0, 0, 0, 0.0};
  }
  commandCount = 0;
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

  if (WiFi.localIP() == "192.168.0.14") {
    sendPort = 4213;
    recvPort = 4212;
    motorALeast = 250;
    motorBLeast = 250;
  }

  else if (WiFi.localIP() == "192.168.0.18") {
    sendPort = 7000;
    recvPort = 7001;
    motorALeast = 250;
    motorBLeast = 250;
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
  delay(2000);
  // while(1){
  //     yawAhrs();
  //     Serial.printf("yaw: %f\n",yaw);
  // }
}
void loop() {
    Time2ChangeMotorCurrent = millis();

    if (motorAState!=targetMotorA || motorBState != targetMotorB){
      if (Time2ChangeMotorCurrent-Time2ChangeMotorPrevious >= motorDelay){
        Time2ChangeMotorPrevious = Time2ChangeMotorCurrent;
        if (motorAState < targetMotorA){
          motorAState += stepUnitA;
          if (motorAState>0) {
            if (motorAState<motorALeast) motorAState=motorALeast;
            else if (motorAState>motorAMax) motorAState=motorAMax;
          }
          else if (motorAState>-motorALeast) motorAState=0;
        }
        else if (motorAState > targetMotorA){
          motorAState -= stepUnitA;
          if (motorAState<0) {
            if (motorAState>-motorALeast) motorAState=-motorALeast;
            else if (motorAState<-motorAMax) motorAState=-motorAMax;
          }
          else if (motorAState<motorALeast) motorAState=0;
        }

        if (motorBState < targetMotorB){
          motorBState += stepUnitB;
          if (motorBState>0) {
            if (motorBState<motorBLeast) motorBState=motorBLeast;
            else if (motorBState>motorBMax) motorBState=motorBMax;
          }
          else if (motorBState>-motorBLeast) motorBState=0;
        }
        else if (motorBState > targetMotorB){
          motorBState -= stepUnitB;
          if (motorBState<0) {
            if (motorBState>-motorBLeast) motorBState=-motorBLeast;
            else if (motorBState<-motorBMax) motorBState=-motorBMax;
          }
          else if (motorBState<motorBLeast) motorBState=0;
        }
        Serial.printf("A:%d, TargetA:%d\n", motorAState, targetMotorA);
        Serial.printf("B:%d, TargetB:%d\n", motorBState, targetMotorB);
        driving(motorAState, motorBState);
        sendMsg(data(startTime, motorAState, motorBState));
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      else{
        motorAState = targetMotorA;
        motorBState = targetMotorB;
        Serial.printf("A:%d, TargetA:%d\n", motorAState, targetMotorA);
        Serial.printf("B:%d, TargetB:%d\n", motorBState, targetMotorB);
        driving(motorAState, motorBState);
        // sendMsg(data(startTime, motorAState, motorBState));   
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

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
            // u
            sendMsg(packetBuffer);
        }
        if (aa == 1) {
            if (strcmp(packetBuffer, "w") == 0) {
                currentState = "w";
                Serial.println("advance");
                targetMotorA = motorAMax;
                targetMotorB = motorBMax;
                sendMsg(data(startTime, targetMotorA, targetMotorB));
            } else if (strcmp(packetBuffer, "a") == 0) {
                currentState = "a";
                targetMotorA = 0;
                targetMotorB = motorBMax;
                sendMsg(data(startTime, targetMotorA, targetMotorB));
            } else if (strcmp(packetBuffer, "d") == 0) {
                currentState = "d";
                targetMotorA = motorAMax;
                targetMotorB = 0;
                sendMsg(data(startTime, targetMotorA, targetMotorB));
            } else if (strcmp(packetBuffer, "s") == 0) {
                currentState = "s";
                targetMotorA = -motorAMax;
                targetMotorB = -motorBMax;
                sendMsg(data(startTime, targetMotorA, targetMotorB));
            } else if (strcmp(packetBuffer, "i") == 0) {
                currentState = "i";
                targetMotorA = 0;
                targetMotorB = 0;
                sendMsg(data(startTime, targetMotorA, targetMotorB));
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
                targetMotorA = 0;
                targetMotorB = 0;
                Serial.println("stop!!!!!!!!!!!!!!!!!!!");
                sendMsg(data(startTime, targetMotorA, targetMotorB));
            } else if(strcmp(packetBuffer, "[die]") == 0) {
                targetMotorA = 0;
                targetMotorB = 0;
                sendMsg(data(startTime, targetMotorA, targetMotorB));
            } else if(strcmp(packetBuffer, "[name]") == 0) {
              tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
              currentLux = tcs.calculateLux(currentR, currentG, currentB);
              currentColorName = colorDefine(currentLux, currentR, currentG, currentB, tuningSize);
              sendMsg(currentColorName);
            } else if(strcmp(packetBuffer,  "=") == 0){
              sendMsg(motorDeviation(20));
            } 
            // else if (strcmp(packetBuffer, "[replay_start]") == 0) {
            //   replayLogSave(packetBuffer);
            //   isReplaying = true;
            // }
            if (isReplaying) {
              // isReplaying = false;
              pathReproduction();
            }
            if(cc>0 && cc<30){
                  String msg = "[save]";
                  sendMsg(msg);
            }
            
          }

        if (strcmp(packetBuffer, "i") == 0) {
          // isReplaying = false;
          driving(0, 0);
          Serial.println("강제정지");
        } 
        replayLogSave(packetBuffer);

        if (isReplaying) {
              // isReplaying = false;
              pathReproduction();
              }
    }
    // if(aa==1){
    //   colorName();
    // }

    // if (isReplaying) {
    //   pathReproduction();
    // }
}