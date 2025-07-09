##########################################################################################################
#불러오는 모듈
import socket
import select

import csv
import time
import random as r

##########################################################################################################
#클래스

class Common:
    num = 0

    def __init__(self, Ip = "", Send_port = "", Rev_port = ""):
#        print("[소캣설정 시작]")

        Common.num += 1

        self.Priority = Common.num
#        print(f"우선순위: {self.Priority}")

        if not Ip:
            Ip = input("ip가 설정되지 않았습니다.ex) 0.0.0.0: ")
        self.Ip = Ip
#        print(f"ip: {self.Ip}")

        if not Send_port:
            Send_port = input("송신포트가 설정되지 않았습니다.ex) 5000: ")
        self.Send_port = int(Send_port)
#        print(f"송신포트: {self.Send_port}")

        if not Rev_port:
            Rev_port = input("수신포트가 설정되지 않았습니다.ex) 5001: ")
        self.Rev_port = int(Rev_port)
#        print(f"수신포트: {self.Rev_port}\n")


        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(("", self.Send_port))

        WorldSockets.append(self.socket)

class Player(Common):
    def __init__(self, Name="", Ip="", Send_port="", Rev_port=""):
#        print("[Player 설정시작]")
        self.Type = "Player"
#        print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("플레이어의 이름이 설정되지 않았습니다.ex) 다오: ")
        self.Name=Name
#        print(f"플레이어 이름: {self.Name}")

        super().__init__(Ip, Send_port, Rev_port)

class kart(Common):
    def __init__(self, Name="", Ip="", Send_port="", Rev_port=""):
#        print("[Kart 설정시작]")
        self.Type = "Kart"
#        print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("카트의 이름이 설정되지 않았습니다.ex) 코튼: ")
        self.Name=Name
#        print(f"카트 이름: {self.Name}")
        
        self.AHRS_start = 0
        self.AHRS = 0

        self.color=""

        self.left_motor = 0
        self.right_motor = 0

        super().__init__(Ip, Send_port, Rev_port)

class Infra(Common):
    def __init__(self, Name="", Ip="", Send_port="", Rev_port=""):
#        print("[Infra 작성시작]")
        self.Type = "Infra"
#        print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("인프라의 이름이 설정되지 않았습니다.ex) 신호등1: ")
        self.Name=Name
#        print(f"인프라 이름: {self.Name}")

        super().__init__(Ip, Send_port, Rev_port)

class User:
    def __init__(self,
                 User_Name="",
                 Name_kart="", Ip_kart="", Send_port_kart="", Rev_port_kart="",
                 Name_player="", Ip_player="", Send_port_player="", Rev_port_player=""):
#        print("-----------------------------")
#        print("[User 설정시작]")
        self.Type = "User"
