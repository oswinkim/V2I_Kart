#불러오는 모듈
import socket
import select

import os

import random as r

import json
import csv

import time
ratsLife = 0
ratList = []
# 클래스

class Common:
    num = 0

    def __init__(self, ip = "", sendPort = "", revPort = ""):
        # print("[소캣설정 시작]")

        Common.num += 1

        self.Priority = Common.num
        # print(f"우선순위: {self.Priority}")

        if not ip:
            ip = input("ip가 설정되지 않았습니다.ex) 0.0.0.0: ")
        self.ip = ip
        # print(f"ip: {self.ip}")

        if not sendPort:
            sendPort = input("송신포트가 설정되지 않았습니다.ex) 5000: ")
        self.sendPort = int(sendPort)
        # print(f"송신포트: {self.send_Port}")

        if not revPort:
            revPort = input("수신포트가 설정되지 않았습니다.ex) 5001: ")
        self.revPort = int(revPort)
        # print(f"수신포트: {self.rev_Port}\n")


        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(("", self.sendPort))

        WorldSockets.append(self.socket)


class Player(Common):
    def __init__(self, Name = "", ip = "", sendPort = "", revPort = ""):
        # print("[Player 설정시작]")
        self.Type = "Player"
        # print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("플레이어의 이름이 설정되지 않았습니다.ex) 다오: ")
        self.Name=Name
        # print(f"플레이어 이름: {self.Name}")

        super().__init__(ip, sendPort, revPort)

class Kart(Common):
    def __init__(self, Name = "", ip = "", sendPort = "", revPort = ""):
        # print("[Kart 설정시작]")
        self.Type = "Kart"
        # print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("카트의 이름이 설정되지 않았습니다.ex) 코튼: ")
        self.Name = Name
        # print(f"카트 이름: {self.Name}")
        
        self.ahrsStart = 0
        self.ahrs = 0

        self.color = "sky"
        self.colorName = "sky"

        self.leftMotor = 0
        self.rightMotor = 0

        super().__init__(ip, sendPort, revPort)

class Infra(Common):
    def __init__(self, Name="", ip="", sendPort="", revPort=""):
        # print("[Infra 작성시작]")
        self.Type = "Infra"
        # print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("인프라의 이름이 설정되지 않았습니다.ex) 신호등1: ")
        self.Name = Name
        # print(f"인프라 이름: {self.Name}")

        super().__init__(ip, sendPort, revPort)

