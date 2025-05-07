import socket
import select

WorldSockets=[]

class User:
    def __init__(self, Name='', Priority=0, EspIp="", EspRecvPort=0, EspSendPort=0, PcIp="", PcRecvPort=0, PcSendPort=0):
        print("[초기설정시작]")
        if Name == '':
            Name = input("이름이 설정되지 않았습니다.ex) 다오")
        self.Name = Name
        print(f"이름: {self.Name}")

        if Priority == 0:
            Priority = input(f"{self.Name}의 우선순위를 정수로 작성해주세요.ex) 1")
        self.Priority=Priority
        print(f"{self.Name}의 우선순위: {self.Priority}")        
        
        #esp32
        if EspIp == "":
            EspIp = input(f"{self.Name}의 (esp32보드)ip를 작성해주세요.ex)192.168.0.0")
        self.EspIp=EspIp
        print(f"{self.Name}의 (esp32보드)ip: {self.EspIp}")

        if EspRecvPort == 0:
            EspRecvPort = input(f"{self.Name}의 esp32가 명령을 받을 수신포트 번호를 작성해주세요.ex)1234")
        self.EspRecvPort=EspRecvPort
        print(f"{self.Name}의 esp32가 명령을 받을 수신포트: {self.EspRecvPort}")        

        if EspSendPort == 0:
            EspSendPort = input(f"{self.Name}의 esp32가 명령을 보낼 송신포트 번호를 작성해주세요.ex)1234")
        self.EspSendPort=EspSendPort
        print(f"{self.Name}의 esp32가 명령을 보낼 송신포트: {self.EspSendPort}")        
        
        #pc
        if PcIp == "":
            PcIp = input(f"{self.Name}의 (유저컴퓨터)ip를 작성해주세요.ex)192.168.0.0")
        self.PcIp=PcIp
        print(f"{self.Name}의 (유저컴퓨터)ip: {self.PcIp}")

        if PcRecvPort == 0:
            PcRecvPort = input(f"{self.Name}의 Pc 키입력을 받을 수신포트 번호를 작성해주세요.ex)1234")
        self.PcRecvPort=PcRecvPort
        print(f"{self.Name}의 Pc 키입력을 받을 수신포트: {self.PcRecvPort}")        

        if PcSendPort == 0:
            PcSendPort = input(f"{self.Name}의 Pc로 데이터를 보낼 송신포트 번호를 작성해주세요.ex)1234")
        self.PcSendPort=PcSendPort
        print(f"{self.Name}의 Pc로 데이터를 보낼 송신포트: {self.PcSendPort}\n")  

        #소캣 바인딩
        self.SockRecv_UserPc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.SockRecv_UserPc.bind(("", self.PcRecvPort))  # PC2에서 키 입력을 받을 소켓

        self.SockSend_UserPc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # PC2로 온도 전송용 소켓

        self.SockSend_esp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.SockSend_esp.bind(("", self.EspSendPort))  # ESP32가 온도 데이터를 보낼 소켓

        self.Sockets=[self.SockRecv_UserPc, self.SockSend_esp]

        WorldSockets.extend(self.Sockets)



Dao = User(Name='다오', Priority=1, EspIp="192.168.0.0", EspRecvPort=4210, EspSendPort=4211, PcIp="192.168.0.0", PcRecvPort=5005, PcSendPort=5006)
# Bazzi = User(Name='배찌', Priority=2, EspIp="192.168.0.0", EspRecvPort=4212, EspSendPort=4213, PcIp="192.168.0.0", PcRecvPort=5007, PcSendPort=5008)
macron=[Dao]

# 우선순위의 정렬
for i in range(len(macron)):
    macron[i].Priority = i + 1

print("*정상적으로 연결되지 않을 경우 네트 워크 설정을 확인하십시오.(공용 -> 개인 네트워크)")
print("Waiting for key input from PC2 and data from ESP32...")

while True:

    try:
        # select를 사용하여 먼저 오는 데이터 처리
        readable, _, _ = select.select(WorldSockets, [], [])

        for sock in readable:
            data, addr = sock.recvfrom(1024)  # 데이터 수신
            msg = data.decode().strip()

            for i in range(len(macron)):
                try:
                        # PC2에서 온 키 입력 처리
                    if sock == macron[i].SockRecv_UserPc:
                        print(f"Received from [{macron[i].Name}]PC: {msg}")

                        if msg == "1":
                            macron[i].SockSend_esp.sendto("1".encode(), (macron[i].EspIp, macron[i].EspRecvPort))  # motor ON
                            print(f"Sent to [{macron[i].Name}]ESP: motor ON")

                        elif msg == "0":
                            macron[i].SockSend_esp.sendto("0".encode(), (macron[i].EspIp, macron[i].EspRecvPort))  # motor OFF
                            print(f"Sent to [{macron[i].Name}]ESP: motor OFF")

                        # 추후 데이터 전송 명령어
                        elif msg == "2":
                            macron[i].SockSend_esp.sendto("REQ".encode(), (macron[i].EspIp, macron[i].EspRecvPort))  # 온도 요청
                            print("랜덤값을 요청하는중...")

                    # ESP32에서 온도 데이터 수신 → PC2로 전달
                    elif sock == macron[i].SockSend_esp:
                        print(f"\nReceived Random Value from [{macron[i].Name}]ESP: {msg}")
                        macron[i].SockSend_UserPc.sendto(msg.encode(), (macron[i].PcIp, macron[i].PcSendPort))  # PC2로 온도 전송
                        print(f"Sent Random Value to PC2: {msg}")
                except:
                    pass

    except KeyboardInterrupt:
        print("Exiting...")
        for sock in WorldSockets:
            sock.close()
            print(sock)
        break
