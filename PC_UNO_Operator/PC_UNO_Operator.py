import socket

class Common:
    num = 0  # 클래스 변수

    def __init__(self, Name, Ip, Send_port, Rev_port):
        print("[초기설정시작]")

        self.Type = "Common"
        print(f"유형: {self.Type}")

        if not Name:
            Name = input("이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.Name = Name
        print(f"이름: {self.Name}")

        Common.num += 1
        self.Priority = Common.num
        print(f"우선순위: {self.Priority}")

        if not Ip:
            Ip = input("ip가 설정되지 않았습니다.ex) 0.0.0.0")
        self.Ip = Ip
        print(f"ip: {self.Ip}")

        if not Send_port:
            Send_port = input("송신포트가 설정되지 않았습니다.ex) 5000")
        self.Send_port = int(Send_port)
        print(f"송신포트: {self.Send_port}")

        if not Rev_port:
            Rev_port = input("수신포트가 설정되지 않았습니다.ex) 5001")
        self.Rev_port = int(Rev_port)
        print(f"수신포트: {self.Rev_port}")

class Player(Common):
    def __init__(self, Name, Ip, Send_port, Rev_port):
        print("[Player 작성시작]")
        super().__init__(Name, Ip, Send_port, Rev_port)
        self.Type = "Player"

        print(f"Player 송신포트: {self.Send_port}")
        self.player_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.player_socket.bind(("", self.Send_port))

class kart(Common):
    def __init__(self, Name, Ip, Send_port, Rev_port):
        print("[Kart 작성시작]")
        super().__init__(Name, Ip, Send_port, Rev_port)
        self.Type = "Kart"

        print(f"Kart 송신포트: {self.Send_port}")
        self.kart_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.kart_socket.bind(("", self.Send_port))

class Infra(Common):
    def __init__(self, Name, Ip, Send_port, Rev_port):
        print("[Infra 작성시작]")
        super().__init__(Name, Ip, Send_port, Rev_port)
        self.Type = "Infra"
        self.infra_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.infra_socket.bind(("", self.Send_port))

class User:
    def __init__(self,
                 User_Name,
                 Name_kart, Ip_kart, Send_port_kart, Rev_port_kart,
                 Name_player, Ip_player, Send_port_player, Rev_port_player):

        if not User_Name:
            User_Name = input("User 이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.User_Name = User_Name
        print(f"User 이름: {self.User_Name}")

        # Kart 초기화
        self.kart = kart(Name_kart, Ip_kart, Send_port_kart, Rev_port_kart)
        self.Name_Kart = self.kart.Name
        self.Ip_Kart = self.kart.Ip
        self.Send_port_Kart = self.kart.Send_port
        self.Rev_port_Kart = self.kart.Rev_port

        # Player 초기화
        self.player = Player(Name_player, Ip_player, Send_port_player, Rev_port_player)
        self.Name_Player = self.player.Name
        self.Ip_Player = self.player.Ip
        self.Send_port_Player = self.player.Send_port
        self.Rev_port_Player = self.player.Rev_port

        self.Type = "User"
Dao = User(
        User_Name="oswin",
        Name_kart="세이버카트", Ip_kart="127.0.0.1", Send_port_kart="5000", Rev_port_kart="5001",
        Name_player="다오", Ip_player="127.0.0.2", Send_port_player="6000", Rev_port_player="6001"
    )


