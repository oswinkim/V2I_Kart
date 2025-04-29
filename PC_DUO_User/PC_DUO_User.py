import numpy as np
import cv2
import socket
import keyboard
import time

# UDP 설정
UDP_IP = "0.0.0.0"  # 모든 네트워크 인터페이스에서 수신
#UDP_IP = "192.168.137.15"
UDP_PORT = 4222     # ESP32와 동일한 포트 사용
CHUNK_SIZE = 1024   # ESP32-CAM에서 설정한 청크 크기

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
jpeg_buffer = bytearray()  # JPEG 데이터를 저장할 버퍼
reciving = False

# PC1과 통신 설정
#'''
PC1_IP = "192.168.137.188"  # car
PC1_PORT = 4210            # PC1이 키 입력을 받을 포트
PC2_RECV_PORT = 4211       # PC1에서 데이터를 받을 포트
'''
PC1_IP = "192.168.137.142"  # PC1 IP
PC1_PORT = 5005            # PC1이 키 입력을 받을 포트
PC2_RECV_PORT = 5006       # PC1에서 데이터를 받을 포트
#'''


sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_send.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)  # 송신 버퍼 크기 증가

sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind(("", PC2_RECV_PORT))  # 온도 데이터 수신 소켓
sock_recv.settimeout(0.0)  # Non-blocking 모드 (딜레이 제거)

# 키 입력 상태
keys = np.array([False] * 4)
prev_keys = np.array([False] * 4)  # 이전 키 상태 저장
ov_color = (220, 200, 20)

prev_keys
key_map = {'s': 0, 'w': 1, 'a': 2, 'd': 3}

# 온도 데이터 수신 함수
def receive_temperature():
    try:
        data, _ = sock_recv.recvfrom(1024)
        temp = data.decode().strip()
        if temp:
            print(f"Received Temperature from PC1: {temp}°C")
    except BlockingIOError:
        pass  # 데이터가 없으면 넘어감
    except UnicodeDecodeError:
        print("Error: Received data could not be decoded.")  # 디코딩 오류 방지

# 키 입력 감지 및 전송 (중복 전송 방지)
def send_key_input():
    for key, index in key_map.items():
        keys[index] = keyboard.is_pressed(key)
        # 키 상태 변경 시 전송
        if keys[index] != prev_keys[index]:
            prev_keys[index] = keys[index]
            if keys[index]:  # 키가 눌렸을 때만 전송
                try:
                    sock_send.sendto(str(index).encode(), (PC1_IP, PC1_PORT))
                    #sock_send.sendto(key.encode(), (PC1_IP, PC1_PORT))
                    #time.sleep(0.01)  # 너무 빠른 전송 방지
                except OSError as e:
                    print(f"UDP Send Error: {e}")
        

# 메인 루프
while True:
    #send_key_input()       # 키 입력 처리

    #'''
    try:
        data, addr = sock.recvfrom(CHUNK_SIZE)  # UDP 데이터 수신

        # JPEG 시작(0xFFD8) 감지 시, 기존 버퍼 초기화
        if data[:3] == b'\xff\xd8\xff':
            jpeg_buffer = bytearray()
            reciving = True

        if reciving:
            #receive_temperature()  # 온도 데이터 수신
            send_key_input()       # 키 입력 처리
            jpeg_buffer.extend(data)  # 데이터 추가

            # JPEG 끝(0xFFD9) 감지 시, 이미지 디코딩
            if len(data) >= 2 and data[-2:] == b'\xff\xd9':
                frame = np.frombuffer(jpeg_buffer, dtype=np.uint8)  # NumPy 배열 변환
                image = cv2.imdecode(frame, cv2.IMREAD_COLOR)  # OpenCV 디코딩
                jpeg_buffer = bytearray()
                reciving = False

                if image is not None:
                    # 이미지 크기 확대
                    image = cv2.resize(image, dsize=(image.shape[1] * 2, image.shape[0] * 2), interpolation=cv2.INTER_AREA)

                    # 화살표 그리기
                    arrowKey = np.array([
                        [[200, 285], [150, 200], [250, 200]],   # ↓
                        [[200, 100], [150, 185], [250, 185]],  # ↑
                        
                        [[25, 200], [100, 250], [100, 150]],   # ←
                        [[385, 200], [300, 250], [300, 150]]   # →
                    ], dtype=np.int32)

                    for i in range(arrowKey.shape[0]):
                        if keys[i]:
                            image = cv2.fillPoly(image, [arrowKey[i]], ov_color, cv2.LINE_AA)
                        image = cv2.polylines(image, [arrowKey[i]], True, ov_color, 5)

                    # 화면에 출력
                    cv2.imshow("ESP32-CAM Stream", image)
                    if cv2.waitKey(1) & 0xFF == 27:  # esc 키를 누르면 종료
                        break
    except:
        pass
    #'''
    

# 리소스 정리
sock.close()
#sock_recv.close()
sock_send.close()
cv2.destroyAllWindows()
