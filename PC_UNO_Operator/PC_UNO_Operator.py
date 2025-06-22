# import socket
# import select

# WorldSockets=[]

# class User:
#     def __init__(self, Name='', Priority=0, EspIp="", EspRecvPort=0, EspSendPort=0, PcIp="", PcRecvPort=0, PcSendPort=0):
 

#         #소캣 바인딩
#         self.SockRecv_UserPc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#         self.SockRecv_UserPc.bind(("", self.PcRecvPort))  # PC2에서 키 입력을 받을 소켓

#         self.SockSend_UserPc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # PC2로 온도 전송용 소켓

#         self.SockSend_esp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#         self.SockSend_esp.bind(("", self.EspSendPort))  # ESP32가 온도 데이터를 보낼 소켓

#         self.Sockets=[self.SockRecv_UserPc, self.SockSend_esp]

#         WorldSockets.extend(self.Sockets)

# Dao = User(Name='다오', Priority=1, EspIp="192.168.0.0", EspRecvPort=4210, EspSendPort=4211, PcIp="192.168.0.0", PcRecvPort=5005, PcSendPort=5006)
# Bazzi = User(Name='배찌', Priority=2, EspIp="192.168.0.0", EspRecvPort=4212, EspSendPort=4213, PcIp="192.168.0.0", PcRecvPort=5007, PcSendPort=5008)


##
import socket
import select

WorldSockets=[]

num = 0
class Common:
    def __init__(self, Name, Ip, Send_port, Rev_port):
        print("[초기설정시작]")

        # if Type == '':
        #      Type = input("유형이 설정되지 않았습니다.ex) 카트, 플레이어")
        self.Type = "Common"
        print(f"유형: {self.Type}")
        
        if Name == '':
             Name = input("이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.Name = Name
        print(f"이름: {self.Name}")

        global num
        num+=1
        self. Priority = num
        print(f"우선순위: {self.Priority}")

        if Ip == '':
             Ip = input("ip가 설정되지 않았습니다.ex) 0.0.0.0")
        self.Ip = Ip
        print(f"ip: {self.Ip}")

        if Send_port == '':
            ip = input("송신포트가 설정되지 않았습니다.ex) 5000")
        self.Send_port = Send_port
        print(f"송신포트: {self.Send_port}")

        if Rev_port == '':
             Rev_port = input("수신포트가 설정되지 않았습니다.ex) 5001")
        self.Rev_port = Rev_port
        print(f"수신포트: {self.Rev_port}")

class Player(Common):
    def __init(self):
        print("[Player 작성시작]")
        super.__init__()
        self.Type = "Player"

        self.player_socket = socket(socket.AF_INET,socket.SOCK_DGRAM)


class kart(Common):
    def __init(self):
        print("[Kart 작성시작]")
        super.__init__()
        self.Type = "Kart"

        self.kart_socket = socket(socket.AF_INET,socket.SOCK_DGRAM)


class Infra(Common):
    def __init(self):
        super.__init__()
        self.Type = "Infra"

        self.kart_socket = socket(socket.AF_INET,socket.SOCK_DGRAM)

class User(kart, Player):
    def __init__(self, User_Name, Name, Ip, Send_port, Rev_port, Name1, Ip1, Send_port1, Rev_port1):
        
        if User_Name == '':
             User_Name = input("이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.User_Name = User_Name
        print(f"User 이름: {self.User_Name}")

        kart.__init__(self, Name, Ip, Send_port, Rev_port)
        self.Name_Kart = self.Name
        self.Ip_Kart = self.Ip
        self.Send_port_Kart = self.Send_port
        self.Rev_port_Kart = self.Rev_port

        Player.__init__(self, Name1, Ip1, Send_port1, Rev_port1)
        self.Name_Player = self.Name
        self.Ip_Player = self.Ip
        self.Send_port_Player = self.Send_port
        self.Rev_port_Player = self.Rev_port

        self.Type = "User"

Dao =   User('세이버',"192.168.0.1",4210, 4211, '다오',"192.168.0.2",4250, 4214)
