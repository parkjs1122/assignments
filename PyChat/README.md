# 개요
TCP/IP 통신과 사용자 정의 프로토콜을 활용한 채팅 프로그램입니다. 스레드를 이용하여 메시지 송수신을 구현하였습니다.

# 프로토콜

## 서버
### 001#접속자 ID
신규 클라이언트 접속 시 사용자에게 입력 받은 사용자 ID 수신
### 002#발신 접속자 ID#메시지
전체메시지 전달
### 003#발신 접속자 ID#수신 접속자 ID#메시지
특정사용자에게 메시지 전달
### 004#접속자 ID
채팅방 퇴장
### 005#접속자 ID
접속 중인 사용자 리스트 확인

## 클라이언트
### 101
신규 사용자가 중복된 ID를 입력했을 경우 ID 재입력을 위하여 송신
### 102
정상적으로 채팅방에서 퇴장하였을 경우 정상 퇴장 안내를 위하여 송신

# 서버
chatFunc() 메소드가 connection 별로 메시지를 수신하여 처리합니다. 그 내용은 다음과 같습니다.

## 신규 사용자 접속 시 처리
``` py3
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
```
중복된 사용자 일 경우 오류메시지를 송신하고 아닐 경우 사용자를 리스트에 append 합니다.

## 모든 사용자에게 메시지 전달
``` py3
if eq(msgArr[0], '002'):
	for con in conns:
	    if not eq(msgArr[1], con[1]):
		sendMsg = msgArr[1] + ' : ' + msgArr[2]
		try:
		    con[0].send(sendMsg.encode())
		except ConnectionResetError as e:
		    disconnect(con)
```
현재 서버에 연결된 모든 connection으로 메시지를 송신합니다.

## 귓속말
``` py3
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
```
해당하는 connection으로 메시지를 송신합니다.

## 퇴장
``` py3
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
```
사용자를 리스트에서 remove 하고 모든 connection으로 공지를 송신합니다.

## 접속자 확인
``` py3
if eq(msgArr[0], '005'):                
	for con in conns:
	    if eq(msgArr[1], con[1]):
		sendMsg = getUserList()
		try:
		    con[0].send(sendMsg.encode())
		except ConnectionResetError as e:
		    disconnect(con)
```
접속자 리스트를 메시지로 변환하여 송신합니다.

## 예외 발생
``` py3
except:
	for con in conns:
	    if con[0] == conn:
		print(con[1], '사용자가 접속을 해제하였습니다.')
		sendMsgAll(con[1], '[공지]' + con[1] + ' 사용자가 퇴장하였습니다.')
		conns.remove([con[0], con[1]])
```
예외 발생시 강제종료로 간주하여 사용자를 리스트에서 remove하고 모든 connection으로 공지를 송신합니다.

# 클라이언트

## 서버 접속
``` py3
serverName = input('Enter server IP : ')
serverPort = int(input('Enter server Port : '))

try:
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn.connect((serverName, serverPort))
except:
    print('네트워크 오류가 발생하였습니다.')
    sys.exit(1)
```
사용자에게 IP와 포트번호를 받아 TCP 소켓을 생성하여 접속을 시도합니다.

## 메시지 수신 메소드
``` py3
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
```
프로토콜을 체크하여 접속 해제일 경우는 프로그램을 종료합니다.


## 메시지 송신 메소드
``` py3
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
```
사용자가 입력한 명령어를 확인하여 메시지를 해당하는 프로토콜로 변환하여 서버로 송신합니다.