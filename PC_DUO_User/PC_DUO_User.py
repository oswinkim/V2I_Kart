import socket
import keyboard
import numpy as np
import cv2

PC1_IP = "192.168.0.0"  # PC1 IP
PC1_PORT = 5005           # PC1이 키 입력을 받을 포트
PC2_RECV_PORT = 5006  # PC1에서 데이터를 받을 포트

UDP_IP = "0.0.0.0"  # 모든 네트워크 인터페이스에서 수신
UDP_PORT = 4222     # ESP32와 동일한 포트 사용
CHUNK_SIZE = 1460   # ESP32-CAM에서 설정한 청크 크기

sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind(("", PC2_RECV_PORT))  # 온도 데이터 수신 소켓

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

jpeg_buffer = bytearray()  # JPEG 데이터를 저장할 버퍼


print("Press 'o' to turn ON LED, 'f' to turn OFF LED, 't' to get temperature, or 'q' to quit.")

while True:
    #---UdpVideoStreaming---#
    data, addr = sock.recvfrom(CHUNK_SIZE)  # UDP 데이터 수신

    # JPEG 시작(0xFFD8) 감지 시, 기존 버퍼 초기화
    if data[0] == 0xFF and data[1] == 0xD8:
        jpeg_buffer = bytearray()
        jpeg_buffer.clear()
    
    jpeg_buffer.extend(data)  # 수신한 데이터를 버퍼에 추가

    # JPEG 끝(0xFFD9) 감지 시, 이미지 디코딩
    if data[-2] == 0xFF and data[-1] == 0xD9:
        frame = np.frombuffer(jpeg_buffer, dtype=np.uint8)  # 바이트 배열을 NumPy 배열로 변환
        image = cv2.imdecode(frame, cv2.IMREAD_COLOR)  # OpenCV를 이용해 이미지 디코딩

        jpeg_buffer = bytearray()
        jpeg_buffer.clear()

        if image is not None:
            cv2.imshow("ESP32-CAM Stream", image)  # 화면에 출력
            if cv2.waitKey(1) & 0xFF == ord("q"):  # 'q' 키를 누르면 종료
                break
    #---UdpVideoStreaming---#



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

sock.close()
cv2.destroyAllWindows()