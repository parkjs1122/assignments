import socket, threading, sys
from operator import eq

DIVIDER = '#'
userID = ''

serverName = input('Enter server IP : ')
serverPort = int(input('Enter server Port : '))

try:
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn.connect((serverName, serverPort))
except:
    print('네트워크 오류가 발생하였습니다.')
    sys.exit(1)

# 메시지 수신
def receive_msg(conn):
    try:
        while True:
            serverMsg = conn.recv(1024).decode()
            if eq(serverMsg, '102'):
                print('채팅방 접속이 해제되었습니다.')
                conn.close()
                sys.exit(1)
            else:
                print(serverMsg)
    except Exception as e:
        print('접속 오류가 발생하였습니다.')

# 메시지 송신
def send_msg(conn):
    try:
        while True:
            sendMsg = input()
            if eq(sendMsg[0:2], '/m'):
                sendMsgArr = sendMsg.split(' ')
                orgMsg = ''
                for i in range(2, len(sendMsgArr)):
                    orgMsg += sendMsgArr[i] + ' '
                sendMsg = '003' + DIVIDER + userID + DIVIDER + sendMsgArr[1] + DIVIDER + orgMsg
                conn.send(sendMsg.encode())
            elif eq(sendMsg[0:2], '/b'):
                sendMsgArr = sendMsg.split(' ')
                orgMsg = ''
                for i in range(1, len(sendMsgArr)):
                    orgMsg += sendMsgArr[i] + ' '
                sendMsg = '002' + DIVIDER + userID + DIVIDER + orgMsg
                conn.send(sendMsg.encode())
            elif eq(sendMsg[0:2], '/q'):
                sendMsgArr = sendMsg.split(' ')
                sendMsg = '004' + DIVIDER + userID
                conn.send(sendMsg.encode())
            elif eq(sendMsg[0:4], '/who'):
                sendMsgArr = sendMsg.split(' ')
                sendMsg = '005' + DIVIDER + userID
                conn.send(sendMsg.encode())
            else:
                print('메시지를 정상적으로 입력해주세요.')
    except Exception as e:
        print('접속 오류가 발생하였습니다.')


# 사용자 아이디를 입력받음, 중복된 아이디일 경우 다시 입력 받음.
while True:
    userID = input('사용자 ID : ')
    sendMsg = '001' + DIVIDER + userID
    conn.send(sendMsg.encode())
    serverMsg = conn.recv(1024).decode()
    if not eq(serverMsg, '101'):
        print(serverMsg)
        break
    else:
        print('중복된 ID입니다. ID를 다시 입력해주세요.')

# 메시지 수신 쓰레드 생성
recv = threading.Thread(target=receive_msg, args=(conn,))
recv.start()

# 메시지 송신 쓰레드 생성
send = threading.Thread(target=send_msg, args=(conn,))
send.start()
