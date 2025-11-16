########################################################################
#불러오는 모듈
import socket
import select

import os

import random as r

import json
import csv

import time
ratsLife = 0
ratList = []
# 클래스
class Common:
    num = 0

    def __init__(self, ip = "", sendPort = "", revPort = ""):
        # print("[소캣설정 시작]")

        Common.num += 1

        self.priority = Common.num
        # print(f"우선순위: {self.priority}")

        if not ip:
            ip = input("ip가 설정되지 않았습니다.ex) 0.0.0.0: ")
        self.ip = ip
        # print(f"ip: {self.ip}")

        if not sendPort:
            sendPort = input("송신포트가 설정되지 않았습니다.ex) 5000: ")
        self.sendPort = int(sendPort)
        # print(f"송신포트: {self.sendPort}")

        if not revPort:
            revPort = input("수신포트가 설정되지 않았습니다.ex) 5001: ")
        self.revPort = int(revPort)
        # print(f"수신포트: {self.revPort}\n")


        # self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # self.socket.bind(("", self.sendPort))

        # worldSockets.append(self.socket)
class Player(Common):
    def __init__(self, name = "", ip = "", sendPort = "", revPort = ""):
        # print("[Player 설정시작]")
        self.type = "Player"
        # print(f"객체 타입: {self.type}")

        if not name:
            name = input("플레이어의 이름이 설정되지 않았습니다.ex) 다오: ")
        self.name = name
        # print(f"플레이어 이름: {self.name}")

        super().__init__(ip, sendPort, revPort)
class Kart(Common):
    def __init__(self, name = "", ip = "", sendPort = "", revPort = ""):
        # print("[Kart 설정시작]")
        self.type = "Kart"
        # print(f"객체 타입: {self.type}")

        if not name:
            name = input("카트의 이름이 설정되지 않았습니다.ex) 코튼: ")
        self.name = name
        # print(f"카트 이름: {self.name}")
        
        self.ahrsStart = 0
        self.ahrs = 0

        self.color = "sky"
        self.colorName = "sky"

        self.leftMotor = 0
        self.rightMotor = 0

        super().__init__(ip, sendPort, revPort)
class Infra(Common):
    def __init__(self, name="", ip="", sendPort="", revPort=""):
        # print("[Infra 작성시작]")
        self.type = "Infra"
        # print(f"객체 타입: {self.type}")

        if not name:
            name = input("인프라의 이름이 설정되지 않았습니다.ex) 신호등1: ")
        self.name = name
        # print(f"인프라 이름: {self.name}")

        super().__init__(ip, sendPort, revPort)
class User:
    def __init__(self,
                 userName = "",
                 nameKart = "", ipKart = "", sendPortKart = "", revPortKart = "",
                 namePlayer = "", ipPlayer = "", sendPortPlayer = "", revPortPlayer = "",
                 role = "jobless", goal = "none"):
        # print("-----------------------------")
        # print("[User 설정시작]")
        self.type = "User"
        self.key = "space"
        # print(f"객체 타입: {self.type}")

        if not userName:
            userName = input("유저 이름이 설정되지 않았습니다.ex) 세이버, 다오")
        self.userName = userName
        self.role = role
        self.goal = goal
        self.life = 1
        # print(f"유저 이름: {self.userName}\n")

        self.Kart = Kart(nameKart, ipKart, sendPortKart, revPortKart)
        self.Player = Player(namePlayer, ipPlayer, sendPortPlayer, revPortPlayer)

        self.drivingRecord = [ [f'userName = {self.userName}', 
                                 f'nameKart = {self.Kart.name}', 
                                 f'ipKart = {self.Kart.ip}', 
                                 f'sendPortKart = {self.Kart.sendPort}', 
                                 f'revPortKart = {self.Kart.revPort}', 
                                 f'namePlayer = {self.Player.name}', 
                                 f'ipPlayer = {self.Player.ip}', 
                                 f'sendPortPlayer = {self.Player.sendPort}', 
                                 f'revPortPlayer = {self.Player.revPort}', 
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


        print(f"<유저[{userName}] 기본 설정 완료>")
        # print("-----------------------------")
########################################################################
# def getUniqueFilename(directory, baseName, extension=".csv"):
#     # 디렉토리 존재 여부 확인 및 생성
#     os.makedirs(directory)

#     fileName = f"{baseName}{extension}"
#     fullPath = os.path.join(directory, fileName)

#     counter = 1
#     while os.path.exists(fullPath):
#         fileName = f"{baseName}_{counter}{extension}"
#         fullPath = os.path.join(directory, fileName)
#         counter += 1

#     return fullPath

def getUniqueFilename(directory, baseName, extension=".csv"):
    """
    지정된 디렉토리에 고유한 파일 이름을 생성합니다.
    """
    # 디렉토리 존재 여부 확인 및 생성
    os.makedirs(directory, exist_ok=True)

    fileName = f"{baseName}{extension}"
    fullPath = os.path.join(directory, fileName)

    counter = 1
    while os.path.exists(fullPath):
        fileName = f"{baseName}_{counter}{extension}"
        fullPath = os.path.join(directory, fileName)
        counter += 1

    return fullPath

def csvFileSave(macron: list):
    """
    사용자의 입력에 따라 데이터를 CSV 파일로 저장합니다.
    """
    for i in range(len(macron)):
        if macron[i].type == "User":
            qual = -1  # 유효하지 않은 값으로 초기화
            while not (0 <= qual <= 2):
                qual_str = input(f"{macron[i].userName}의 데이터 수집 결과 결정 (GOOD:2, NOMAL:1, BAD:0)\n입력: ")
                try:
                    qual = int(qual_str)
                    if not (0 <= qual <= 2):
                        print(f"'{qual_str}'은(는) 유효한 입력이 아닙니다. 0, 1, 2 중 하나를 입력해주세요.")
                except ValueError:
                    print(f"입력하신 자료형은 정수가 아닙니다. (입력값: '{qual_str}')")

            # 선택된 값에 따라 qual 문자열 설정
            if qual == 0:
                qual_str = "[bad]"
            elif qual == 1:
                qual_str = "[nomal]"
            else:
                qual_str = "[good]"
            
            # 파일 저장 경로를 동적으로 설정하여 권한 문제를 방지
            documents_path = os.path.join(os.path.expanduser('~'), 'Documents')
            filePath = os.path.join(documents_path, 'V2I_Kart', 'PC_UNO_Operator', 'data')

            baseName = qual_str + macron[i].userName
            fileName = getUniqueFilename(filePath, baseName)

            with open(fileName, 'w', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                # writer.writerow(...)  # 주석 처리된 헤더 부분은 필요에 따라 사용하세요.
                for row in macron[i].drivingRecord:
                    writer.writerow(row)
                print(f"데이터를 성공적으로 저장했습니다: {fileName}")


###############################################
dao = User(
        userName = "빨강",
        nameKart = "빨강색카트", ipKart = "192.168.3.197", sendPortKart = "7000", revPortKart = "7001",
        namePlayer = "배찌", ipPlayer = "192.168.3.187", sendPortPlayer = "8000", revPortPlayer = "8000",
        role="rat"
    )

dao.drivingRecord = [["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"],
                     ["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"],
                     ["현재구간", "최초 연결시간", "현재시간", "왼쪽 모터상태", "오른쪽 모터상태", "방향변환값","변환된 컬러값",  "LUX", "컬러R", "컬러G", "컬러B", "raw방향값"]
                     ]

macron = [dao]
# csvFileSave(macron)

import numpy as np

print(np.arctan(1)*180/np.pi)
