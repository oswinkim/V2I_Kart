import socket
import select

WorldSockets=[]
keys_move=["w","a","s","d"]

class Common:
    num = 0

    def __init__(self, Ip = "", Send_port = "", Rev_port = ""):
        print("[소캣설정 시작]")


        Common.num += 1

        self.Priority = Common.num
        print(f"우선순위: {self.Priority}")

        if not Ip:
            Ip = input("ip가 설정되지 않았습니다.ex) 0.0.0.0: ")
        self.Ip = Ip
        print(f"ip: {self.Ip}")

        if not Send_port:
            Send_port = input("송신포트가 설정되지 않았습니다.ex) 5000: ")
        self.Send_port = int(Send_port)
        print(f"송신포트: {self.Send_port}")

        if not Rev_port:
            Rev_port = input("수신포트가 설정되지 않았습니다.ex) 5001: ")
        self.Rev_port = int(Rev_port)
        print(f"수신포트: {self.Rev_port}\n")

    def Bind_listen(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(("", self.Send_port))

        WorldSockets.append(self.socket)

class Player(Common):
    def __init__(self, Name="", Ip="", Send_port="", Rev_port=""):
        print("[Player 설정시작]")
        self.Type = "Player"
        print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("플레이어의 이름이 설정되지 않았습니다.ex) 다오: ")
        self.Name=Name
        print(f"플레이어 이름: {self.Name}")

        super().__init__(Ip, Send_port, Rev_port)

        super().Bind_listen()

class kart(Common):
    def __init__(self, Name="", Ip="", Send_port="", Rev_port=""):
        print("[Kart 설정시작]")
        self.Type = "Kart"
        print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("카트의 이름이 설정되지 않았습니다.ex) 코튼: ")
        self.Name=Name
        print(f"카트 이름: {self.Name}")

        super().__init__(Ip, Send_port, Rev_port)

        super().Bind_listen()

class Infra(Common):
    def __init__(self, Name="", Ip="", Send_port="", Rev_port=""):
        print("[Infra 작성시작]")
        self.Type = "Infra"
        print(f"객체 타입: {self.Type}")

        if not Name:
            Name = input("인프라의 이름이 설정되지 않았습니다.ex) 신호등1: ")
        self.Name=Name
        print(f"인프라 이름: {self.Name}")

        super().__init__(Ip, Send_port, Rev_port)

        super().Bind_listen()

class User:
    def __init__(self,
                 User_Name="",
                 Name_kart="", Ip_kart="", Send_port_kart="", Rev_port_kart="",
                 Name_player="", Ip_player="", Send_port_player="", Rev_port_player=""):
        print("-----------------------------")
        print("[User 설정시작]")
        self.Type = "User"
        print(f"객체 타입: {self.Type}")

        if not User_Name:
            User_Name = input("유저 이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.User_Name = User_Name
        print(f"유저 이름: {self.User_Name}\n")

        self.kart = kart(Name_kart, Ip_kart, Send_port_kart, Rev_port_kart)
        self.player = Player(Name_player, Ip_player, Send_port_player, Rev_port_player)

        print("<User 설정 완료>")
        print("-----------------------------")


Dao = User(
        User_Name="almaeng",
        Name_kart="핑크베놈", Ip_kart="192.168.191.240", Send_port_kart="4213", Rev_port_kart="4212",
        Name_player="다오", Ip_player="192.168.191.138", Send_port_player="5005", Rev_port_player="5006"
    )

"""
Bazzi = User(
        User_Name="sama",
        Name_kart="버스트", Ip_kart="128.0.0.3", Send_port_kart="7000", Rev_port_kart="7001",
        Name_player="배찌", Ip_player="128.0.0.4", Send_port_player="8000", Rev_port_player="8001"
    )
"""

print(Dao)

macron=[Dao]


print("*정상적으로 연결되지 않을 경우 네트 워크 설정을 확인하십시오.(공용 -> 개인 네트워크)")
print("Waiting for key input from Player and data from Kart...")

while True:
    try:
        # select를 사용하여 먼저 오는 데이터 처리
        readable, _, _ = select.select(WorldSockets, [], [])

        for sock in readable:
            data, addr = sock.recvfrom(1024)  # 데이터 수신
            msg = data.decode().strip()

            for i in range(len(macron)):
                # player에서 온 키 입력 처리
                try:
                    if sock == macron[i].player.socket:
                        print(f"Received from [{macron[i].player.Name}]PC: {msg}")

                        if msg in keys_move:
                            macron[i].kart.socket.sendto(msg.encode(), (macron[i].kart.Ip, macron[i].kart.Rev_port))  # motor ON
                            print(f"Sent to [{macron[i].kart.Name}]ESP: {msg}")

                        elif msg == "i":
                            macron[i].kart.socket.sendto("i".encode(), (macron[i].kart.Ip, macron[i].kart.Rev_port))  # motor OFF
                            print(f"Sent to [{macron[i].kart.Name}]ESP: motor OFF")

                        # 추후 데이터 전송 명령어
                        elif msg == "m":
                            macron[i].kart.socket.sendto("ahrs".encode(), (macron[i].kart.Ip, macron[i].kart.Rev_port))  # ahrs값
                            print("ahrs값을 요청하는중...")

                        elif msg == "c":
                            macron[i].kart.socket.sendto("Color".encode(), (macron[i].kart.Ip, macron[i].kart.Rev_port))  # ahrs값
                            print("color값을 요청하는중...")


                        else:
                            print(f"undefine key value: {msg}")

                    # ESP32에서 온도 데이터 수신 → PC2로 전달
                    elif sock == macron[i].kart.socket:
                        print(f"\nReceived from [{macron[i].kart.Name}]ESP: {msg}")

                        if "[ahrs]" in msg:
                            msg=msg[6:]
                            macron[i].player.socket.sendto(msg.encode(), (macron[i].player.Ip, macron[i].player.Rev_port))  
                            print(f"Sent ahrs Value to PC2: {msg}")

                        elif "[color]" in msg:
                            msg=msg[7:]
                            macron[i].player.socket.sendto(msg.encode(), (macron[i].player.Ip, macron[i].player.Rev_port))  
                            print(f"Sent color Value to PC2: {msg}")


                        
                except:
                    print("목표가 격파됨")
                    pass

    except KeyboardInterrupt:
        print("Exiting...")
        for sock in WorldSockets:
            sock.close()
            print(sock)
        break
