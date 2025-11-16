#include "HardwareSerial.h"

// AHRS 설정
#define maxLineLength 64
HardwareSerial ahrsSerial(2);
float roll = 0, pitch = 0, yaw = 0, startYaw = 0, yawDiff = 0;
char line[maxLineLength];
int lineIndex = 0;

// 시간 저장
unsigned long startTime = 0;
unsigned long currentTime = 0;

//-------------------------------------함수---------------------------------------------------------//

// yaw값 업데이트 함수
void yawAhrs(){
  // 최신 AHRS 한 줄 수신
  String latestLine1 = "";
  String latestLine2 = "";
  int count = 0;
  unsigned long startTime = millis();
  while (ahrsSerial.available()) {
    char c = ahrsSerial.read();
    if (c == '\n') {
      latestLine2 = latestLine1;
      latestLine1 = "";
    }
    else latestLine1 += c;
    count++;
  }
  unsigned long duringTime = millis() - startTime;
  Serial.println();
  Serial.print("runningTime: ");
  Serial.println(duringTime);
  Serial.print("count: ");
  Serial.println(count);
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
}


//-------------------------------------실행---------------------------------------------------------//


void setup() {

  Serial.begin(9600);
  ahrsSerial.begin(115200, SERIAL_8N1, 34, -1);

}
int a = 0;
int timet = 500;
void loop() {
  yawAhrs();
  Serial.print("number: ");
  Serial.println(a);
  Serial.println(yaw);
  delay(timet);
  Serial.print("delayTime: ");
  Serial.println(timet);
  a++;
  timet += 500;
}