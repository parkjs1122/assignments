from socket import *
from operator import eq
import time, threading, sys

bindIP = 'localhost'
sendPort = 12000
rcvPort = 12001

senderSocket = socket(AF_INET, SOCK_DGRAM)
senderSocket.bind( (bindIP, rcvPort) )

windowSize = int(input('window size: '))
timeout = int(input('timeout(sec): '))
count = int(input('how many packets to send?: '))
startYN = input('Do you want to start?(y/n): ')

lastAck = -1 # 마지막 ACK 된 패킷  번호
sendTime = [0] * (count + 10) # 송신 시간 저장
status = [0] * (count + 10) # 0: 비송신 / 1: 송신완료 / 2: 재송신 해야함 / 3: ACK 완료
startTime = 0 # 시작 시간
sending = 0 # 동시 송신을 막기 위한 뮤텍스
reSending = 0 # 패캣 재전송 중에 패킷 수신을 막기 위한 뮤텍스

# 메시지 송신
def send_msg():
    for i in range(1, windowSize+1):
        if (lastAck + i) < count and status[lastAck + i] != 1:
            sendMsg = 'pkt#' + str(lastAck + i) + '#0' # 프로토콜 : pkt#패킷 번호#마지막 패킷 여부
            senderSocket.sendto( sendMsg.encode(), (bindIP, sendPort) )
            chkTime = time.time() - startTime
            sendTime[lastAck + i] = chkTime # 패킷 송신 시간 저장
            if status[lastAck + i] != 2: # 재송신이 아닐 경우
                print('\t%.3f pkt: %d Sender > Receiver\n' % (chkTime, lastAck + i), end='')
            else: # 재송신일 경우
                print('\t%.3f pkt: %d Sender > Receiver (retransmitted)\n' % (chkTime, lastAck + i), end='')
            status[lastAck + i] = 1 # 송신완료로 표시

# 패킷 Timeout 체크 및 재송신
def timeout_chk():
    global lastAck, sending, reSending
    while lastAck < count:
        chkTime = time.time() - startTime
        if lastAck+1 < count:
            if status[lastAck+1] == 1 and (chkTime - sendTime[lastAck+1]) >= timeout:
                # 뮤텍스 세팅
                reSending = 1
                print('\n\t%.3f pkt: %d | Timeout since %.3f\n\n' % (chkTime, lastAck+1, sendTime[lastAck+1]), end='')
                for i in range(1, windowSize + 1):
                    if lastAck+i < count and status[lastAck+i] == 1:
                        status[lastAck+i] = 2 # 재송신 해야하는 패킷으로 표시

                while sending == 1: # recv_msg 함수에서 패킷 송신 중일 때 진입 차단
                    continue
                # 뮤텍스 세팅 후 패킷 송신
                sending = 1
                send_msg()
                sending = 0
                reSending = 0
        time.sleep(0.01)

# ACK 수신
def recv_msg():
    global lastAck, sending, reSending
    while lastAck < count:
        recvMsg, rcvAddress = senderSocket.recvfrom(2048)
        recvMsg = recvMsg.decode().split('#')
        while reSending == 1: # 패킷 Timeout 체크 및 재송신 중일 때 진입 차단
            continue
        chkTime = time.time() - startTime
        print('\t%.3f ACK: %s Sender < Receiver\n' % (chkTime, recvMsg[1]), end='')
        if eq(recvMsg[0], 'ack') and eq(recvMsg[1], str(lastAck+1)) and lastAck+1 < count: # 정상 ACK 일 경우
            status[lastAck+1] = 3 # ACK 완료 패킷으로 표시
            lastAck += 1 # 정상 ACK 일 경우 마지막 수신 ACK 번호 단조 증가
            if(lastAck == count - 1): # 최종 결과 표시
                print('\n\t%.3f | %d packet transmission completed. Throughput: %.2f pkts/sec \n' % (chkTime, count, count/chkTime), end='')
                sendMsg = 'pkt#' + str(lastAck) + '#1'
                senderSocket.sendto( sendMsg.encode(), (bindIP, sendPort) ) # 마지막 패킷까지 ACK 되었음을 송신
                exit()
            while sending == 1: # timeout_chk 함수에서 패킷 송신 중일 때 진입 차단
                continue
            # 뮤텍스 세팅 후 패킷 송신
            sending = 1
            send_msg()
            sending = 0

if not eq(startYN, 'y'):
    exit()

print()

# 시작 시간 초기화
startTime = time.time()
send_msg()

# Timeout 체크 쓰레드 생성
tchk = threading.Thread(target=timeout_chk)
tchk.start()

# 수신 쓰레드 생성
recv = threading.Thread(target=recv_msg)
recv.start()
