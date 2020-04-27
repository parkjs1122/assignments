from socket import *
from operator import eq
import time, random

bindIP = 'localhost'
rcvPort = 12000
sendPort = 12001

rcvSocket = socket(AF_INET, SOCK_DGRAM)
rcvSocket.bind( (bindIP, rcvPort) )

timeCount = 1 # 시작 시간 저장 여부 확인

# 확률 입력
probability = -1
while not (probability >= 0 and probability <= 1):
    probability = float(input('packet loss probability: '))
print()

# Buffer Size 입력 및 설정
bufferSize = int(input('socket recv buffer size: '))
if bufferSize < 10000000:
    bufferSize = 10000000
    print('socket recv buffer size updated: %d' % bufferSize)
rcvSocket.setsockopt(SOL_SOCKET, SO_RCVBUF, bufferSize)

# 확률 처리 - 1~100 랜덤 정수 생성하여 해당 범위안에 들면 Loss
probability = int(probability * 100)

lastAck = -1 # 마지막 송신 ACK 번호

print('The server is ready to receive')

# ACK 송신
def send_msg(pktNum):
    global lastAck
    
    try:
        sendMsg = 'ack#' + str(pktNum) # 프로토콜 : ack#ACK 번호
        rcvSocket.sendto( sendMsg.encode(), (bindIP, sendPort) )
        print('\t%.3f ACK: %d Receiver > Sender' % ((time.time() - startTime), pktNum))

    except Exception as e:
        print(e)
        print('접속 오류가 발생하였습니다.')

print()

# 패킷 수신
while True:
    recvMsg, rcvAddress = rcvSocket.recvfrom(2048)

    if timeCount == 1: # 시작 시간 초기화
        startTime = time.time()
        timeCount = 0
        
    recvMsg = recvMsg.decode().split('#')
    if eq(recvMsg[0], 'pkt'):
        chkTime = time.time()
        if eq(recvMsg[2], '0'): print('\t%.3f pkt: %d Receiver < Sender' % ((chkTime - startTime), int(recvMsg[1])))
        if random.randint(1, 100) > probability: # 패킷 정상 수신
            chkTime = time.time()
            if eq(recvMsg[1], str(lastAck+1)): lastAck += 1 # 정상 패킷일 경우 마지막 송신 ACK 번호 단조 증가
            if eq(recvMsg[2], '1'): # 마지막 패킷 여부 체크 후 마지막 송신 ACK 번호 초기화
                lastAck = -1
            else:
                send_msg(lastAck)
        else: # 패킷 Drop
            print('\t%.3f pkt: %d | Dropped' % ((chkTime - startTime), int(recvMsg[1])))

