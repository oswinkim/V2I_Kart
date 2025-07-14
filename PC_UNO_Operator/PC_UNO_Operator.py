#---모듈 불러오기---
import socket
import select
import os
import random as r

import json
import csv

import math

#---테스트 용 리스트--
rats_life=0
rat_list=[]

#---클래스 정의---
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
        # 월드 소켓 리스트에 소켓 추가
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
    def __init__(self, Name="", Ip="", Send_port="", Rev_port="", ):
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
        self.color_name=""

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
                 Name_kart="", Ip_kart="", Send_port_kart="", Recv_port_kart="",
                 Name_player="", Ip_player="", Send_port_player="", Recv_port_player="",
                 role="jobless", goal="none"):
#        print("-----------------------------")
#        print("[User 설정시작]")
        self.Type = "User"
#        print(f"객체 타입: {self.Type}")

        if not User_Name:
            User_Name = input("유저 이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.User_Name = User_Name
        self.role = role
        self.goal = goal
        self.life = 1
#        print(f"유저 이름: {self.User_Name}\n")

        self.kart = kart(Name_kart, Ip_kart, Send_port_kart, Recv_port_kart)
        self.player = Player(Name_player, Ip_player, Send_port_player, Recv_port_player)

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

        print(f"<유저[{User_Name}] 기본 설정 완료>")
#       print("-----------------------------")
        self.junction_count = 0
        self.junction_range = 4
        self.junction_select_log = [0 for _ in range(self.junction_range)]
        self.is_awaiting_exchange = False
        self.last_segment = 0
        self.others_junction_log = [0,0,0,0]
        self._initialize_segments()

    def _initialize_segments(self):
        start_segment = 10 +10
        self.exchange_segment = math.ceil((self.junction_range+3))*10
        end_segment = ((self.junction_range+2)*2+1)*10
        self.control_segments = {
            start_segment, 
            self.exchange_segment, 
            end_segment
        }

    def segment_detection(self):
        try: 
            current_segment = int(self.driving_record[-1][0])
        except:
            current_segment = 0
        if self.last_segment < current_segment:
            segment_is_junction       = ((current_segment//10)%2 == 1) and \
                                        (current_segment not in self.control_segments)
            segment_in_junction_range = self.junction_count < self.junction_range
            if segment_is_junction and segment_in_junction_range:
                    self.junction_select_log[self.junction_count] = current_segment%10
                    self.junction_count += 1
            if current_segment == self.exchange_segment:
                self.is_awaiting_exchange = True
            self.last_segment = current_segment

#---함수 정의--
def handle_kart_junction_log(user_obj, received_msg):
    print(f"INFO: Kart ({user_obj.kart.Name})로부터 Junction Log 수신: {received_msg}")
    try:
        received_log_str = received_msg[14:] 
        received_log_list = [int(x) for x in received_log_str.split('|')]
        user_obj.others_junction_log = received_log_list
        print(f"INFO: Kart의 Junction Log 저장됨: {user_obj.others_junction_log}")
        send(user_obj.kart, "success") 
        print(f"INFO: Kart ({user_obj.kart.Name})에게 Junction Log 수신 성공 응답 보냄.")
        return True
    except Exception as e:
        print(f"WARNING: Kart Junction Log 처리 중 오류 발생: {e}")
        return False

def check_all_users_awaiting_exchange(user_list):
    for user in user_list:
        if not user.is_awaiting_exchange:
            return False
    print("INFO: all user are awaiting exchange. start junction_select_log exchange")
    junction_info_exchange(user_list)
    print("all user's awaiting exchange mode OFF")

def junction_info_exchange(user_list):
    for user in user_list:
        log_to_send = user.junction_select_log
        #log_to_send = user.junction_select_log[:len(user.junction_select_log)//2]
        log_to_send = log_to_send[::-1]
        message_to_send = f"[junction_log]{"|".join(map(str, log_to_send))}"

        sending_and_recv_check(user.player, message_to_send)
        sending_and_recv_check(user.kart, message_to_send)
        print(f"{user}'s junction_select_log sending for all users (log:{message_to_send})")
        print(f"{user}'s awaiting exchange mode OFF")
        user.is_awaiting_exchange = False

def sending_and_recv_check(target_device, data_to_send, MAX_RETRIES=1024):
    for _ in range(MAX_RETRIES):
        send(target_device, data_to_send)

        readable_sockets, _, _ = select.select(WorldSockets, [], [], 1)
        for sock in readable_sockets:
            if sock == target_device.socket: # 현재 처리 중인 디바이스의 소켓인지 확인
                data, addr = sock.recvfrom(1024)
                msg = data.decode().strip()
                if msg == "success":
                    print(f"INFO: {target_device}가 수신 성공")
                    send(target_device, "success")
                    return True
                else:
                    print(f"WARNING: 송신과 관계없는 메시지가 수신됨({target_device}: {msg})")
        print(f"INFO: {target_device}로부터 수신이 확인되지 않음. 재전송...")
    print(f"{MAX_RETRIES}번의 확인 시도가 실패함. 처리를 건너뜀")
    return False

#       print("-----------------------------")

#---함수 정의---
def send(target, msg):
    target.socket.sendto(msg.encode(), (target.Ip, target.Rev_port))
    # print(f"{target.Name}에게 {msg}를 보냈습니다.")

def sends(t:list, msg):
    for target in t:
        target.socket.sendto(msg.encode(), (target.Ip, target.Rev_port))
        print(f"{target.Name}에게 {msg}를 보냈습니다.")

def get_unique_filename(directory, base_name, extension=".csv"):
    # 디렉토리 존재 여부 확인 및 생성
    os.makedirs(directory, exist_ok=True)

    filename = f"{base_name}{extension}"
    full_path = os.path.join(directory, filename)

    counter = 1
    while os.path.exists(full_path):
        filename = f"{base_name}_{counter}{extension}"
        full_path = os.path.join(directory, filename)
        counter += 1

    return full_path

def csv_file_save(MACRON:list):
    for i in range(len(MACRON)):
        if (MACRON[i].Type == "User"):
            qual = 0
            while 1:
                qual = input(f"INFO: {MACRON[i].User_Name}의 데이터 수집 결과 결정 (GOOD:2, NOMAL:1, BAD:0)\n입력: ")
                try:
                    qual=int(qual)
                    if 0<=qual<=2:
                        final = input(f"INFO: {qual}을 선택하신게 맞다면 1을 아니라면 0을 눌러주세요.\n입력:")
                        print(f"{final}을 입력하셨습니다.")
                        break
                    else:
                        print(f"WARNING: {qual}을 입력하셨습니다.\n 0,1,2 중에 입력해주세요")
                except:
                    print(f"WARNING: 입력하신 자료형은 정수가 아닙니다.({type(qual)}형으로 입력됨)")

            if qual == 0:
                qual = "[bad]"
            elif qual == 1:
                qual = "[nomal]"
            else:
                qual = "[good]"
            
            file_path = "c:\\Users\\PC-16\\Desktop\\V2I_Kart\\PC_UNO_Operator\\data\\" 
            base_name = qual + MACRON[i].User_Name 
            file_name = get_unique_filename(file_path,base_name)

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

def connecting(device_list:list):
    #난수 발생
    key_num = str(r.randrange(10,100))

    '''    
    for _ in range(MAX_RETRIES):
        send(target_device, data_to_send)

        readable_sockets, _, _ = select.select(WorldSockets, [], [], 1)
        for sock in readable_sockets:
            if sock == target_device.socket: # 현재 처리 중인 디바이스의 소켓인지 확인
                data, addr = sock.recvfrom(1024)
                msg = data.decode().strip()
                if msg == "success":
                    print(f"INFO: {target_device}가 수신 성공")
                    send(target_device, "success")
                    return True
                else:
                    print(f"WARNING: 송신과 관계없는 메시지가 수신됨({target_device}: {msg})")
        print(f"INFO: {target_device}로부터 수신이 확인되지 않음. 재전송...")
    print(f"{MAX_RETRIES}번의 확인 시도가 실패함. 처리를 건너뜀")
    return False'''

    for device in range(len(device_list)):
        if (device_list[device].Type == "User"):
            user_name = device_list[device].User_Name
            player_device = device_list[device].player
            kart_device = device_list[device].kart
            print(f"DEBUG: 유저{user_name}와 연결 시작")

            #플레이어 연결
            print(f"DEBUG: 유저 {user_name}의 PLAYER와 연결 시작...")
            while True:
                #난수 전송
                send(player_device, key_num)
                readable, _, _ = select.select(WorldSockets, [], [], 1)
                if readable:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()
                    if sock == player_device.socket:
                        if key_num in msg:
                            print(f"DEBUG: 유저 {user_name}의 PLAYER와 연결성공!")
                            send(player_device, "success")
                            break
                        else:
                            print(f"WARNING: 유저 {user_name}의 PLAYER의 연결과 관계없는 메시지가 수신됨({msg})")
                else:
                    print(f"WARNING: 유저 {user_name}의 PLAYER로부터 수신이 확인되지 않음. 재전송...")
            #카트 연결
            print(f"DEBUG: 유저 {user_name}의 KART와 연결 시작...")
            while True:
                #난수 전송
                send(kart_device, key_num)
                readable, _, _ = select.select(WorldSockets, [], [], 1)
                if readable:
                    for sock in readable:
                        data, addr = sock.recvfrom(1024)  # 데이터 수신
                        msg = data.decode().strip()
                    if sock == kart_device.socket:
                        if key_num in msg:
                            print(f"DEBUG: 유저 {user_name}의 KART와 연결성공!")
                            send(player_device, "success")
                            break
                        else:
                            print(F"WARNING: 유저 {user_name} KART의 연결과  관계없는 메시지가 수신됨({msg})")
                else:
                    print(f"WARNING: 유저 {user_name}의 KART로부터 수신이 확인되지 않음. 재전송...")
                
            # #AHRS 정상 작동 확인
            # print(f"\n카트[{M[i].kart.Name}]의 AHRS센서 정상 작동 확인중...")
            # while 1:
            #     send(M[i].kart, "ahrs")

            #     readable, _, _ = select.select(WorldSockets, [], [], 1)
                
            #     if not readable:
            #         print("패스")
            #     else:
            #         for sock in readable:
            #             data, addr = sock.recvfrom(1024)  # 데이터 수신
            #             msg = data.decode().strip()

            #         if sock == M[i].kart.socket:
            #             if "[ahrs]" in msg:
            #                 msg = msg[6:]
            #                 try:
            #                     msg = float(msg)
            #                     if abs(msg) > 0:
            #                         M[i].kart.AHRS_start = msg
            #                         print("AHRS 정상 작동")
            #                         print(f"작동값:{msg}")
            #                         break
            #                     else:
            #                         print("WARNING:AHRS 오류")
            #                         print(f"작동값:{msg}")
            #                 except:
            #                     print(f"WARNING:메시지 형식 오류")
            #                     print(f"msg:{msg}")
            #                     print(f"type:{type(msg)}")

            #             elif num not in msg:
            #                 print("WARNING:요청하지 않은 메시지")
            #                 print(f"msg:{msg}")

            # #color센서 정상 작동 확인
            # print(f"\n카트[{M[i].kart.Name}]의 Color센서 정상 작동 확인중...")
            # while 1:
            #     send(M[i].kart, "Color")
                
            #     readable, _, _ = select.select(WorldSockets, [], [], 1)
            #     if not readable:
            #         print("패스")
            #     else:
            #         for sock in readable:
            #             data, addr = sock.recvfrom(1024)  # 데이터 수신
            #             msg = data.decode().strip()

            #         if sock == M[i].kart.socket:
            #             if "[Color]" in msg:
            #                 msg = msg[7:]

            #                 lux, msg_r, msg_g, msg_b = msg.split(',')
            #                 if abs(int(lux)) > 0 and abs(int(msg_r)) > 0 and abs(int(msg_g)) > 0 and abs(int(msg_b)) > 0:
            #                     M[i].kart.color = msg
            #                     print("Color 정상 작동")
            #                     print(f"작동값:{msg}")
            #                     # print(f"현재색깔:{color_define(lux,msg_r,msg_g,msg_b,color_tunning, "None")}")
            #                     break
            #                 else:
            #                     print("WARNING:Color 오류")
            #                     print(f"작동값:{msg}")

            #             elif num not in msg:
            #                 print("WARNING:요청하지 않은 메시지")
            #                 print(f"msg:{msg}")

            print(f"DEBUG: 유저{device_list[device].User_Name}의 모든 DEVICE와 연결 성공\n")


def player2kart(U, msg):
    global rats_life
    global game_state
    # if (U.role == "rat" and "enter" in msg):
    #     send(U.player, "skill activative")
    #     send(U.kart, "stop")
        
    # else:
    if msg in keys_move:
        U.kart.socket.sendto(msg.encode(), (U.kart.Ip, U.kart.Rev_port)) # move
        print(f"Sent to [{U.kart.Name}]ESP: {msg}")

    elif msg == "space":
        U.kart.socket.sendto("i".encode(), (U.kart.Ip, U.kart.Rev_port))  # motor OFF
        print(f"Sent to [{U.kart.Name}]ESP: motor OFF")

    elif "enter" in msg:
        if U.role == "cat":
            print(f"카트색:{U.kart.color_name}")
            print(f"카트색(딕셔너리 넘버):{color_relation[U.kart.color_name]}")
            numm=color_relation[U.kart.color_name]
            if (numm%2 == 0):
                send(U.kart, "stop")
                send(U.player, "사냥구역이 입니다. 주변 8칸을 사냥합니다(5초 간 사냥)")

                for j in rat_list:
                    print(type(j))
                    if(color_relation[j.kart.color_name]-color_relation[j.kart.color_name]== 1):
                        send(U.player,f"{j.User_Name}을 사냥했습니다.")
                        send(U.kart, "[die]")
                        rats_life -= 1
            else:
                send(U.kart, "stop")
                send(U.player, "사냥구역이 아닙니다. 패널티(5초 정지)")
        else: #rat
            if (color_relation[U.kart.color_name]%2 == 0):
                if(U.goal==U.kart.color_name):
                    send(U.kart, "[die]")
                    rats_life -= 1
                    if rats_life==0:
                        game_state=0
                    print("게임오버, 생명: ", rats_life)
                else:
                    send(U.player,f"[목표색|{U.goal}][|{U.kart.color_name}]")
            else:
                send(U.player, "해당 구간에서는 목적지를 확인할 수 없습니다.")
                
    # elif msg == "m":
    #     U.kart.socket.sendto("ahrs".encode(), (U.kart.Ip, U.kart.Rev_port))  # ahrs값
    #     print("ahrs값을 요청하는중...")
    # elif msg == "c":
    #     U.kart.socket.sendto("Color".encode(), (U.kart.Ip, U.kart.Rev_port))  # ahrs값
    #     print("color값을 요청하는중...")
    '''
    if not U.is_awaiting_exchange:
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
    else:
        U.kart.socket.sendto("i".encode(), (U.kart.Ip, U.kart.Rev_port))  # motor OFF
        print(f"Sent to [{U.kart.Name}]ESP: motor OFF, Because awaiting exchange.")
    '''

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
    #         Dao.kart = msg
    # print(f"2player:{msg}")

    if msg in color_all:
        # msg = msg[8:]
        # histo = msg.split('|')
        # #msg = "0|1234|red|..."
        # #원상형태["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"]
        # #최종형태["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"]
        # #같음
        # U.driving_record.append(histo)

        # print(f"{U.User_Name}의 color:{msg}")
        U.kart.color_name=msg

color_all=["mmdf", "red", "blue", "green", "pink", "orange", "sky", "white"]
color_relation={"mdf":7, "red":2, "blue":6, "green":8, "pink":3, "orange":4, "sky":5, "white":1}

def color_adjust(User):
    if(User.Type == "User"):
        pass
    else: 
        print("WARNING: wrong type in color_adjust")
        return

    all_color_type = ["mmdf", "red", "blue", "green", "pink", "orange", "sky", "white"]
    #6색상
    stored_colors = []
    initial_msg = "[color_adjust]"
    user_name = User.User_Name
    kart = User.kart

    # color_adjust상태로 연결시도
    print(f"DEBUG: {user_name}의 KART를 컬러보정 상태로 변경중...")
    #플레이어 연결
    while True:
        #난수 전송
        send(kart, initial_msg)
        readable, _, _ = select.select(WorldSockets, [], [], 1)
        if readable:
            for sock in readable:
                data, addr = sock.recvfrom(1024)  # 데이터 수신
                msg = data.decode().strip()
            if sock == kart.socket:
                if initial_msg in msg:
                    print(f"DEBUG: 유저 {user_name}의 KART 색상 조정 준비 완료")
                    send(kart, "success")
                    break
                else:
                    print(f"WARNING: 유저 {user_name}의 KART 색상 조정과 관계없는 메시지가 수신됨({msg})")
        else:
            print(f"WARNING: 유저 {user_name}의 KART로부터 수신이 확인되지 않음. 재전송...")
    
    print(f"DEBUG: 유저 {user_name}의 KART 색상 조정 시작")
    initial_adjust_msg = "[raw_color]"
    pass_color_adjust = False
    for color_id in range(len(all_color_type)):
        print(f"INFO: 유저 {user_name}의 KART의 {all_color_type[color_id]}색상을 보정...({(color_id+1)}/{len(all_color_type)})")
        adjust_msg_convention = "color="+all_color_type[color_id]
        if pass_color_adjust:
            break
        while True:
            send(kart, adjust_msg_convention)
            readable, _, _ = select.select(WorldSockets, [], [], 1)
            if readable:
                for sock in readable:
                    data, addr = sock.recvfrom(1024)  # 데이터 수신
                    msg = data.decode().strip()
                if sock == kart.socket:
                    if initial_adjust_msg in msg:
                        print(f"DEBUG: ---유저 {user_name}의 {all_color_type[color_id]}의 원본 색상 데이터---")
                        print(f"Origin:  Name={msg[5*5]}, Lux={msg[5*5+1]}, R={msg[5*5+2]}, G={msg[5*5+3]}, B={msg[5*5+4]}")
                        print(f"Average: Name={msg[5*5]}, Lux={msg[5*5+1]}, R={msg[5*5+2]}, G={msg[5*5+3]}, B={msg[5*5+4]}")
                        msg = msg[12:]
                        msg = msg.split("|")
                        for i in range(5):
                            print(f"{i+1}: Name={msg[5*i]}, Lux={msg[5*i+1]}, R={msg[5*i+2]}, G={msg[5*i+3]}, B={msg[5*i+4]}")
                        print(f"DEBUG: -----------------------------------------")
                        if (all_color_type[color_id] in msg):
                            measurement_accept = input(f"INFO: 유저 {user_name}의 현재 {all_color_type[color_id]}의 색상 값을 반영하시겠습니까? (y/n/p): ")
                        else:
                            measurement_accept = "false"
                        if measurement_accept.lower() == 'y':
                            stored_colors.append([msg[5*5], msg[5*5+1], msg[5*5+2], msg[5*5+3], msg[5*5+4]])
                            print(f"DEBUG: 유저 {user_name}에 반영된 {all_color_type[color_id]}의 색상 값: {[msg[5*5], msg[5*5+1], msg[5*5+2], msg[5*5+3], msg[5*5+4]]}")
                            print(f"DEBUG: 유저 {user_name}에 반영된 {all_color_type[color_id]}의 색상 값: {stored_colors[::-1]}")
                            break
                        elif measurement_accept.lower() == 'p':
                            pass_color_adjust = True
                            break
                        else:
                            print(f"INFO: 유저 {user_name}의 현재 {all_color_type[color_id]}의 색상값 거부 및 재측정 시작")
                    else:
                        print(f"WARNING: 유저 {user_name}의 KART 색상 조정과 관계없는 메시지가 수신됨({msg})")
            else:
                print(f"WARNING: 유저 {user_name}의 KART로부터 수신이 확인되지 않음. {all_color_type[color_id]}색상값 재요청...(color_id: {color_id})")

    print(f"DEBUG: 유저 {user_name}의 모든 색상({all_color_type}) 보정 완료")
    all_color_type=["mdf", "red", "blue", "green", "pink", "orange", "sky", "white"]
    print("유저 {user_name}의 색상 정보 수집 완료")

    msg="color_data"
    for i in range(len(stored_colors)):
        msg += f"|{stored_colors[i][0]}|{stored_colors[i][1]}|{stored_colors[i][2]}|{stored_colors[i][3]}|{stored_colors[i][4]}"
    print(f"DEBUG: 유저 {user_name}에게 전송될 색상 보정 메세지:{type(stored_colors[0][1])}")
    adjusted_color_msg = msg
    while True:
        #난수 전송
        send(kart, adjusted_color_msg)
        readable, _, _ = select.select(WorldSockets, [], [], 1)
        if readable:
            for sock in readable:
                data, addr = sock.recvfrom(1024)  # 데이터 수신
                msg = data.decode().strip()
            if sock == kart.socket:
                if "[save]" in msg:
                    print(f"DEBUG: 유저 {user_name}의 KART에 조정된 색상 저장 완료")
                    #send(kart, "success")
                    break
                else:
                    print(f"WARNING: 유저 {user_name}의 KART 색상 조정과 관계없는 메시지가 수신됨({msg})")
        else:
            print(f"WARNING: 유저 {user_name}의 KART로부터 수신이 확인되지 않음. 재전송...")
    
def goal_set(U):
    
    color = color_all[r.choice([1 ,2, 3, 5])]
    goal=f"[goal]{U.User_Name}의 목표 위치의 색깔은 {color}입니다."
    send(U.player, goal)

    U.goal=color
    print(f"{U.User_Name}의 목표색은 [{color}]로 설정되었습니다.")
    
def game_play():
    global game_state
    global rats_life
    if game_state != 0:
        return
    print(f"[상태|{game_state}]")
    for i in user_list:
        if (i.goal != "none"):
            print(f"[이름|{i.User_Name}]\n역할: {i.role}\n현재색상: {i.kart.color_name}\n목표 색상: {i.goal}")
        else:
            print(f"[이름|{i.User_Name}]\n역할: {i.role}\n현재색상: {i.kart.color_name}")
    while 1:
        decision=input("게임종료를 인정하시겠습니까?(y/n): ")
        if decision.lower() == 'y':
            print("게임종료")
            game_state = -1
            return
        else:
            print(f"[{decision}]키를 눌렀습니다.\n게임을 계속 진행합니다.")
            for i in user_list:
                if (i.goal != "none"):
                    life=input(f"\n[이름|{i.User_Name}]\n[목숨:{i.life}]을 살리시겠습니까?(y/n)")
                    if life.lower() == 'y':
                        i.life = 1
                        rats_life+=1
                        game_state = 1
                        print(f"[이름|{i.User_Name}]\n[목숨:{i.life}]\n")
                    else:
                        i.life = 0
                        print(f"[이름|{i.User_Name}]\n[목숨:{i.life}]\n")
            game_state = 1
            print("[게임상태:{game_state}]\n 게임재게")
            return
##########################################################################################################
#실행 시 변경해야 할 부분

WorldSockets=[]
keys_move=["w","a","s","d"]

game_state=1

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
        Name_kart="핑크베놈", Ip_kart="192.168.0.2",    Send_port_kart="4213",   Recv_port_kart="4212",
        Name_player="다오",   Ip_player="192.168.0.13", Send_port_player="5005", Recv_port_player="5006", 
        role="cat"
    )
Bazzi = User(
        User_Name="sama",
        Name_kart="버스트", Ip_kart="192.168.0.12",   Send_port_kart="7000",   Recv_port_kart="7001",
        Name_player="배찌", Ip_player="192.168.0.21", Send_port_player="8000", Recv_port_player="8001",
        role="rat"
    )

#통신하는 모든 디바이스들
#device_list = [Dao]
device_list = [Dao, Bazzi]

#---메인 앱---
print("------------------------------------------------------------------------------------------")
print("INFO: *정상적으로 연결되지 않을 경우, 네트워크 설정을 확인하십시오.(공용 -> 개인 네트워크)")
print("------------------------------------------------------------------------------------------")
print("Waiting for key input from Player and data from Kart...")

#user분리
user_list=[]
for i in device_list:
    if i.Type == "User":
        user_list.append(i)
connecting(device_list)

for i in user_list:
    if (i.role=="rat"):
        goal_set(i)
        rats_life+=1
        rat_list.append(i)

for i in user_list:
    print("user의 kart 색상 보정")
    color_adjust(i)
color_all=["mdf", "red", "blue", "green", "pink", "orange", "sky", "white"]

while True:
    try:
        if (rats_life==0):
            game_state == 0
            print(f"게임값:{game_state}")

        game_play()            
        if(game_state==-1):
            break

        # select를 사용하여 먼저 오는 데이터 처리
        readable, _, _ = select.select(WorldSockets, [], [])

        for sock in readable:
            data, addr = sock.recvfrom(1024)  # 데이터 수신
            msg = data.decode().strip()

            for i in range(len(device_list)):
                # try:
                    if (device_list[i].Type == "User"):
                        
                        # player에서 온 키 입력 처리
                        if sock == device_list[i].player.socket:
                            player2kart(device_list[i], msg)
                        # kart에서 온 데이터 처리
                        elif sock == device_list[i].kart.socket:
                            # print("kart에서 데이터가 수신됨!!!!!!!!!!!!!!!")
                            # 메시지 타입을 먼저 확인하여 분배
                            if msg.startswith("[junction_log]"):
                                handle_kart_junction_log(device_list[i], msg) # 별도 함수 호출
                            else:
                                kart2player(device_list[i], msg) # 기존 kart2player는 다른 메시지 처리
                        send(device_list[i].kart,"[name]")
                        
                        device_list[i].segment_detection()


                    elif (device_list[i].Type == "Infra"):
                        # infra에서 온 데이터 처리
                        if sock == device_list[i].socket:
                            send(device_list[i],msg)

                # except:
                #     print("WARNING:데이터가공 오류!")
                #     print(f"WARNING:[{msg}]")
                #     print(f"길이:{len(msg)}")
                #     pass

            check_all_users_awaiting_exchange(user_list)

    except KeyboardInterrupt:
        print("Exiting...")
        #csv파일 저장
        csv_file_save(device_list)
        for sock in WorldSockets:
            sock.close()
            print(sock)
        break

print("Exiting...")
#csv파일 저장
csv_file_save(device_list)
for sock in WorldSockets:
    sock.close()
    print(sock)