class User:
    def __init__(self,
                 userName = "",
                 nameKart = "", ipKart = "", sendPortKart = "", revPortKart = "",
                 namePlayer = "", ipPlayer = "", sendPortPlayer = "", revPortPlayer = "",
                 role = "jobless", goal = "none"):
        # print("-----------------------------")
        # print("[User 설정시작]")
        self.Type = "User"
        self.key = "space"
        # print(f"객체 타입: {self.Type}")

        if not userName:
            userName = input("유저 이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.userName = userName
        self.role = role
        self.goal = goal
        self.life = 1
        # print(f"유저 이름: {self.userName}\n")

        self.Kart = Kart(nameKart, ipKart, sendPortKart, revPortKart)
        self.Player = Player(namePlayer, ipPlayer, sendPortPlayer, revPortPlayer)

        self.drivingRecord = [ [f'userName = {self.userName}', 
                                 f'nameKart = {self.Kart.Name}', 
                                 f'ipKart = {self.Kart.ip}', 
                                 f'sendPortKart = {self.Kart.sendPort}', 
                                 f'revPortKart = {self.Kart.revPort}', 
                                 f'namePlayer = {self.Player.Name}', 
                                 f'ipPlayer = {self.Player.ip}', 
                                 f'sendPortPlayer = {self.Player.sendPort}', 
                                 f'revPortPlayer = {self.Player.revPort}', 
                                 ], 
                                ["현재구간", 
                                 "최초 연결시간", 
                                 "현재시간", 
                                 "왼쪽 모터상태", 
                                 "오른쪽 모터상태", 
                                 "방향변환값",
                                 "변환된 컬러값",  
                                 "LUX", 
                                 "컬러R", "컬러G", "컬러B", 
                                 "raw방향값"]]
        
        self.trace = []


        print(f"<유저[{userName}] 기본 설정 완료>")
        # print("-----------------------------")

# 함수

def send(target, msg):
    target.socket.sendto(msg.encode(), (target.ip, target.revPort))
    if "[name]" not in msg:
        print(f"{target.Name}에게 {msg}를 보냈습니다.")

def sends(t:list, msg):
    for target in t:
        target.socket.sendto(msg.encode(), (target.ip, target.revPort))
        print(f"{target.Name}에게 {msg}를 보냈습니다.")

def get_Unique_Filename(directory, baseName, extension=".csv"):
    # 디렉토리 존재 여부 확인 및 생성
    os.makedirs(directory, existOk=True)

    filename = f"{baseName}{extension}"
    fullPath = os.path.join(directory, filename)

    counter = 1
    while os.path.exists(fullPath):
        filename = f"{baseName}_{counter}{extension}"
        fullPath = os.path.join(directory, filename)
        counter += 1

    return fullPath

def csvFileSave(MACRON:list):
    for i in range(len(MACRON)):
        if (MACRON[i].Type == "User"):
            qual = 0
            while 1:
                qual = input(f"{MACRON[i].userName}의 데이터 수집 결과 결정 (GOOD:2, NOMAL:1, BAD:0)\n입력: ")
                try:
                    qual = int(qual)
                    if 0 <= qual <= 2:
                        final = input(f"{qual}을 선택하신게 맞다면 1을 아니라면 0을 눌러주세요.\n입력:")
                        print(f"{final}을 입력하셨습니다.")
                        break
                    else:
                        print(f"{qual}을 입력하셨습니다.\n 0,1,2 중에 입력해주세요")
                except:
                    print(f"입력하신 자료형은 정수가 아닙니다.({type(qual)}형으로 입력됨)")

            if qual == 0:
                qual = "[bad]"
            elif qual == 1:
                qual = "[nomal]"
            else:
                qual = "[good]"
            
            filePath = "c:\\Users\\PC-16\\Desktop\\V2I_Kart\\PC_UNO_Operator\\data\\" 
            baseName = qual + MACRON[i].userName 
            fileName = get_Unique_Filename(filePath,baseName)


            with open(fileName, 'w', newLine = '', encoding = 'utf-8') as f:
                writer = csv.writer(f)
                # writer.writerow([f'userName = {MACRON[i].userName}', 
                #                  f'nameKart = {MACRON[i].Kart.Name}', 
                #                  f'ipKart = {MACRON[i].Kart.ip}', 
                #                  f'sendPortKart = {MACRON[i].Kart.sendPort}', 
                #                  f'revPortKart = {MACRON[i].Kart.revPort}', 
                #                  f'namePlayer = {MACRON[i].Player.Name}', 
                #                  f'ipPlayer = {MACRON[i].Player.ip}', 
                #                  f'sendPortPlayer = {MACRON[i].Player.sendPort}', 
                #                  f'revPortPlayer = {MACRON[i].Player.revPort}', 
                #                  ])
                # writer.writerow([]) 
                for row in MACRON[i].drivingRecord:
                    writer.writerow(row)  # 각 행 쓰기
                print(f"저장된 데이터 파일명: {fileName}")

def connecting(M:list):
    # 난수 발생
    num = str(r.randrange(10,100))
    
    for i in range(len(M)):
        if (M[i].Type == "User"):
            print(f"[{M[i].userName}과 연결 시작]")

            # 플레이어 연결
            print(f"\n플레이어[{M[i].Player.Name}]와 연결중...")
            while 1:
                # 난수 전송
                send(M[i].Player, num)

                readable, _, _ = select.select(WorldSockets, [], [], 1)

                if not readable:
                    print("재전송")
                else:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()

                    if sock == M[i].Player.socket:
                        if num in msg:
                            print("연결성공!")
                            send(M[i].Player, "success")
                            break
                        else:
                            print("ERROR:요청하지 않은 메시지")
                            print(f"msg:{msg}")

            # 카트 연결
            print(f"\n카트[{M[i].Kart.Name}]와 연결중...")
            while 1:
                # 난수 전송
                send(M[i].Kart, num)

                readable, _, _ = select.select(WorldSockets, [], [], 1)

                if not readable:
                    print("재전송")

                else:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()

                    
                    if sock == M[i].Kart.socket:
                        if num in msg:
                            print("연결성공!")
                            send(M[i].Kart, "success")
                            break
                        else:
                            print("ERROR:요청하지 않은 메시지")
                            print(f"msg:{msg}")

            # #AHRS 정상 작동 확인
            # print(f"\n카트[{M[i].Kart.Name}]의 AHRS센서 정상 작동 확인중...")
            # while 1:
            #     send(M[i].Kart, "ahrs")

            #     readable, _, _ = select.select(WorldSockets, [], [], 1)
                
            #     if not readable:
            #         print("패스")
            #     else:
            #         for sock in readable:
            #             data, addr = sock.recvfrom(1024)  # 데이터 수신
            #             msg = data.decode().strip()

            #         if sock == M[i].Kart.socket:
            #             if "[ahrs]" in msg:
            #                 msg = msg[6:]
            #                 try:
            #                     msg = float(msg)
            #                     if abs(msg) > 0:
            #                         M[i].Kart.ahrsStart = msg
            #                         print("AHRS 정상 작동")
            #                         print(f"작동값:{msg}")
            #                         break
            #                     else:
            #                         print("ERROR:AHRS 오류")
            #                         print(f"작동값:{msg}")
            #                 except:
            #                     print(f"ERROR:메시지 형식 오류")
            #                     print(f"msg:{msg}")
            #                     print(f"type:{type(msg)}")

            #             elif num not in msg:
            #                 print("ERROR:요청하지 않은 메시지")
            #                 print(f"msg:{msg}")

            # #color센서 정상 작동 확인
            # print(f"\n카트[{M[i].Kart.Name}]의 Color센서 정상 작동 확인중...")
            # while 1:
            #     send(M[i].Kart, "Color")
                
            #     readable, _, _ = select.select(WorldSockets, [], [], 1)
            #     if not readable:
            #         print("패스")
            #     else:
            #         for sock in readable:
            #             data, addr = sock.recvfrom(1024)  # 데이터 수신
            #             msg = data.decode().strip()

            #         if sock == M[i].Kart.socket:
            #             if "[Color]" in msg:
            #                 msg = msg[7:]

            #                 lux, msgR, msgG, msgB = msg.split(',')
            #                 if abs(int(lux)) > 0 and abs(int(msgR)) > 0 and abs(int(msgG)) > 0 and abs(int(msgB)) > 0:
            #                     M[i].Kart.color = msg
            #                     print("Color 정상 작동")
            #                     print(f"작동값:{msg}")
            #                     # print(f"현재색깔:{colorDefine(lux,msgR,msgG,msgB,colorTunning, "None")}")
            #                     break
            #                 else:
            #                     print("ERROR:Color 오류")
            #                     print(f"작동값:{msg}")

            #             elif num not in msg:
            #                 print("ERROR:요청하지 않은 메시지")
            #                 print(f"msg:{msg}")

            print(f"[{M[i].userName}과 연결 완료]\n")


def Player2Kart(U, msg):
    global ratsLife
    global gameState
    # if (U.role == "rat" and "enter" in msg):
    #     send(U.Player, "skill activative")
    #     send(U.Kart, "stop")
        
    # else:
    if msg in keysMove:
        # if U.key == "w" and msg == "s":
        #     send(U.Kart, "space")
        #     U.key = msg
        # elif U.key == "s" and msg == "w":
        #     send(U.Kart, "space")
        #     U.key = msg
        # else:    
            U.Kart.socket.sendto(msg.encode(), (U.Kart.ip, U.Kart.rev_Port)) # move
            print(f"Sent to [{U.Kart.Name}]ESP: {msg}")
            U.key = msg
        

    elif msg == "space":
        U.Kart.socket.sendto("i".encode(), (U.Kart.ip, U.Kart.rev_Port))  # motor OFF
        print(f"Sent to [{U.Kart.Name}]ESP: motor OFF")

    elif "enter" in msg:
        if U.role == "cat":
            print(f"카트색:{U.Kart.colorName}")
            print(f"카트색(딕셔너리 넘버):{colorRelation[U.Kart.colorName]}")
            numm = colorRelation[U.Kart.colorName]
            if (numm%2 == 0):
                send(U.Kart, "i")
                send(U.Player, "사냥구역이 입니다. 주변 8칸을 사냥합니다")

                for j in ratList:
                    print(type(j))
                    if(colorRelation[U.Kart.colorName]-colorRelation[j.Kart.colorName]== 1):
                        send(U.Player,f"{j.userName}을 사냥했습니다.")
                        # send(U.Kart, "[die]")
                        send(U.Kart, "i")
                        send(j.Kart, "i")
                        # rats_Life -= 1
                        j.life = 0
            else:
                send(U.Kart, "i")
                send(U.Player, "사냥구역이 아닙니다. 패널티")
        else: # rat
            if (colorRelation[U.Kart.colorName]%2 == 0):
                if(U.goal == U.Kart.colorName):
                    send(U.Kart, "i")
                    # send(U.Kart, "[die]")
                    # rats_Life -= 1
                    U.life = 0
                    print("게임오버, 생명: ", ratsLife)
                else:
                    send(U.Player,f"[현재색]{U.Kart.colorName}")
            else:
                send(U.Player, "해당 구간에서는 목적지를 확인할 수 없습니다.")
                
    # elif msg == "m":
    #     U.Kart.socket.sendto("ahrs".encode(), (U.Kart.ip, U.Kart.rev_Port))  # ahrs값
    #     print("ahrs값을 요청하는중...")
    # elif msg == "c":
    #     U.Kart.socket.sendto("Color".encode(), (U.Kart.ip, U.Kart.rev_Port))  # ahrs값
    #     print("color값을 요청하는중...")

    else:
        print(f"undefine key value: {msg}")

"""
def colorDefine(lux, r, g, b, tuning:list, U):
    raw = [lux,r,g,b]
    print(f"raw color value:{raw}")
    deviation = []

    try:
        if U.Type == "User":
            Dao.Kart.colorR = int(r)
            Dao.Kart.colorG = int(g)
            Dao.Kart.colorB = int(b)

        else:
            print("객체의 타입이 잘못되었습니다.(User를 위한 함수)")
    except:
        print("객체없음")

    for i in range(len(tuning)):
        temp = 0
        for j in range(4):
            temp += (int(tuning[i][j+1])-int(raw[j]))**2
        deviation.append(temp)

    for i in range(len(deviation)):
        if min(deviation) == deviation[i]:
            print(f"컬러 편차제곱합의 최소: {min(deviation)}")
            #출력결과
            return tuning[i][0]
"""
            
def Kart2Player(U,msg):
    #     if "[ahrs]" in msg:
    #         msg = msg[6:]
    #         U.Player.socket.sendto(msg.encode(), (U.Player.ip, U.Player.revPort))  
    # #        print(f"Sent ahrs Value to PC2: {msg}")
    #         U.Kart.ahrs = msg
    #     elif "[color]" in msg:
    #         msg=msg[7:]
    #         U.Player.socket.sendto(msg.encode(), (U.Player.ip, U.Player.revPort))  
    # #        print(f"Sent color Value to PC2: {msg}")
    #         Dao.Kart = msg
    # print(f"2Player:{msg}")

    if msg in colorAll:
        # msg = msg[8:]
        # histo = msg.split('|')
        # #msg = "0|1234|red|..."
        # #원상형태["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"]
        # #최종형태["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"]
        # #같음
        # U.driving_record.append(histo)

        # print(f"{U.userName}의 color:{msg}")
        U.Kart.colorName = msg
        print(f"{U.userName}의 color:{msg}")
        # if(U.role=="rat"):
        send(U.Player, f"[현재색]{U.Kart.colorName}")

colorAll = ["mmdf", "red", "blue", "green", "pink", "orange", "sky", "white"]
colorRelation = {"mdf" : 7, "red" : 2, "blue" : 6, "green" : 8, "pink" : 3, "orange" : 4, "sky" : 5, "white" : 1}
def colorAdjust(U):
    colorAll = ["mmdf", "red", "blue", "green", "pink", "orange", "sky", "white"]
    try:
        if(U.Type == "User"):
            pass
    except:
        print("wrong type in colorAdjust")
        return

    #6색상
    storedColors = []

    # colorAdjust상태로 연결시도
    print("esp32를 컬러보정 상태로 변경중...")
    while 1:
        send(U.Kart, "[colorAdjust]")
        readable, _, _ = select.select(WorldSockets, [], [], 1)

        if not readable:
            print("재전송")
        else:
            for sock in readable:
                data, addr = sock.recvfrom(1024)  # 데이터 수신
                msg = data.decode().strip()

            if sock == U.Kart.socket:
                if "[colorAdjust]" in msg:
                    send(U.Kart, "success")
                    print("ready to adjust")
                    break
                else:
                    print("ERROR:요청하지 않은 메시지")
                    print(f"msg:{msg}")

    for p in range(8):
        print(f"{colorAll[p]}색상을 보정합니다(진행도: {(p+1)}/{len(colorAll)}).")
        while 1:
            
            send(U.Kart, "color=" + colorAll[p])
            readable, _, _ = select.select(WorldSockets, [], [], 1)

            if not readable:
                print(f"{colorAll[p]}색상값 재요청, p:{p}")
            else:
                for sock in readable:
                    data, addr = sock.recvfrom(1024)  # 데이터 수신
                    msg = data.decode()
                    
                if (sock == U.Kart.socket):
                    if "[raw_color]" in msg:
                        print(f"원상데이터:{msg}")
                        msg = msg[12:]
                        msg = msg.split("|")

                        for i in range(5):
                            print(f"{i+1}: Name = {msg[5*i]}, Lux = {msg[5*i+1]}, R = {msg[5*i+2]}, G = {msg[5*i+3]}, B={msg[5*i+4]}")
                        
                        print(f"Average: Name = {msg[5*5]}, Lux = {msg[5*5+1]}, R = {msg[5*5+2]}, G = {msg[5*5+3]}, B = {msg[5*5+4]}")

                        if (colorAll[p] in msg):
                        # 테스트용 주석
                            decision =input("Accept this measurement? (y/n): ")
                        else:
                            decision = "false"

                        if decision.lower() == 'y':
                            print(f"저장값: {[msg[5*5], msg[5*5+1], msg[5*5+2], msg[5*5+3], msg[5*5+4]]}")
                            storedColors.append([msg[5*5], msg[5*5+1], msg[5*5+2], msg[5*5+3], msg[5*5+4]])
                            break
                        else:
                            print("현재 측정값 거부 재측정시작")

                    else:
                        print("ERROR:요청하지 않은 메시지")
                        print(f"msg:{msg}")

        print(f"{colorAll}색상 보정 완료")
        colorAll = ["mdf", "red", "blue", "green", "pink", "orange", "sky", "white"]
    print("수집완료")

    msg="colorData"
    for i in range(len(storedColors)):
        msg += f"|{storedColors[i][0]}|{storedColors[i][1]}|{storedColors[i][2]}|{storedColors[i][3]}|{storedColors[i][4]}"
    print(f"형식:{type(storedColors[0][1])}")
    sendmsg = msg

    while 1:
        send(U.Kart, sendmsg)

        readable, _, _ = select.select(WorldSockets, [], [], 1)

        if not readable:
            print("재전송")
        else:
            for sock in readable:
                data, addr = sock.recvfrom(1024)  # 데이터 수신
                msg = data.decode().strip()

            if sock == U.Kart.socket:
                if "[save]" in msg:
                    # print(msg)
                    print("success to save color")
                    break
                else:
                    print("ERROR:요청하지 않은 메시지")
                    print(f"msg:{msg}")

def goalSet(U):
    
    color = colorAll[r.choice([1 ,2, 3, 5])]
    goal = f"[goal]{U.userName}의 목표 위치의 색깔은 {color}"
    send(U.Player, goal)

    U.goal = color
    print(f"{U.userName}의 목표색은 [{color}]로 설정되었습니다.")
    

def game_play():
    global gameState
    global ratsLife
    if gameState != 0:
        return
    print(f"[상태|{gameState}]")
    for i in userList:
        send(i.Player,"gameOver")
        if (i.goal != "none"):
            print(f"[이름|{i.userName}]\n역할: {i.role}\n현재색상: {i.Kart.colorName}\n목표 색상: {i.goal}")
        else:
            print(f"[이름|{i.userName}]\n역할: {i.role}\n현재색상: {i.Kart.colorName}")
    while 1:
        decision=input("게임종료를 인정하시겠습니까?(y/n): ")
        if decision.lower() == 'y':
            for i in userList:
                send(i.Player,"gameOver")
                send(i.Kart, "i")
            print("게임종료")
            gameState = -1
            return
        else:
            print(f"[{decision}]키를 눌렀습니다.\n게임을 계속 진행합니다.")
            for i in userList:
                if (i.goal != "none"):
                    life=input(f"\n[이름|{i.userName}][목숨:{i.life}]을 살리시겠습니까?(y/n)")
                    if life.lower() == 'y':
                        i.life = 1
                        gameState = 1
                        print(f"[이름|{i.userName}][목숨:{i.life}]\n")
                    else:
                        i.life = 0
                        print(f"[이름|{i.userName}][목숨:{i.life}]\n")
            gameState = 1
            print(f"[게임상태:{gameState}]\n 게임재게")
            return
# 실행 시 변경해야 할 부분

WorldSockets = []
keysMove = ["w","a","s","d"]

gameState = 1


"""
#함수 자동화 필요(컬러센서 마다 편차 존재)
colorTunning=[["white", 920, 2500, 1700, 1300], 
               ["red", 65200, 1700, 300, 250],
               ["pink", 50, 2200, 800, 750],
               ["orange", 65300, 2200, 500, 350],
               ["green", 750, 700, 750, 330],
               ["blue", 550, 600, 900, 900],
               ["sky", 850, 1300, 1300, 1100]]
"""
               
Dao = User(
        userName = "파랑",
        nameKart = "파랑색카트", ipKart = "192.168.0.14", sendPortKart = "4213", revPortKart = "4212",
        namePlayer = "다오", ipPlayer = "192.168.0.12", sendPortPlayer = "5006", revPortPlayer = "5006", 
        role = "cat"
    )

Bazzi = User(
        userName = "빨강",
        nameKart = "빨강색카트", ipKart = "192.168.0.13", sendPortKart = "7000", revPortKart = "7001",
        namePlayer = "배찌", ipPlayer = "192.168.0.17", sendPortPlayer = "8000", revPortPlayer = "8001",
        role="rat"
    )
    
# d = User(
#         userName = "testuser",
#         nameKart = "예비카트", ipKart = "192.168.0.14", sendPortKart = "4213", revPortKart = "4212",
#         namePlayer = "다오예비", ipPlayer = "192.168.0.17", sendPortPlayer = "8000", revPortPlayer = "8001",
#         role = "cat"
#     )
colorTime = {}

# 통신하는 모든 객체들
# macron = [Dao]
macron = [Dao, Bazzi]

# 동작부분

# user분리
userList = []
for i in macron:
    if i.Type == "User":
        userList.append(i)

print("*정상적으로 연결되지 않을 경우 네트 워크 설정을 확인하십시오.(공용 -> 개인 네트워크)")
print("Waiting for key input from Player and data from Kart...")

# connecting(macron)


# for i in userList:
#     if (i.role == "rat"):
#         goalSet(i)
#         ratList.append(i)


a = 0
while 1:
    if a == "1":
        break
    for i in userList:
        if (i.role == "rat"):
            goalSet(i)
            ratList.append(i)
    for j in ratList:
        print(f"{j.userName}의 목표:{j.goal}")
    print("이게 맞습니까?")
    a = (input(f"맞다면 1 아니라면 0: "))
    
    
# 0719 

# for i in userList:
#     print("user의 Kart 색상 보정")
#     colorAdjust(i)
colorAll = ["mdf", "red", "blue", "green", "pink", "orange", "sky", "white"]

print("[모든 설정이 완료되었습니다]\n[게임을 시작합니다!]")
while True:
    try:
        ratsLife = 0
        for i in ratList:
            ratsLife += i.life

        if(ratsLife == 0):
            gameState = 0
        if(ratsLife > 0):
            gameState = 1
        if (ratsLife < 0):
            gameState == -1
            print(f"게임값:{gameState}")

        game_play()            
        if(gameState == -1):
            break

        # now = time.time()
        # for obj in userList:
        #     name = obj.Kart.Name
        #     if name not in colorTime or now - colorTime[name]>1:
        #         send(obj.Kart, "[name]")
        #         colorTime[name] = now

        # select를 사용하여 먼저 오는 데이터 처리
        readable, _, _ = select.select(WorldSockets, [], [])

        for sock in readable:
            data, addr = sock.recvfrom(1024)  # 데이터 수신
            msg = data.decode().strip()

            for i in range(len(macron)):
                # try:
                    if (macron[i].Type == "User"):
                        
                        # Player에서 온 키 입력 처리
                        if sock == macron[i].Player.socket:
                            Player2Kart(macron[i], msg)
                            send(macron[i].Kart, "[name]")
                        # Kart에서 온 데이터 처리
                        elif sock == macron[i].Kart.socket:
                            # print("Kart에서 데이터가 옴!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11")
                            Kart2Player(macron[i], msg)


                    elif (macron[i].Type == "Infra"):
                        # infra에서 온 데이터 처리1
                        if sock == macron[i].socket:
                            send(macron[i],msg)

                # except:
                #     print("ERROR:데이터가공 오류!")
                #     print(f"ERROR:[{msg}]")
                #     print(f"길이:{len(msg)}")
                #     pass

    except KeyboardInterrupt:
        for i in userList:
            send(i.Player,"gameOver")
        print("Exiting...")
        # csv파일 저장
        csvFileSave(macron)
        for sock in WorldSockets:
            sock.close()
            print(sock)
        break

print("Exiting...")
# csv파일 저장
csvFileSave(macron)
for sock in WorldSockets:
    sock.close()
    print(sock)
