import numpy as np
import cv2
import socket
import keyboard
import time

# 네트워크 설정
MY_IP = socket.gethostbyname(socket.gethostname())  # 모든 네트워크 인터페이스에서 수신
# ESPcam 영상 스트리밍 경로 설정
vid_path = 0#"http://192.168.0.12:81/stream"
# PC1 통신 설정
PC1_IP = "192.168.0.7"  # car
PC1_PORT = 5005            # PC1이 키 입력을 받을 포트
PC2_RECV_PORT = 5006       # PC1에서 데이터를 받을 포트
sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_send.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)  # 송신 버퍼 크기 증가
sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind(("", PC2_RECV_PORT))  # 온도 데이터 수c신 소켓
sock_recv.settimeout(0.0)  # Non-blocking 모드 (딜레이 제거)

# 키 입력 상태
keys = np.array([False] * 9)
prev_keys = np.array([False] * 9)  # 이전 키 상태 저장
key_map = {'w': 0, 'a': 1, 's': 2, 'd': 3, 'i': 4}
ov_color = (220, 200, 20)
arrowKey = np.array([
    [[200, 285], [150, 200], [250, 200]],   # ↓
    [[200, 100], [150, 185], [250, 185]],  # ↑
    [[25, 200], [100, 250], [100, 150]],   # ←
    [[385, 200], [300, 250], [300, 150]]   # →
], dtype=np.int32)

# 데이터 수신 함수
def receive_data():
    try:
        data, _ = sock_recv.recvfrom(1024)
        message = data.decode().strip()
        if message:
            print(f"Received data from PC1: {message}")
    except BlockingIOError:
        pass  # 데이터가 없으면 넘어감
    except UnicodeDecodeError:
        print("Error: Received data could not be decoded.")  # 디코딩 오류 방지

# 키 입력 감지 및 전송 (중복 전송 방지)
def send_key_input():
    if keyboard.is_pressed('esc'):
        return False
    for key, index in key_map.items():
        keys[index] = keyboard.is_pressed(key)
        # 키 상태 변경 시 전송
        if keys[index] != prev_keys[index]:
            prev_keys[index] = keys[index]
            if keys[index]:  # 키가 눌렸을 때만 전송
                try:
                    #sock_send.sendto(str(index).encode(), (PC1_IP, PC1_PORT))   # 입력된 키에 해당하는 인덱스를 전달
                    sock_send.sendto(key.encode(), (PC1_IP, PC1_PORT))          # 입력된 키를 전달
                    print("send to PC1: ",str(key))
                except OSError as e:
                    print(f"PC1 Send Error: {e}")

img_size = [1152, 864]
print(sock_recv.getsockname())
img = np.empty((img_size[1], img_size[0], 3), dtype=np.uint8)     
cap = cv2.VideoCapture(vid_path)

win_name = "ESP32-CAM Stream"
cv2.namedWindow(win_name, cv2.WINDOW_AUTOSIZE)
cv2.resizeWindow(win_name, img_size[1], img_size[0])

#연산컴퓨터와 연결
'''
while 1:
    print("try to connect...")
    try:
        data, _ = sock_recv.recvfrom(1024)
        message = data.decode().strip()
        if message:
            print(f"Received data from PC1: {message}")
            if "success" in message:
                print("연산컴퓨터와 연결 성공")
                break
            sock_send.sendto(message.encode(), (PC1_IP, PC1_PORT))
    except BlockingIOError:
        pass  # 데이터가 없으면 넘어감
    except UnicodeDecodeError:
        print("Error: Received data could not be decoded.")  # 디코딩 오류 방지
#'''

# 메인 루프
while True:
    if False == send_key_input():       # 키 입력 처리
        break
    #영상 읽어오기
    frame, img = cap.read()
    #읽어오는 동영상 위치가 잘못되었으면 종료
    if not cap: # ret이 False인 경우 (프레임 읽기 실패)
        print("Failed to grab frame or stream ended. ([T^T])")
        cap.release()
        cap = cv2.VideoCapture(vid_path)
        if not cap.isOpened():
            print("Could not re-open video stream.([T^T])")
            break
        else:
            print("Reconnected to stream.\[+_+]/")
        break # 프레임 읽기 실패 시 루프 종료

    if not frame:
        img = np.empty((img_size[1], img_size[0], 3), dtype=np.uint8)
        #print("Received an empty image frame. ['-']>") # img가 None인 경우

    img = cv2.resize(img, dsize=(img_size[1], int(img_size[1]*(img.shape[0]/img.shape[1]))), interpolation=cv2.INTER_AREA)
    cv2.imshow(win_name, img)

    if cv2.waitKey(1) & 0xFF == 27:  # esc 키를 누르면 종료
        break   

# 리소스 정리
sock_recv.close()
sock_send.close()
cv2.destroyAllWindows()