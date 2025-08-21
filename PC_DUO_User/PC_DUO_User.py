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
myIp = socket.gethostbyname(socket.gethostname())  # 모든 네트워크 인터페이스에서 수신
# ESPcam 영상 스트리밍 경로 설정



ratName = "러너"
catName = "헌터"
role = catName



'''
#빨간자동차(러너)
esp32camIp = "192.168.0.15"
pc1Port = 8000            # PC1이 키 입력을 받을 포트
myPort = 8001       # PC1에서 데이터를 받을 포트
#파란 자동차(헌터)
'''
esp32camIp = "192.168.0.15"
pc1Port = 5006            # PC1이 키 입력을 받을 포트
myPort = 5006       # PC1에서 데이터를 받을 포트
#'''

esp32camPort = 81
vid_path = f"http://{esp32camIp}:{esp32camPort}/stream"
# PC1 통신 설정
pc1Ip = "192.168.0.7"

mySock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
mySock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536) # 수신 버퍼를 64KB로 설정
mySock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536) # 송신 버퍼를 64KB로 설정
mySock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 포트 재사용 허용
mySock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1) # 브로드캐스트 허용
mySock.settimeout(0.0)
mySock.bind(("", myPort))  # 데이터 수신 소켓

#---이미지 및 창 관련---
# 이미지 및 창 크기
widthHeightRatio = 1.33
windowImgRatio = 0.85
targetWindowSize = np.array([1280, 720], dtype = np.uint16)
targetImgSize = np.array([int(targetWindowSize[1]*windowImgRatio*widthHeightRatio), 
                            int(targetWindowSize[1]*windowImgRatio)], dtype = np.uint16)
# 윈도우 세팅
winName = " "
cv2.namedWindow(winName, cv2.WINDOW_AUTOSIZE)
cv2.resizeWindow(winName, targetImgSize[0], targetImgSize[1])
# 이미지 불러오기
eTime = time.time()

img = np.empty((targetImgSize[1], targetImgSize[0], 3), dtype = np.uint8)
imgCanvas = np.empty((targetWindowSize[1], targetWindowSize[0], 3), dtype = np.uint8)

# ---UI 오버레이 관련---
# UI 색상
ovColor = (220, 200, 20)
# 화살표
scale = 3
targetPosition = np.array([115, 600], dtype = np.int16)
arrows = np.array([ 
    [[0, 0], [0, 0], [0, 0]],
    [[40, 20], [30, 37], [50, 37]],
    [[40, 57], [30, 40], [50, 40]],
    [[5, 40],  [20, 50], [20, 30]], 
    [[77, 40], [60, 50], [60, 30]] ], dtype = np.int32)
arrows[0] = np.mean(arrows[1:5].reshape(-1, 2), axis=0).astype(np.int32)
arrows = (arrows + arrows[0]) * scale - arrows[0]
arrows = arrows + (targetPosition - arrows[0, 0])

#---키입력 관련---
# 키 입력 상태
#'''
#'''
keyMap = {
            'up': ('w', 'down'),
            'down': ('s', 'up'),
            'left': ('a', 'right'),
            'right': ('d', 'left'),
            'space': ('space', 'enter'),
            'enter': ('enter', 'space')
        }
'''
keyMap = {
            'w': (0, 's'),
            's': (1, 'w'),
            'a': (2, 'd'),
            'd': (3, 'a'),
            'space': (4, 'enter'),
            'enter': (5, 'space')
        }

'''
#'''
keyMap = {
            'up': (0, 'down', 'w'),
            'down': (1, 'up', 's'),
            'left': (2, 'right', 'a'),
            'right': (3, 'left', 'd'),
            'space': (4, 'enter', 'space'),
            'enter': (5, 'space', 'enter')
        }
#'''
currentKeys = np.array([False] * len(keyMap), np.int8)
prevKeys    = np.array([False] * len(keyMap), np.int8)  # 이전 키 상태 저장

