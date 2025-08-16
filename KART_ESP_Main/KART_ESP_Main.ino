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
int MOTOR_A_state = 200;
int MOTOR_B_state = 250;

// AHRS 설정
#define MAX_LINE_LENGTH 64
HardwareSerial AHRS_Serial(2);
float Roll = 0, Pitch = 0, Yaw = 0.1, start_yaw = 0;
char line[MAX_LINE_LENGTH];
int lineIndex = 0;

// 컬러 센서 설정
// #define MAX_COLORS 6  // 색상 수 고정
// #define ATTR_COUNT 5  // color, lux, r, g, b
// String tuning[MAX_COLORS][ATTR_COUNT];
String tokens[200];          // 1차원 배열로 파싱된 결과
String tuning[8][5];         // 최종 2차원 배열
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

//구간 데이터
int section_num = 1;

#define row  13
#define col  2
String section[row][col] = {
  {"red", "red"}, //1
  {"wood", "wood"}, //2
  {"blue", "green"}, //31,32
  {"wood", "wood"}, //4
  {"pink", "orange"},//51,52
  {"wood", "wood"},//6
  {"red", "red"},//7
  {"wood", "wood"},//8
  {"pink", "orange"},//91,92
  {"wood", "wood"},//10
  {"blue", "green"},//111,112
  {"wood", "wood"},//12
  {"red", "red"}//13
};

// 시간 저장
unsigned long start_time = 0;
unsigned long current_time = 0;


bool conected_with_server = true;

int start_segment = 20;
int end_segment   = 130; //확인 필요
int log_exchange_segment = 70; //확인 필요
int last_segment = 0;

bool log_exchange_mode = false;
int my_junction_log[4] = {0,0,0,0};
;int my_junction_log_index = 0;
int others_junction_log[4] = {0,0,0,0};

unsigned long started_time  = 0;
unsigned long ended_time = 0;
unsigned long duration_time = 0;
unsigned long playtime = 0;
unsigned int penalty_time  = 3000;
unsigned int penalty_count = 0;


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
    int sec_num,
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
    currentColorName = color_define(currentLux, currentR, currentG, currentB, tuningSize);

    // 전송
    String msg = "[record]" + String(sec_num) +"|" + String(start_time) + "|" + String(current_time) + "|" +
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

<<<<<<< HEAD
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
      Serial.printf("Sending data: %s\n", msgBuffer);        
}

void motor_stop(){
    ledcWrite(MOTOR_A_IN1, 0);
    ledcWrite(MOTOR_A_IN2, 0);
    ledcWrite(MOTOR_B_IN1, 0);
    ledcWrite(MOTOR_B_IN2, 0);
}

bool segment_log_recv() {
    char packetBuffer[255];
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(packetBuffer, 255);
        packetBuffer[len] = '\0';
        //Serial.print("Received: ");
        //Serial.println(packetBuffer);

        if (strcmp(packetBuffer, "success") == 0) {
            //Serial.println("connecting success:");
            return true;
        }
        else if (strncmp(packetBuffer, "[junction_log]", 14) == 0) {
            String receivedLog = String(packetBuffer).substring(14);
            int commaIndex = 0;
            int prevCommaIndex = -1;
            for(int k=0; k<4; k++){
                commaIndex = receivedLog.indexOf('|', prevCommaIndex + 1);
                String segmentStr;
                if(commaIndex == -1) {
                    segmentStr = receivedLog.substring(prevCommaIndex + 1);
                } else {
                    segmentStr = receivedLog.substring(prevCommaIndex + 1, commaIndex);
                }
                others_junction_log[k] = segmentStr.toInt();
                prevCommaIndex = commaIndex;
                if(commaIndex == -1){
                    udp.beginPacket(PC1_IP, sendPort);
                    udp.write((const uint8_t*)"success", strlen("success"));
                    udp.endPacket();
                    return true;
                }
            }
            /*
            Serial.print("Updated others_junction_log: [");
            for(int k=0; k<4; k++) {
                Serial.print(others_junction_log[k]);
                Serial.print(",");
            }
            Serial.println("]");
            */
            udp.beginPacket(PC1_IP, sendPort);
            udp.write((const uint8_t*)"success", strlen("success"));
            udp.endPacket();
            return true;
        }
    }
    return false;
}

void segment_log_send(String msg) {
    char msgBuffer[128];
    msg.toCharArray(msgBuffer, sizeof(msgBuffer));
    udp.beginPacket(PC1_IP, sendPort);
    udp.write((const uint8_t*)msgBuffer, strlen(msgBuffer));
    udp.endPacket();
    //Serial.printf("Sending data: %s\n", msgBuffer);
}

