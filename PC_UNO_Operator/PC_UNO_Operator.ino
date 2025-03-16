import socket
import select

# ESP32 관련 설정
ESP_IP = "192.168.0.0"  # ESP32 IP
ESP_RECV_PORT = 4210      # ESP32가 명령을 받을 포트
ESP_SEND_PORT = 4211      # ESP32가 데이터를 보낼 포트

# PC2 관련 설정
PC2_IP = "192.168.0.0"  # PC2 IP
PC2_RECV_PORT = 5005  # PC2에서 키 입력을 받을 포트
PC2_SEND_PORT = 5006  # PC2로 데이터를 보낼 포트
 

# UDP 소켓 생성 및 바인딩
sock_pc2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_pc2.bind(("", PC2_RECV_PORT))  # PC2에서 키 입력을 받을 소켓

sock_esp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_esp.bind(("", ESP_SEND_PORT))  # ESP32가 온도 데이터를 보낼 소켓

sock_pc2_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # ✅ PC2로 온도 전송용 소켓

# 감시할 소켓 리스트
sockets = [sock_pc2, sock_esp]

print("Waiting for key input from PC2 and data from ESP32...")

while True:
    try:
        # select를 사용하여 먼저 오는 데이터 처리
        readable, _, _ = select.select(sockets, [], [])

        for sock in readable:
            data, addr = sock.recvfrom(1024)  # 데이터 수신
            msg = data.decode().strip()

            # PC2에서 온 키 입력 처리
            if sock == sock_pc2:
                print(f"Received from PC2: {msg}")

                if msg == "o":
                    sock_esp.sendto("1".encode(), (ESP_IP, ESP_RECV_PORT))  # motor ON
                    print("Sent to ESP: motor ON")
                elif msg == "f":
                    sock_esp.sendto("0".encode(), (ESP_IP, ESP_RECV_PORT))  # motor OFF
                    print("Sent to ESP: motor OFF")

                # 추가 명령어<추후 데이터 전송용>
                # elif msg == "t":
                #     sock_esp.sendto("REQ".encode(), (ESP_IP, ESP_RECV_PORT))  # 온도 요청
                #     print("Requesting temperature from ESP32...")

            # ESP32에서 온도 데이터 수신 → PC2로 전달
            elif sock == sock_esp:
                print(f"Received Temperature from ESP: {msg}°C")
                sock_pc2_send.sendto(msg.encode(), (PC2_IP, PC2_SEND_PORT))  # ✅ PC2로 온도 전송
                print(f"Sent Temperature to PC2: {msg}°C")

    except KeyboardInterrupt:
        print("Exiting...")
        break