#        print(f"객체 타입: {self.Type}")

        if not User_Name:
            User_Name = input("유저 이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.User_Name = User_Name
#        print(f"유저 이름: {self.User_Name}\n")

        self.kart = kart(Name_kart, Ip_kart, Send_port_kart, Rev_port_kart)
        self.player = Player(Name_player, Ip_player, Send_port_player, Rev_port_player)

        self.driving_record = [ [f'User_Name={self.User_Name}', 
                                 f'Name_kart={self.kart.Name}', 
                                 f'Ip_kart={self.kart.Ip}', 
                                 f'Send_port_kart={self.kart.Send_port}', 
                                 f'Rev_port_kart={self.kart.Rev_port}', 
                                 f'Name_player={self.player.Name}', 
                                 f'Ip_player={self.player.Ip}', 
                                 f'Send_port_player={self.player.Send_port}', 
                                 f'Rev_port_player={self.player.Rev_port}', 
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

        print(f"<유저[{User_Name}] 기본 설정 완료>")
#        print("-----------------------------")

##########################################################################################################
#함수

def send(target, msg):
    target.socket.sendto(msg.encode(), (target.Ip, target.Rev_port))
    print(f"{target.Name}에게 {msg}를 보냈습니다.")

def sends(t:list, msg):
    for target in t:
        target.socket.sendto(msg.encode(), (target.Ip, target.Rev_port))
        print(f"{target.Name}에게 {msg}를 보냈습니다.")

def csv_file_save(MACRON:list):
    for i in range(len(MACRON)):
        if (MACRON[i].Type == "User"):
            qual = 0
            while 1:
                qual = input(f"{MACRON[i].User_Name}의 데이터 수집 결과 결정 (GOOD:2, NOMAL:1, BAD:0)\n입력: ")
                try:
                    qual=int(qual)
                    if 0<=qual<=2:
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
            
            file_name = "c:\\Users\\PC-16\\Desktop\\V2I_Kart\\PC_UNO_Operator\\data\\" + qual + MACRON[i].User_Name + ".csv"

            with open(file_name, 'w', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                # writer.writerow([f'User_Name={MACRON[i].User_Name}', 
                #                  f'Name_kart={MACRON[i].kart.Name}', 
                #                  f'Ip_kart={MACRON[i].kart.Ip}', 
                #                  f'Send_port_kart={MACRON[i].kart.Send_port}', 
                #                  f'Rev_port_kart={MACRON[i].kart.Rev_port}', 
                #                  f'Name_player={MACRON[i].player.Name}', 
                #                  f'Ip_player={MACRON[i].player.Ip}', 
                #                  f'Send_port_player={MACRON[i].player.Send_port}', 
                #                  f'Rev_port_player={MACRON[i].player.Rev_port}', 
                #                  ])
                # writer.writerow([]) 
                for row in MACRON[i].driving_record:
                    writer.writerow(row)  # 각 행 쓰기
                print(f"저장된 데이터 파일명: {file_name}")

def connecting(M:list):
    #난수 발생
    num = str(r.randrange(10,100))
    
    for i in range(len(M)):
        if (M[i].Type == "User"):
            print(f"[{M[i].User_Name}과 연결 시작]")

            #플레이어 연결
            print(f"\n플레이어[{M[i].player.Name}]와 연결중...")
            while 1:
                #난수 전송
                send(M[i].player, num)

                readable, _, _ = select.select(WorldSockets, [], [], 1)

                if not readable:
                    print("재전송")
                else:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()

                    if sock == M[i].player.socket:
                        if num in msg:
                            print("연결성공!")
                            send(M[i].player, "success")
                            break
                        else:
                            print("ERROR:요청하지 않은 메시지")
                            print(f"msg:{msg}")

            #카트 연결
            print(f"\n카트[{M[i].kart.Name}]와 연결중...")
            while 1:
                #난수 전송
                send(M[i].kart, num)

                readable, _, _ = select.select(WorldSockets, [], [], 1)

                if not readable:
                    print("재전송")

                else:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()

                    
                    if sock == M[i].kart.socket:
                        if num in msg:
                            print("연결성공!")
                            send(M[i].kart, "success")
                            break
                        else:
                            print("ERROR:요청하지 않은 메시지")
                            print(f"msg:{msg}")

            #AHRS 정상 작동 확인
            print(f"\n카트[{M[i].kart.Name}]의 AHRS센서 정상 작동 확인중...")
            while 1:
                send(M[i].kart, "ahrs")

                readable, _, _ = select.select(WorldSockets, [], [], 1)
                
                if not readable:
                    print("패스")
                else:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()

                    if sock == M[i].kart.socket:
                        if "[ahrs]" in msg:
                            msg = msg[6:]
                            try:
                                msg = float(msg)
                                if abs(msg) > 0:
                                    M[i].kart.AHRS_start = msg
                                    print("AHRS 정상 작동")
                                    print(f"작동값:{msg}")
                                    break
                                else:
                                    print("ERROR:AHRS 오류")
                                    print(f"작동값:{msg}")
                            except:
                                print(f"ERROR:메시지 형식 오류")
                                print(f"msg:{msg}")
                                print(f"type:{type(msg)}")

                        elif num not in msg:
                            print("ERROR:요청하지 않은 메시지")
                            print(f"msg:{msg}")

            #color센서 정상 작동 확인
            print(f"\n카트[{M[i].kart.Name}]의 Color센서 정상 작동 확인중...")
            while 1:
                send(M[i].kart, "Color")
                
                readable, _, _ = select.select(WorldSockets, [], [], 1)
                if not readable:
                    print("패스")
                else:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()

                    if sock == M[i].kart.socket:
                        if "[Color]" in msg:
                            msg = msg[7:]

                            lux, msg_r, msg_g, msg_b = msg.split(',')
                            if abs(int(lux)) > 0 and abs(int(msg_r)) > 0 and abs(int(msg_g)) > 0 and abs(int(msg_b)) > 0:
                                M[i].kart.color = msg
                                print("Color 정상 작동")
                                print(f"작동값:{msg}")
                                # print(f"현재색깔:{color_define(lux,msg_r,msg_g,msg_b,color_tunning, "None")}")
                                break
                            else:
                                print("ERROR:Color 오류")
                                print(f"작동값:{msg}")

                        elif num not in msg:
                            print("ERROR:요청하지 않은 메시지")
                            print(f"msg:{msg}")

            print(f"[{M[i].User_Name}과 연결 완료]\n")


def player2kart(U, msg):
    if msg in keys_move:
        U.kart.socket.sendto(msg.encode(), (U.kart.Ip, U.kart.Rev_port)) # move
        print(f"Sent to [{U.kart.Name}]ESP: {msg}")

    elif msg == "i":
        U.kart.socket.sendto("i".encode(), (U.kart.Ip, U.kart.Rev_port))  # motor OFF
        print(f"Sent to [{U.kart.Name}]ESP: motor OFF")
    # elif msg == "m":
    #     U.kart.socket.sendto("ahrs".encode(), (U.kart.Ip, U.kart.Rev_port))  # ahrs값
    #     print("ahrs값을 요청하는중...")
    # elif msg == "c":
    #     U.kart.socket.sendto("Color".encode(), (U.kart.Ip, U.kart.Rev_port))  # ahrs값
    #     print("color값을 요청하는중...")

    else:
        print(f"undefine key value: {msg}")

"""
def color_define(lux,r,g,b,tuning:list, U):
    raw=[lux,r,g,b]
    print(f"raw color value:{raw}")
    deviation=[]

    try:
        if U.Type == "User":
            Dao.kart.color_R = int(r)
            Dao.kart.color_G = int(g)
            Dao.kart.color_B = int(b)

        else:
            print("객체의 타입이 잘못되었습니다.(User를 위한 함수)")
    except:
        print("객체없음")

    for i in range(len(tuning)):
        temp=0
        for j in range(4):
            temp+=(int(tuning[i][j+1])-int(raw[j]))**2
        deviation.append(temp)

    for i in range(len(deviation)):
        if min(deviation)==deviation[i]:
            print(f"컬러 편차제곱합의 최소: {min(deviation)}")
            #출력결과
            return tuning[i][0]
"""
            
def kart2player(U,msg):
    #     if "[ahrs]" in msg:
    #         msg=msg[6:]
    #         U.player.socket.sendto(msg.encode(), (U.player.Ip, U.player.Rev_port))  
    # #        print(f"Sent ahrs Value to PC2: {msg}")
    #         U.kart.AHRS = msg
    #     elif "[color]" in msg:
    #         msg=msg[7:]
    #         U.player.socket.sendto(msg.encode(), (U.player.Ip, U.player.Rev_port))  
    # #        print(f"Sent color Value to PC2: {msg}")
    #         Dao.kart.color = msg
    # print(f"2player:{msg}")
    if "[record]" in msg:
        msg = msg[8:]
        histo = msg.split('|')
        #msg = "0|1234|red|..."
        #원상형태["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"]
        #최종형태["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"]
        #같음
        U.driving_record.append(histo)

##########################################################################################################
#실행 시 변경해야 할 부분

WorldSockets=[]
keys_move=["w","a","s","d"]

"""
#함수 자동화 필요(컬러센서 마다 편차 존재)
color_tunning=[["white", 920, 2500, 1700, 1300], 
               ["red", 65200, 1700, 300, 250],
               ["pink", 50, 2200, 800, 750],
               ["orange", 65300, 2200, 500, 350],
               ["green", 750, 700, 750, 330],
               ["blue", 550, 600, 900, 900],
               ["sky", 850, 1300, 1300, 1100]]
"""
               
Dao = User(
        User_Name="almaeng",
        Name_kart="핑크베놈", Ip_kart="192.168.0.2", Send_port_kart="4213", Rev_port_kart="4212",
        Name_player="다오", Ip_player="192.168.0.13", Send_port_player="5005", Rev_port_player="5006"
    )

""""
Bazzi = User(
        User_Name="sama",
        Name_kart="버스트", Ip_kart="128.0.0.3", Send_port_kart="7000", Rev_port_kart="7001",
        Name_player="배찌", Ip_player="128.0.0.4", Send_port_player="8000", Rev_port_player="8001"
    )
"""

#통신하는 모든 객체들
macron=[Dao]
#macron=[Dao, Bazzi]

##########################################################################################################
#동작부분

#user분리
user_list=[]
for i in macron:
    if i.Type == "User":
        user_list.append(i)

print("*정상적으로 연결되지 않을 경우 네트 워크 설정을 확인하십시오.(공용 -> 개인 네트워크)")
print("Waiting for key input from Player and data from Kart...")

connecting(macron)

while True:
    try:
        # select를 사용하여 먼저 오는 데이터 처리
        readable, _, _ = select.select(WorldSockets, [], [])

        for sock in readable:
            data, addr = sock.recvfrom(1024)  # 데이터 수신
            msg = data.decode().strip()

            for i in range(len(macron)):
                try:
                    if (macron[i].Type == "User"):
                        # player에서 온 키 입력 처리
                        if sock == macron[i].player.socket:
                            player2kart(macron[i], msg)
                        # kart에서 온 데이터 처리
                        elif sock == macron[i].kart.socket:
                            kart2player(macron[i], msg)

                    elif (macron[i].Type == "Infra"):
                        # infra에서 온 데이터 처리
                        if sock == macron[i].socket:
                            send(macron[i],msg)

                except:
                    print("ERROR:데이터가공 오류!")
                    print(f"ERROR:{msg}")
                    pass

    except KeyboardInterrupt:
        print("Exiting...")
        #csv파일 저장
        csv_file_save(macron)
        for sock in WorldSockets:
            sock.close()
            print(sock)
        break
