import numpy as np
import cv2
import socket
import keyboard
import time
import select
from datetime import datetime
import os
from PIL import ImageFont, ImageDraw, Image

'''
D:\2405_WIFIcar\V2IKart_fork\PC_DUO_User\PC_DUO_User.py

D:\2405_WIFIcar\V2IKart_fork\PC_UNO_Operator\PC_UNO_Operator.py
'''

# 네트워크 설정
MY_IP = socket.gethostbyname(socket.gethostname())  # 모든 네트워크 인터페이스에서 수신
# ESPcam 영상 스트리밍 경로 설정



rat_name = "러너"
cat_name = "헌터"
role = cat_name



'''
#빨간자동차(러너)
ESP32CAM_IP = "192.168.0.15"
PC1_PORT = 8000            # PC1이 키 입력을 받을 포트
MY_PORT = 8001       # PC1에서 데이터를 받을 포트
#파란 자동차(헌터)
'''
ESP32CAM_IP = "192.168.0.15"
PC1_PORT = 5006            # PC1이 키 입력을 받을 포트
MY_PORT = 5006       # PC1에서 데이터를 받을 포트
#'''

ESP32CAM_PORT = 81
vid_path = f"http://{ESP32CAM_IP}:{ESP32CAM_PORT}/stream"
# PC1 통신 설정
PC1_IP = "192.168.0.7"

MySock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
MySock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536) # 수신 버퍼를 64KB로 설정
MySock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536) # 송신 버퍼를 64KB로 설정
MySock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 포트 재사용 허용
MySock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1) # 브로드캐스트 허용
MySock.settimeout(0.0)
MySock.bind(("", MY_PORT))  # 데이터 수신 소켓

#---이미지 및 창 관련---
# 이미지 및 창 크기
width_height_ratio = 1.33
window_img_ratio = 0.85
target_window_size = np.array([1280, 720], dtype=np.uint16)
target_img_size = np.array([int(target_window_size[1]*window_img_ratio*width_height_ratio), 
                            int(target_window_size[1]*window_img_ratio)], dtype=np.uint16)
# 윈도우 세팅
win_name = " "
cv2.namedWindow(win_name, cv2.WINDOW_AUTOSIZE)
cv2.resizeWindow(win_name, target_img_size[0], target_img_size[1])
# 이미지 불러오기
etime = time.time()

img = np.empty((target_img_size[1], target_img_size[0], 3), dtype=np.uint8)
img_canvas = np.empty((target_window_size[1], target_window_size[0], 3), dtype=np.uint8)

# ---UI 오버레이 관련---
# UI 색상
ov_color = (220, 200, 20)
# 화살표
scale = 3
target_position = np.array([115, 600], dtype=np.int16)
arrows = np.array([ 
    [[0, 0], [0, 0], [0, 0]],
    [[40, 20], [30, 37], [50, 37]],
    [[40, 57], [30, 40], [50, 40]],
    [[5, 40],  [20, 50], [20, 30]], 
    [[77, 40], [60, 50], [60, 30]] ], dtype=np.int32)
arrows[0] = np.mean(arrows[1:5].reshape(-1, 2), axis=0).astype(np.int32)
arrows = (arrows + arrows[0]) * scale - arrows[0]
arrows = arrows + (target_position - arrows[0, 0])

#---키입력 관련---
# 키 입력 상태
#'''
#'''
key_map = {
            'up': ('w', 'down'),
            'down': ('s', 'up'),
            'left': ('a', 'right'),
            'right': ('d', 'left'),
            'space': ('space', 'enter'),
            'enter': ('enter', 'space')
        }
'''
key_map = {
            'w': (0, 's'),
            's': (1, 'w'),
            'a': (2, 'd'),
            'd': (3, 'a'),
            'space': (4, 'enter'),
            'enter': (5, 'space')
        }

'''
#'''
key_map = {
            'up': (0, 'down', 'w'),
            'down': (1, 'up', 's'),
            'left': (2, 'right', 'a'),
            'right': (3, 'left', 'd'),
            'space': (4, 'enter', 'space'),
            'enter': (5, 'space', 'enter')
        }
#'''
current_keys = np.array([False] * len(key_map), np.int8)
prev_keys    = np.array([False] * len(key_map), np.int8)  # 이전 키 상태 저장

# 데이터 수신 함수
def receive_data(buffer_size = 1024):
    try:
        received_bytes, address = MySock.recvfrom(buffer_size)
        return received_bytes, address
    except BlockingIOError:
        return None, None
    except UnicodeDecodeError:
        print("Error: Received data could not be decoded.")  # 디코딩 오류 방지
        return None, None
    except Exception as e:
        print(f"receive error MySock - {e}")
        return None, None

last_sent = time.time()
interval = 0.25
auto_send_count = 0
auto_send_max = 1 
message = 'space'
current_message = 'space'
def auto_send():
    global last_sent
    global message
    global auto_send_count
    global auto_send_max
    global current_message
    if auto_send_max >= auto_send_count and message != "":
        if time.time() - last_sent > interval:
            UDP_send()
            last_sent = time.time()
            auto_send_count += 1
    if message != current_message:
        auto_send_count = 0
        current_message = message

