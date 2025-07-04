##########################################################################################################
#불러오는 모듈
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
        self.AHRS= 0
        self.color=""

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

        self.driving_record = [["구간", "현재시간", "컬러변환값", "왼쪽 모터상태","오른쪽 모터상태", "방향변환값", "raw컬러값", "raw방향값"],]

        self.stopline = 0

        self.kart = kart(Name_kart, Ip_kart, Send_port_kart, Rev_port_kart)
        self.player = Player(Name_player, Ip_player, Send_port_player, Rev_port_player)

        print(f"<유저[{User_Name}] 기본 설정 완료>")
#        print("-----------------------------")

##########################################################################################################

def csv_file_save(MACRON:list):
    for i in range(len(MACRON)):
        if (MACRON[i].Type == "User"):
            file_path= MACRON[i].User_Name + ".csv"
            with open(file_path, 'w', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                writer.writerow([f'User_Name={MACRON[i].User_Name}', 
                                 f'Name_kart={MACRON[i].kart.Name}', 
                                 f'Ip_kart={MACRON[i].kart.Ip}', 
                                 f'Send_port_kart={MACRON[i].kart.Send_port}', 
                                 f'Rev_port_kart={MACRON[i].kart.Rev_port}', 
                                 f'Name_player={MACRON[i].player.Name}', 
                                 f'Ip_player={MACRON[i].player.Ip}', 
                                 f'Send_port_player={MACRON[i].player.Send_port}', 
                                 f'Rev_port_player={MACRON[i].player.Rev_port}', 
                                 ])
                writer.writerow([]) 
                for row in MACRON[i].driving_record:
                    writer.writerow(row)  # 각 행 쓰기

######################################################################


Dao = User(
        User_Name="almaeng",
        Name_kart="핑크베놈", Ip_kart="192.168.0.2", Send_port_kart="4213", Rev_port_kart="4212",
        Name_player="다오", Ip_player="192.168.0.8", Send_port_player="5005", Rev_port_player="5006"
    )

macron=[Dao]

histo=[1, 2, 3, 4, 5, 6, 7, 8]
Dao.driving_record.append(histo)

csv_file_save(macron)

