import socket

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
        self.player_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.player_socket.bind(("", self.Send_port))

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

        print("-----------------------------")


Dao = User(
        User_Name="oswin",
        Name_kart="세이버", Ip_kart="127.0.0.1", Send_port_kart="5000", Rev_port_kart="5001",
        Name_player="다오", Ip_player="127.0.0.2", Send_port_player="6000", Rev_port_player="6001"
    )

print(Dao)