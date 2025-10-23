import socket
import keyboard
import numpy as np
import cv2

pc1Ip = "192.168.3.228"  # PC1 IP
pc1Port = 8000           # PC1이 키 입력을 받을 포트
pc2RecvPort = 8000  # PC1에서 데이터를 받을 포트


sockSend = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sockRecv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sockRecv.bind(("", pc2RecvPort))  # 온도 데이터 수신 소켓

# while 1:
#     sockRecv.settimeout(0.1)  # 데이터가 없으면 넘어가기
#     try:
#         data, addr = sockRecv.recvfrom(1024)  # 데이터 수신
#         msg = data.decode().strip()
#         print(f"Received from PC1: {msg}")  
#         if ("success" in msg):
#             break
#         sockSend.sendto(msg.encode(), (PC1_IP, PC1_PORT))
#         print(f"Sent to PC1: {msg}")
                        
#     except socket.timeout:
#         pass  # 데이터 없으면 넘어감


while True:
    try:
        event = keyboard.read_event()
        if event.event_type == keyboard.KEY_DOWN:  # 키 눌림 감지
            key = event.name  # 눌린 키 이름

            if key == "q":  # 'q' 입력 시 종료
                print("Exiting...")
                break

            # PC1로 키 값 전송
            sockSend.sendto(key.encode(), (pc1Ip, pc1Port))
            print(f"Sent to PC1: {key}")

        sockRecv.settimeout(0.1)  # 데이터가 없으면 넘어가기
        try:
            data, addr = sockRecv.recvfrom(1024)  # 데이터 수신
            msg = data.decode().strip()
            print(f"Received from PC1: {msg}")
        except socket.timeout:
            pass  # 데이터 없으면 넘어감

    except KeyboardInterrupt:
        break    