//데이터 파싱, 컬러 데이터 전용
void data_parsing(String msg) {
    String result[6][5];  // 데이터를 저장할 2차원 String 배열
    String initial = "color_data|";
    int initial_len = initial.length();

    if (msg.startsWith(initial)) {
      msg = msg.substring(initial_len);  // "color_data|" 접두사 제거

      int rowIndex = 0;
      int colIndex = 0;
      int startIndex = 0;
      int endIndex = 0;

      Serial.print("V: [");  // 디버깅을 위한 출력 시작

      // 데이터를 파싱하고 inputValue에 저장하는 루프
      while (rowIndex < 6 && (endIndex = msg.indexOf('|', startIndex)) != -1) {
        String dataPoint = msg.substring(startIndex, endIndex);
        result[rowIndex][colIndex] = dataPoint;  // 데이터 저장

        Serial.print(result[rowIndex][colIndex]);  // 저장된 값 출력 (디버깅용)
        Serial.print(", ");

        colIndex++;
        if (colIndex >= 5) {  // 현재 행이 꽉 차면 다음 행으로 이동
          colIndex = 0;
          rowIndex++;
        }
        startIndex = endIndex + 1;  // 다음 데이터 검색을 위해 인덱스 업데이트
      }

      // 마지막 데이터 포인트 처리 (뒤에 '|'가 없는 경우)
      if (rowIndex < 6 && colIndex < 5 && startIndex < msg.length()) {
        String lastDataPoint = msg.substring(startIndex);
        result[rowIndex][colIndex] = lastDataPoint;
        Serial.print(result[rowIndex][colIndex]);
      }

      Serial.print("]\n");  // 디버깅 출력 종료
      Serial.println("\n--- Parsed inputValue Array ---");
      for (int r = 0; r < 6; r++) {  // Iterate through rows
        Serial.print("{");
        for (int c = 0; c < 5; c++) {  // Iterate through columns
          Serial.print("\"");
          Serial.print(result[r][c]);
          Serial.print("\"");
          if (c < 4) {  // Don't print comma after the last element in a row
            Serial.print(", ");
          }
        }
        Serial.print("}");
        if (r < 5) {  // Don't print comma after the last row
          Serial.println(",");
        }
      }
      Serial.println("\n-------------------------------\n");
    } else {
      Serial.print("M: ");
      Serial.println(msg);
    }
}

void control_by_segment(int segment = 10) {
    if (conected_with_server == true){
      if (segment == start_segment){
          started_time = millis();
          String message = "[playtime-start]" + String(playtime)
                        + "|" + String(duration_time) + "|" + String(penalty_time) 
                        + "|" + String(started_time)  + "|" + String(ended_time);
          segment_log_send(message);
      } else if (segment == end_segment){
          ended_time = millis();
          duration_time = ended_time - started_time;
          playtime = duration_time + (penalty_count*penalty_time);
          motor_stop();
          String message = "[playtime-end]" + String(playtime)
                        + "|" + String(duration_time) + "|" + String(penalty_count) 
                        + "|" + String(started_time)  + "|" + String(ended_time);
          segment_log_send(message);
          //추가 데이터 전송
      } else if (segment == log_exchange_segment){
          log_exchange_mode = true;
          motor_stop();
          String message = "[junction_log]";
          for(int i=0; i<4; i++){
                message += String(my_junction_log[i]);
                if(i < 3) message += "|";
          }
          while (log_exchange_mode == true){
            segment_log_send(message);
            if (segment_log_recv()){
              log_exchange_mode = false;
            }
            delay(10);
          }
      } else if (segment != 0 && segment > last_segment){
          if (segment %2 != 0 && my_junction_log_index < 4) { // 배열 범위 체크
              my_junction_log[my_junction_log_index] = segment%10;
              if (segment > log_exchange_segment && my_junction_log_index < 4){
                  if (segment%10 == others_junction_log[my_junction_log_index]){
                    penalty_count++;
                  }
              }
              if (my_junction_log_index < 3){                
                my_junction_log_index++;
              }
          }
          String message = "[current_segment]" + String(segment);
          segment_log_send(message);
          last_segment = segment;
        }
    }
=======
// 구간 판별 함수
int classify(int sec_num){
    if (sec_num>20){
      sec_num /=10;
    }

    tcs.getRawData(&currentR, &currentG, &currentB, &currentC);
    uint16_t currentLux = tcs.calculateLux(currentR, currentG, currentB);
    String ColorName = color_define(currentLux, currentR, currentG, currentB, tuning, tuningSize);

    if (section[sec_num-1][0] != ColorName  || section[sec_num-1][1] != ColorName){
      if (section[sec_num][0] == ColorName  || section[sec_num][1] == ColorName){
        currentColorName = ColorName;
        if (sec_num%2 == 0){
          if(section[sec_num][0] == section[sec_num][1]){
            sec_num++;
          }
          else if (ColorName == section[sec_num][0]){
            sec_num = (sec_num+1)*10 + 1;
          }
          else{
            sec_num = (sec_num+1)*10 + 2;
          }
        }
        else{
          sec_num++;
        }
      }
      else{
        Serial.println("sameColor: " + ColorName);
      }
    }
    if (section_num/10 == sec_num ){
      return section_num;
    }
    return sec_num;
>>>>>>> recording
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

    if(WiFi.localIP()=="192.168.0.2"){
        sendPort = 4213;
        recvPort = 4212;
    }

    else if(WiFi.localIP()=="192.168.0.12"){
        sendPort = 7000;
        recvPort = 7001;
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

    section_num = classify(section_num);

    if (packetSize) {
        // color_name();

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
<<<<<<< HEAD
            MOTOR_A_state = 200;
=======
            MOTOR_A_state = 250;
>>>>>>> recording
            MOTOR_B_state = 250;
            if (strcmp(packetBuffer, "w") == 0) {
                ledcWrite(MOTOR_A_IN1, MOTOR_A_state);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, MOTOR_B_state);
                ledcWrite(MOTOR_B_IN2, 0);
                // data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "a") == 0) {
<<<<<<< HEAD
                MOTOR_B_state = 150;
=======
                MOTOR_A_state = 0;
                MOTOR_B_state = 130;
>>>>>>> recording
                ledcWrite(MOTOR_A_IN1, 0);
                ledcWrite(MOTOR_A_IN2, 0);
                ledcWrite(MOTOR_B_IN1, MOTOR_B_state);
                ledcWrite(MOTOR_B_IN2, 0);
                // data(start_time, MOTOR_A_state, MOTOR_B_state, start_yaw);
            } else if (strcmp(packetBuffer, "d") == 0) {
                MOTOR_A_state = 160;
<<<<<<< HEAD
=======
                MOTOR_B_state = 0;

>>>>>>> recording
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
    //int current_segment = 32; //테스트용값
    //conected_with_server = aa;
    //control_by_segment(current_segment);
}
