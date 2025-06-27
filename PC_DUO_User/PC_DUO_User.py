import numpy as np
import cv2
import socket
import keyboard
import time

# 네트워크 설정
MY_IP = socket.gethostbyname(socket.gethostname())  # 모든 네트워크 인터페이스에서 수신
# ESPcam 영상 스트리밍 경로 설정
# vid_path = "http://192.168.43.21:81/stream"
vid_path=0  #자체카메라
# PC1 통신 설정
PC1_IP = "192.168.191.53"  # car
PC1_PORT = 5005            # PC1이 키 입력을 받을 포트
PC2_RECV_PORT = 5006       # PC1에서 데이터를 받을 포트
sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_send.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)  # 송신 버퍼 크기 증가
sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind(("", PC2_RECV_PORT))  # 온도 데이터 수신 소켓
sock_recv.settimeout(0.0)  # Non-blocking 모드 (딜레이 제거)

# 키 입력 상태
keys = np.array([False] * 4)
prev_keys = np.array([False] * 4)  # 이전 키 상태 저장
key_map = {'f': 0, 'o': 1, 't': 2}
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
    try:
        key = keyboard.read_key()  # 사용자가 누른 키를 읽음
        print("send to PC1:", key)
        sock_send.sendto(key.encode(), (PC1_IP, PC1_PORT))
    except OSError as e:
        print(f"PC1 Send Error: {e}")



print(sock_recv.getsockname())
img = np.empty((240, 320, 3), dtype=np.uint8)     
#cap = cv2.VideoCapture(vid_path)
# 메인 루프
while True:

    try:
        receive_data()
        send_key_input()       # 키 입력 처리

    except:
        pass
    #영상 읽어오기
    '''  
    # frame, img = cap.read()
    #읽어오는 동영상 위치가 잘못되었으면 종료
      
    if not cap.isOpened():
        print("file open failed! ([T^T])")
        break
    #프레임이 없다면 종료
    elif not frame:
        print("frame over!  ['-']>")
        break


    cv2.resize(img, dsize=(img.shape[1] * 2, img.shape[0] * 2), interpolation=cv2.INTER_AREA)
    # 화면에 출력
    cv2.imshow("ESP32-CAM Stream", img)
  
    #time.sleep(1)  # 너무 빠른 업데이트 방지
    '''
    if cv2.waitKey(1) & 0xFF == 27:  # esc 키를 누르면 종료
        break 

# 리소스 정리
sock_recv.close()
sock_send.close()
cv2.destroyAllWindows()