# 데이터 수신 함수
def receiveData(bufferSize = 1024):
    try:
        receivedBytes, address = mySock.recvfrom(bufferSize)
        return receivedBytes, address
    except BlockingIOError:
        return None, None
    except UnicodeDecodeError:
        print("Error: Received data could not be decoded.")  # 디코딩 오류 방지
        return None, None
    except Exception as e:
        print(f"receive error mySock - {e}")
        return None, None

lastSent = time.time()
interval = 0.25
autoSendCount = 0
autoSendMax = 1 
message = 'space'
currentMessage = 'space'
def autoSend():
    global lastSent
    global message
    global autoSendCount
    global autoSendMax
    global currentMessage
    if autoSendMax >= autoSendCount and message != "":
        if time.time() - lastSent > interval:
            udpSend()
            lastSent = time.time()
            autoSendCount += 1
    if message != currentMessage:
        autoSendCount = 0
        currentMessage = message

def udpSend():
    global message
    try:
        mySock.sendto(message.encode(), (pc1Ip, pc1Port))   # 입력된 키에 해당하는 인덱스를 전달         
        #print("send to PC1: ",message.encode())
    except OSError as e:
        print(f"PC1 Send Error: {e} (try to send: {message.encode()})")

# 키 입력 감지 및 전송 (중복 전송 방지)
def sendKeyInput(event):
    global message
    key = event.name

    if key in keyMap:
        idx, opposite, message = keyMap[key]
        if event.eventType == 'down' and not currentKeys[idx]:
            currentKeys[idx] = True
            currentKeys[keyMap[opposite][0]] = False
            #message = str(key) # 입력된 키를 전달
            message = str(message) # 입력된 키에 해당하는 인덱스를 전달
            #message = str(currentKeys[:4])
            udpSend()
        elif event.eventType == 'up' and currentKeys[idx]:
            currentKeys[idx] = False
            #message = str(key) # 입력된 키를 전달
            message = str(message) # 입력된 키에 해당하는 인덱스를 전달
            #message = str(currentKeys[:4])
            udpSend()













def hand_shake_with_operator(MAX_RETRIES=1024):
    for _ in range(MAX_RETRIES):
        try: # 현재 처리 중인 디바이스의 소켓인지 확인
                data, addr = mySock.recvfrom(1024)
                msg = data.decode().strip()
                if msg:
                    print(f"INFO: 연산컴퓨터의 데이터를 수신 성공 ({msg})")
                    mySock.sendto(msg.encode(), (pc1Ip, pc1Port))
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
    mySock.close()
    cv2.destroyAllWindows()
    print("shut down properly. [^-^]/")