def UDP_send():
    global message
    try:
        MySock.sendto(message.encode(), (PC1_IP, PC1_PORT))   # 입력된 키에 해당하는 인덱스를 전달         
        #print("send to PC1: ",message.encode())
    except OSError as e:
        print(f"PC1 Send Error: {e} (try to send: {message.encode()})")

# 키 입력 감지 및 전송 (중복 전송 방지)
def send_key_input(event):
    global message
    key = event.name

    if key in key_map:
        idx, opposite, message = key_map[key]
        if event.event_type == 'down' and not current_keys[idx]:
            current_keys[idx] = True
            current_keys[key_map[opposite][0]] = False
            #message = str(key) # 입력된 키를 전달
            message = str(message) # 입력된 키에 해당하는 인덱스를 전달
            #message = str(current_keys[:4])
            UDP_send()
        elif event.event_type == 'up' and current_keys[idx]:
            current_keys[idx] = False
            #message = str(key) # 입력된 키를 전달
            message = str(message) # 입력된 키에 해당하는 인덱스를 전달
            #message = str(current_keys[:4])
            UDP_send()













def hand_shake_with_operator(MAX_RETRIES=1024):
    for _ in range(MAX_RETRIES):
        try: # 현재 처리 중인 디바이스의 소켓인지 확인
                data, addr = MySock.recvfrom(1024)
                msg = data.decode().strip()
                if msg:
                    print(f"INFO: 연산컴퓨터의 데이터를 수신 성공 ({msg})")
                    MySock.sendto(msg.encode(), (PC1_IP, PC1_PORT))
                    if "success" in msg:
                        print("연산컴퓨터와 연결 성공")
                        return True
                    else:
                        print(f"ERROR: 송신과 관계없는 메시지가 수신됨 ({msg})")
        except BlockingIOError:
            print(f"INFO: 수신이 확인되지 않음. 재전송...")
        except UnicodeDecodeError:
            print("ERROR: 수신받은 데이터를 디코딩 할 수 없음")
        time.sleep(0.1)
    print(f"{MAX_RETRIES}번의 확인 시도가 실패함. 처리를 건너뜀")
    return False














# 리소스 정리
def cleanup():
    MySock.close()
    cv2.destroyAllWindows()
    print("shut down properly. [^-^]/")

