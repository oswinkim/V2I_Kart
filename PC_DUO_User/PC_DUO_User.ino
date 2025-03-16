import socket
import keyboard

PC1_IP = "192.168.0.0"  # PC1 IP
PC1_PORT = 5005           # PC1이 키 입력을 받을 포트
PC2_RECV_PORT = 5006  # PC1에서 데이터를 받을 포트

sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind(("", PC2_RECV_PORT))  # 온도 데이터 수신 소켓

print("Press 'o' to turn ON LED, 'f' to turn OFF LED, 't' to get temperature, or 'q' to quit.")

while True:
    try:
        event = keyboard.read_event()
        if event.event_type == keyboard.KEY_DOWN:  # 키 눌림 감지
            key = event.name  # 눌린 키 이름

            if key == "q":  # 'q' 입력 시 종료
                print("Exiting...")
                break

            # PC1로 키 값 전송
            sock_send.sendto(key.encode(), (PC1_IP, PC1_PORT))
            print(f"Sent to PC1: {key}")

        # 온도 데이터 수신
        sock_recv.settimeout(0.1)  # 데이터가 없으면 넘어가기
        try:
            data, _ = sock_recv.recvfrom(1024)
            temp = data.decode()
            print(f"Received Temperature from PC1: {temp}°C")
        except socket.timeout:
            pass  # 데이터 없으면 넘어감

    except KeyboardInterrupt:
        break