# 메인 앱
if __name__ == "__main__":
    print("\n\n\n\n")
    print("------------------------------------------------------------------------------------------")
    print(f"DEBUG: 프로그램 시작:            {datetime.now()}")
    print(f"DEBUG: 나의 소켓 정보:           {myIp}:{myPort}")
    print(f"DEBUG: 나의 코드 위치:           {os.path.abspath(__file__)}") 
    print("------------------------------------------------------------------------------------------")
    print(f"DEBUG: 정상적으로 연결되지 않을 경우, 네트워크 설정을 확인하십시오.(공용 -> 개인 네트워크)")
    print("------------------------------------------------------------------------------------------")
    print("")

    keyboard.hook(sendKeyInput)
    #연산컴퓨터와 연결
    # while 1:
    #     try:
    #         data, _ = mySock.recvfrom(1024)
    #         message = data.decode().strip()
    #         if message:
    #             print(f"Received data from PC1: {message}")
    #             if "success" in message:
    #                 print("연산컴퓨터와 연결 성공")
    #                 break
    #             mySock.sendto(message.encode(), (pc1Ip, pc1Port))
    #     except BlockingIOError:
    #         pass  # 데이터가 없으면 넘어감
    #     except UnicodeDecodeError:
    #         print("Error: Received data could not be decoded.")  # 디코딩 오류 방지

    print(f"myIp: {mySock.getsockname()}")

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
        #autoSend()
        #영상 읽어오기
        '''
        frame, img = cap.read()
        #읽어오는 동영상 위치가 잘못되었으면 종료
        if not cap.isOpened():
            img = np.empty((targetImgSize[0], targetImgSize[1], 3), dtype=np.uint8)
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
            img = np.empty((targetImgSize[1], targetImgSize[0], 3), dtype=np.uint8)
            #print("Received an empty image frame. ['-']>") # img가 None인 경우
        '''
        img = np.empty((targetImgSize[1], targetImgSize[0], 3), dtype=np.uint8) #지워야함

        # 이미지 크기 재지정
        img = cv2.resize(img, dsize=(targetImgSize[0], targetImgSize[1]), interpolation=cv2.INTER_AREA)
        img = cv2.flip(cv2.flip(img, 1), 0)
        # img를 imgCanvas 중앙에 배치
        imgCanvas[:] = 0
        img_size = img.shape[:2][::-1]
        img_offset = np.array(((targetWindowSize[0] - img.shape[1]) // 2, (targetWindowSize[1] - img.shape[0]) // 2), dtype = np.uint16)
        imgCanvas[img_offset[1]:img_offset[1]+img_size[1], img_offset[0]:img_offset[0]+img_size[0], :] = img

        ##
        for i, arrow in enumerate(arrows[1:]):
            imgCanvas = cv2.polylines(imgCanvas, [arrow], True, (250, 220, 100), 3)
            if currentKeys[i]:
                imgCanvas = cv2.fillPoly(imgCanvas, [arrow], (250, 220, 100), cv2.LINE_AA)

        pil_img = Image.fromarray(imgCanvas)

        normal_texts = [["", 0, 0]]
        big_texts    = [["", 0, 0]]

        readable, _, _ = select.select([mySock], [], [], 0)
        if readable:
            data, addr = receiveData()
            if data:
                data = data.decode().strip()
                #print(f"Received from {addr}: {data}")

                if role == catName:
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

                if role == ratName:
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

        if role == ratName:
            normal_texts = [[f"역할: {role}", 10, 10],
                            [f"목표: {goal} 구역 중심에서 스킬로 탈출", 10, 50],
                            [f"구역: {color}", 10, 90],
                            #[f"스킬: {skill_to_kor}", 10, 130],
                            #[f"상태: {game_over_kor}", 10, 170]
                            ]
        elif role == catName:
            goal = ratName
            big_texts = [[f"", targetWindowSize[0]//2-300, targetWindowSize[1]//2-80]]


            if penalty == True:
                penalty_iterval = int(time.time() - penalty_time_start)
                if penalty_iterval <= 5:
                    big_texts = [[f"  스킬 사용 실패\n5초간 강제 정지", targetWindowSize[0]//2-300, targetWindowSize[1]//2-80]]
                else:
                    penalty = False
            else:
                big_texts = [[f"", targetWindowSize[0]//2-300, targetWindowSize[1]//2-80]]
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
            big_texts = [[f"게임오버", targetWindowSize[0]//2-300, targetWindowSize[1]//2-80]]
        else:
            big_texts = [[f"", targetWindowSize[0]//2-300, targetWindowSize[1]//2-80]]
                
        
        for text in normal_texts:
            draw = ImageDraw.Draw(pil_img)
            draw.text((text[1], text[2]), text[0], font=normal_font, fill=(250, 220, 100))
        for text in big_texts:
            draw = ImageDraw.Draw(pil_img)
            draw.text((text[1], text[2]), text[0], font=big_font, fill=(20,0,150))

        ###
        imgCanvas = np.array(pil_img)#cv2.cvtColor(np.array(pil_img), cv2.COLOR_BGR2RGB)
        cv2.imshow(winName, imgCanvas)
        cv2.waitKey(1)

        if keyboard.is_pressed('esc'):
            break

    cleanup()