# 메인 앱
if __name__ == "__main__":
    print("\n\n\n\n")
    print("------------------------------------------------------------------------------------------")
    print(f"DEBUG: 프로그램 시작:            {datetime.now()}")
    print(f"DEBUG: 나의 소켓 정보:           {MY_IP}:{MY_PORT}")
    print(f"DEBUG: 나의 코드 위치:           {os.path.abspath(__file__)}") 
    print("------------------------------------------------------------------------------------------")
    print(f"DEBUG: 정상적으로 연결되지 않을 경우, 네트워크 설정을 확인하십시오.(공용 -> 개인 네트워크)")
    print("------------------------------------------------------------------------------------------")
    print("")

    keyboard.hook(send_key_input)
    #연산컴퓨터와 연결
    # while 1:
    #     try:
    #         data, _ = MySock.recvfrom(1024)
    #         message = data.decode().strip()
    #         if message:
    #             print(f"Received data from PC1: {message}")
    #             if "success" in message:
    #                 print("연산컴퓨터와 연결 성공")
    #                 break
    #             MySock.sendto(message.encode(), (PC1_IP, PC1_PORT))
    #     except BlockingIOError:
    #         pass  # 데이터가 없으면 넘어감
    #     except UnicodeDecodeError:
    #         print("Error: Received data could not be decoded.")  # 디코딩 오류 방지

    print(f"MY_IP: {MySock.getsockname()}")

    font_fath = "Freesentation-9Black.ttf"
    normal_font = ImageFont.truetype(font_fath, 30)
    big_font = ImageFont.truetype(font_fath, 100)

    color = ""
    
    skill = False
    penalty = True
    skill_cool = 0
    goal = "[색상 구역 중심에서 스킬로 해제]"
    skill_to_kor = "--"
    penalty_time_start =  time.time()
    game_over = False
    
    

    '''
    cap = cv2.VideoCapture(vid_path)
    if not cap.isOpened():
        print(f"connect to http server error. time: {time.time()-etime}")
    '''
   




    # 메인 루프
    while True:
        #auto_send()
        #영상 읽어오기
        '''
        frame, img = cap.read()
        #읽어오는 동영상 위치가 잘못되었으면 종료
        if not cap.isOpened():
            img = np.empty((target_img_size[0], target_img_size[1], 3), dtype=np.uint8)
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
            img = np.empty((target_img_size[1], target_img_size[0], 3), dtype=np.uint8)
            #print("Received an empty image frame. ['-']>") # img가 None인 경우
        '''
        img = np.empty((target_img_size[1], target_img_size[0], 3), dtype=np.uint8) #지워야함

        # 이미지 크기 재지정
        img = cv2.resize(img, dsize=(target_img_size[0], target_img_size[1]), interpolation=cv2.INTER_AREA)
        img = cv2.flip(cv2.flip(img, 1), 0)
        # img를 img_canvas 중앙에 배치
        img_canvas[:] = 0
        img_size = img.shape[:2][::-1]
        img_offset = np.array(((target_window_size[0] - img.shape[1]) // 2, (target_window_size[1] - img.shape[0]) // 2), dtype = np.uint16)
        img_canvas[img_offset[1]:img_offset[1]+img_size[1], img_offset[0]:img_offset[0]+img_size[0], :] = img

        ##
        for i, arrow in enumerate(arrows[1:]):
            img_canvas = cv2.polylines(img_canvas, [arrow], True, (250, 220, 100), 3)
            if current_keys[i]:
                img_canvas = cv2.fillPoly(img_canvas, [arrow], (250, 220, 100), cv2.LINE_AA)

        pil_img = Image.fromarray(img_canvas)

        normal_texts = [["", 0, 0]]
        big_texts    = [["", 0, 0]]

        readable, _, _ = select.select([MySock], [], [], 0)
        if readable:
            data, addr = receive_data()
            if data:
                data = data.decode().strip()
                #print(f"Received from {addr}: {data}")

                if role == cat_name:
                    if data in "사냥구역이 입니다. 주변 8칸을 사냥합니다(5초 간 사냥)":
                        penalty = False
                        skill = True
                    elif data in "페널티(5초 정지)":
                        penalty = True
                        skill = True
                        penalty_time_start =  time.time()
                    elif data in "을 사냥했습니다.":
                        print("사냥했습니다")
                        penalty = False
                        skill = True
                    elif data in "game_over" or "game_over" in data:
                        skill = True
                        game_over = True

                if role == rat_name:
                    print("rat_recv")
                    if data in "[현재색]" or "[현재색]" in data:
                        color = data[5:]
                        print(f"현재 color: {color}")
                    if data in "goal" or "goal" in data:
                        goal = data[21:]
                        skill = True
                        print(f"현재 goal: {goal}")
                    if data in "없습니다." or "없습니다."in data:
                        print(f"없습니다.")
                        skill = True
                    if data in "game_over" or "game_over" in data:
                        skill = True
                        game_over = True
        '''
        if message == "enter":
            skill = True
        else:
            skill = False'''
        if skill == True:
            skill_to_kor = "사용 성공"
        else:
            skill_to_kor = "사용 해제"

        if game_over:
            game_over_kor = "종료"
        else:
            game_over_kor = "진행중"

        if role == rat_name:
            normal_texts = [[f"역할: {role}", 10, 10],
                            [f"목표: {goal} 구역 중심에서 스킬로 탈출", 10, 50],
                            [f"구역: {color}", 10, 90],
                            #[f"스킬: {skill_to_kor}", 10, 130],
                            #[f"상태: {game_over_kor}", 10, 170]
                            ]
        elif role == cat_name:
            goal = rat_name
            big_texts = [[f"", target_window_size[0]//2-300, target_window_size[1]//2-80]]


            if penalty == True:
                penalty_iterval = int(time.time() - penalty_time_start)
                if penalty_iterval <= 5:
                    big_texts = [[f"  스킬 사용 실패\n5초간 강제 정지", target_window_size[0]//2-300, target_window_size[1]//2-80]]
                else:
                    penalty = False
            else:
                big_texts = [[f"", target_window_size[0]//2-300, target_window_size[1]//2-80]]
                penalty_iterval = 0

            normal_texts = [[f"역할: {role}", 10, 10],
                            [f"목표: {goal}와 같은 구역 중심에서 스킬로 사냥 ", 10, 50],
                            [f"스킬: {skill_to_kor}", 10, 90],
                            [f"스킬 쿨타임: {penalty_iterval}/5초 남음", 10, 130],
                            ]
            normal_texts = [[f"역할: {role}", 10, 10],
                            [f"목표: {goal}와 같은 구역 중심에서 스킬로 사냥 ", 10, 50],
                            [f"스킬: {skill_to_kor}", 10, 90],
                            ]
            

        ####
        if game_over == True:
            big_texts = [[f"게임오버", target_window_size[0]//2-300, target_window_size[1]//2-80]]
        else:
            big_texts = [[f"", target_window_size[0]//2-300, target_window_size[1]//2-80]]
                
        
        for text in normal_texts:
            draw = ImageDraw.Draw(pil_img)
            draw.text((text[1], text[2]), text[0], font=normal_font, fill=(250, 220, 100))
        for text in big_texts:
            draw = ImageDraw.Draw(pil_img)
            draw.text((text[1], text[2]), text[0], font=big_font, fill=(20,0,150))

        ###
        img_canvas = np.array(pil_img)#cv2.cvtColor(np.array(pil_img), cv2.COLOR_BGR2RGB)
        cv2.imshow(win_name, img_canvas)
        cv2.waitKey(1)

        if keyboard.is_pressed('esc'):
            break

    cleanup()
