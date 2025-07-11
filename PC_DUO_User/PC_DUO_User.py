import numpy as np
import cv2
import socket
import keyboard
import time

# 네트워크 설정
MY_IP = socket.gethostbyname(socket.gethostname())  # 모든 네트워크 인터페이스에서 수신
# ESPcam 영상 스트리밍 경로 설정
ESP32CAM_IP = "192.168.0.12"
ESP32CAM_PORT = 81

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
keys = np.array([False] * 6)
prev_keys = np.array([False] * 6)  # 이전 키 상태 저장
'''
key_map = {
            'up': (0, 'down'),
            'down': (1, 'up'),
            'left': (2, 'right'),
            'right': (3, 'left'),
            'i': (4, 'i')
        }
'''
key_map = {
            'w': (0, 's'),
            's': (1, 'w'),
            'a': (2, 'd'),
            'd': (3, 'a'),
            'i': (4, 'z'),
            'z': (5, 'i')
        }
#'''
#UI 관련
ov_color = (220, 200, 20)
arrows = np.array([ 
    [[40, 20], [30, 37], [50, 37]],
    [[40, 57], [30, 40], [50, 40]],
    [[5, 40],  [20, 50], [20, 30]], 
    [[77, 40], [60, 50], [60, 30]] ], dtype=np.int32)
center = (-200, -150)
scale = 3
arrows =  (arrows - center) * scale + center

vid_path = 0#f"http://{ESP32CAM_IP}:{ESP32CAM_PORT}/stream"
a = time.time()
cap = cv2.VideoCapture(vid_path)
if not cap.isOpened():
    print(f"time: {time.time()-a}")

img_size = [1152, 864]
img = np.empty((img_size[1], img_size[0], 3), dtype=np.uint8)     

win_name = " "
cv2.namedWindow(win_name, cv2.WINDOW_AUTOSIZE)
cv2.resizeWindow(win_name, img_size[1], img_size[0])

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

last_sent = time.time()
interval = 0.15
auto_send_count = 0
auto_send_max = 3
message = str("eee").encode()
current_message = str("eee").encode()
def auto_send():
    global last_sent
    global message
    global auto_send_count
    global auto_send_max
    global current_message
    if auto_send_max >= auto_send_count:
        if time.time() - last_sent > interval:
            send()
            last_sent = time.time()
            auto_send_count += 1
    if message != current_message:
        auto_send_count = 0
        current_message = message

def send():
    global message
    try:
        sock_send.sendto(message, (PC1_IP, PC1_PORT))   # 입력된 키에 해당하는 인덱스를 전달         
        print("send to PC1: ",message)
    except OSError as e:
        print(f"PC1 Send Error: {e} (try to send: {message})")

# 키 입력 감지 및 전송 (중복 전송 방지)
def send_key_input(event):
    global message
    key = event.name
    if key in key_map:
        idx, opposite = key_map[key]
        if event.event_type == 'down' and not keys[idx]:
            keys[idx] = True
            keys[key_map[opposite][0]] = False
            message = str(key).encode() # 입력된 키를 전달
            #message = str(index).encode() # 입력된 키에 해당하는 인덱스를 전달
            #message = str(keys[:4]).encode()
            send()
        elif event.event_type == 'up' and keys[idx]:
            keys[idx] = False
            message = str(key).encode() # 입력된 키를 전달
            #message = str(index).encode() # 입력된 키에 해당하는 인덱스를 전달
            #message = str(keys[:4]).encode()
            send()

def hand_shake_with_operator():
    #연산컴퓨터와 연결
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

def hand_shake_with_operator(MAX_RETRIES=1024):
    for _ in range(MAX_RETRIES):
        try: # 현재 처리 중인 디바이스의 소켓인지 확인
                data, addr = sock_recv.recvfrom(1024)
                msg = data.decode().strip()
                if msg == "success":
                    print(f"INFO: 연산컴퓨터와 수신 성공")
                    return True
                else:
                    print(f"ERROR: 송신과 관계없는 메시지가 수신됨")
        except BlockingIOError:
            print(f"INFO: 수신이 확인되지 않음. 재전송...")
        except UnicodeDecodeError:
            print("Error: Received data could not be decoded.")  # 디코딩 오류 방지
        time.sleep(0.1)
    print(f"{MAX_RETRIES}번의 확인 시도가 실패함. 처리를 건너뜀")
    return False

# 리소스 정리
def cleanup():
    sock_recv.close()
    sock_send.close()
    cv2.destroyAllWindows()
    print("shut down properly. [^-^]/")


keyboard.hook(send_key_input)
#hand_shake_with_operator()
print(f"MY_IP: {sock_recv.getsockname()}")

# 메인 루프
while True:
    auto_send()
    
    #영상 읽어오기
    frame, img = cap.read()
    #읽어오는 동영상 위치가 잘못되었으면 종료
    if not cap.isOpened():
        img = np.empty((img_size[1], img_size[0], 3), dtype=np.uint8)
    if not cap: # ret이 False인 경우 (프레임 읽기 실패)
        print("Failed to grab frame or stream ended. ([T^T])")
        cap.release()
        cap = cv2.VideoCapture(vid_path)
        if not cap.isOpened():
            print("Could not re-open video stream.([T^T])")
            break
        else:
            print(r"Reconnected to stream.\[+_+]/")
        break # 프레임 읽기 실패 시 루프 종료
    if not frame:
        img = np.empty((img_size[1], img_size[0], 3), dtype=np.uint8)
        #print("Received an empty image frame. ['-']>") # img가 None인 경우

    for i, arrow in enumerate(arrows):
        cv2.polylines(img, [arrow], True, (220, 200, 20), 3)
        if keys[i]:
            cv2.fillPoly(img, [arrow], (220, 200, 20), cv2.LINE_AA)
    img = cv2.resize(img, dsize=(img_size[1], int(img_size[1]*(img.shape[0]/img.shape[1]))), interpolation=cv2.INTER_AREA)
    cv2.imshow(win_name, img)
    cv2.waitKey(1)

    if keyboard.is_pressed('esc'):
        break

cleanup()