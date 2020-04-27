import socket, threading, sys
from operator import eq

conns = []

serverPort = int(input('서버 포트 번호 : '))

try:
    serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serverSocket.bind(('127.0.0.1', serverPort))
    serverSocket.listen(10)
    print('서버 구동 완료')
except:
    print('네트워크 오류가 발생하였습니다.')
    sys.exit(1)

def sendMsgAll(sender, sendMsg):
    for con in conns:
        if not eq(sender, con[1]):
            try:
                con[0].send(sendMsg.encode())
            except ConnectionResetError as e:
                conns.remove(con)

def getUserList():
    userList = '=== 접속자 리스트 ===\n'
    for con in conns:
        userList += con[1] + '\n'
    userList += '====================='

    return userList

def disconnect(con):
    print(con[1], '사용자가 접속을 해제하였습니다.')
    conns.remove(con)

def chatFunc(conn):
    try:
        while True:
            msg = conn.recv(1024).decode()
            msgArr = msg.split('#')

            # 신규 사용자 접속, 이미 존재하는 ID일 경우 오류메시지 전달
            if eq(msgArr[0], '001'):
                existYN = 0
                for con in conns:
                    if eq(con[1], msgArr[1]):
                        existYN = 1
                    
                if existYN == 1:
                    conn.send('101'.encode()) # 에러메시지 전달
                else:
                    conns.append([conn, msgArr[1]]) # 사용자 리스트에 추가

                    # 기존 접속자들에게 새로운 사용자 접속 공지
                    sendMsgAll(msgArr[1], '[공지]' + msgArr[1] + ' 사용자가 접속하였습니다.')
                    print(msgArr[1], '사용자가 연결되었습니다.')

                    sendMsg = '채팅방에 입장하였습니다.'
                    conn.send(sendMsg.encode())

                    sendMsg = getUserList()
                    conn.send(sendMsg.encode())

            # 전체에게 메시지 전달
            if eq(msgArr[0], '002'):
                for con in conns:
                    if not eq(msgArr[1], con[1]):
                        sendMsg = msgArr[1] + ' : ' + msgArr[2]
                        try:
                            con[0].send(sendMsg.encode())
                        except ConnectionResetError as e:
                            disconnect(con)

            # 특정 사용자에게 메시지 전달
            if eq(msgArr[0], '003'):
                isOk = 0
                for con in conns:
                    if eq(msgArr[2], con[1]):
                        sendMsg = msgArr[1] + '님의 귓속말 : ' + msgArr[3]
                        try:
                            con[0].send(sendMsg.encode())
                            isOk = 1
                        except ConnectionResetError as e:
                            disconnect(con)
                            
                if isOk == 0:
                    for con in conns:
                        if eq(msgArr[1], con[1]):
                            sendMsg = '해당 사용자가 존재하지 않습니다.'
                            con[0].send(sendMsg.encode())

            # 채팅 나가기
            if eq(msgArr[0], '004'):
                for con in conns:
                    if eq(msgArr[1], con[1]):
                        sendMsg = '102'
                        try:
                            con[0].send(sendMsg.encode())
                            disconnect(con)
                            sendMsgAll(msgArr[1], '[공지]' + msgArr[1] + ' 사용자가 퇴장하였습니다.')
                        except ConnectionResetError as e:
                            disconnect(con)

            # 접속자 리스트 보기
            if eq(msgArr[0], '005'):                
                for con in conns:
                    if eq(msgArr[1], con[1]):
                        sendMsg = getUserList()
                        try:
                            con[0].send(sendMsg.encode())
                        except ConnectionResetError as e:
                            disconnect(con)
                            
    except:
        for con in conns:
            if con[0] == conn:
                print(con[1], '사용자가 접속을 해제하였습니다.')
                sendMsgAll(con[1], '[공지]' + con[1] + ' 사용자가 퇴장하였습니다.')
                conns.remove([con[0], con[1]])

while True:
    conn, addr = serverSocket.accept()
    client = threading.Thread(target=chatFunc, args=(conn,))
    client.start()
