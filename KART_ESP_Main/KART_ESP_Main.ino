#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "a12";      // WiFi 이름
const char* password = "12345678";  // WiFi 비밀번호

WiFiUDP udp;
const unsigned int recvPort = 4212;  // PC1에서 LED 제어 명령을 받을 포트
const unsigned int sendPort = 4213;  // PC1으로 데이터를 보낼 포트

IPAddress PC1_IP(192, 168, 137, 205);  // PC1의 IP 주소

#define MOTOR_A_IN1 25  // PWM핀
#define MOTOR_A_IN2 26
#define MOTOR_B_IN1 32
#define MOTOR_B_IN2 33

void setup() {
    Serial.begin(115200);
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


}

void loop() {
    char packetBuffer[255];
    int packetSize = udp.parsePacket();

    if (packetSize) {
        udp.read(packetBuffer, 255);
        packetBuffer[packetSize] = '\0';  // 문자열 종료
        Serial.print("Received: ");
        Serial.println(packetBuffer);

        // 모터 제어
        if (strcmp(packetBuffer, "1") == 0) {
            digitalWrite(MOTOR_A_IN1, HIGH);
            digitalWrite(MOTOR_B_IN1, HIGH);

            Serial.println("MORTOR ON");
        } 
        else if (strcmp(packetBuffer, "0") == 0) {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_B_IN1, LOW);

            Serial.println("MORTOR OFF");
        } 
        // 데이터 전송
        else if (strcmp(packetBuffer, "REQ") == 0) {
            Serial.printf("NONE_oh");
            
            /*  온도 전송 예시
            float temp = dht.readTemperature();
            Serial.printf("Sending Temperature: %.2f°C\n", temp);

            char tempStr[10];
            dtostrf(temp, 4, 2, tempStr);  // float → 문자열 변환
            udp.beginPacket(PC1_IP, sendPort);
            udp.write(tempStr);
            udp.endPacket();
            */
        }
    }
}

/*
PC1(서버컴): 망우 17번
PC2(이용자): 망우 16번
공유기:사마의 촉빠 휴대폰 핫스팟
*/