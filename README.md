멀티 스레드를 이용한 TCP/IP 소켓 통신 서버 구현
---

1. 서버는 클라이언트가 접속을 요청할 때마다 새로운 스레드를 생성합니다.
2. 클라이언트는 서버에 접속하여 선택지를 얻습니다 (방 생성, 방 입장, 게임 종료)
3. 방을 생성한 클라이언트는, 새로운 클라이언트의 입장을 기다립니다.
4. 또다른 클라이언트가 방에 입장하면 게임이 시작됩니다.
5. 게임이 종료된 후에는 로비로 돌아갑니다.



---
#### 적용기술
---
- VMware (Ubuntu 18.04 LTS)
- Socket
- Multi thread
- Mutex
- File

---
#### 실행방법
---


1. Linux 기반 OS에서 시작합니다. (이 프로그램은 Ubuntu 18.04 LTS 에서 작성되었습니다.)
2. 이 레포지토리 폴더 안에서 터미널을 실행합니다.
3. 서버를 구동시킵니다.

<pre> ./server <포트번호> </pre>


4. 새로운 터미널 내에서 서버에 입장합니다.
<pre> ./client <서버 IP(=127.0.0.1)> <포트번호> <닉네임></pre>


5. <strong>만약 프로그램이 정상적으로 작동하지 않는다면, 아래 명령어를 실행 한 후 3, 4번을 재시도합니다.</strong>

 <pre>
 make clean

 make server

 make client
</pre>





---
#### 초기 구성
---
![](https://user-images.githubusercontent.com/53927414/170985821-cd135ac5-15bb-4976-891e-81b7705b21ea.png)

* 서버를 열고 4명의 클라이언트가 접속합니다.


---
#### 프로세스 확인 (pstree 명령어)
---
![](https://user-images.githubusercontent.com/53927414/170966796-ef59223d-22a6-4b84-b672-f9178aa8e714.png)

* pstree 명령어로 프로세스 및 스레드를 확인합니다.

* 현재 사용자가 4명이므로 4개의 스레드를 가지는 서버 프로세스 thread_s 1개가 존재합니다.

* 1개의 스레드를 가지는 클라이언트 프로세스 thread_c 4개가 존재합니다.



---
#### 게임 구동
---
![](https://user-images.githubusercontent.com/53927414/170966815-75fffc70-9c1d-46a1-bd7d-1f5d88a6d830.png)

* 게임 상황입니다.

* woo와 alice가 1번 방에서 게임을 시작했습니다.

* alice의 패가 woo의 패보다 높아 게임을 승리했고 금액을 얻게 됩니다. 



---
#### 예외 처리
---
![](https://user-images.githubusercontent.com/53927414/170966902-e4bda9b6-e4d0-473d-ba08-4dda6b3bb194.png)

* 이번에는 bob이 게임 방 생성을 요청합니다.

* 1번 방은 기존에 사용되었었지만 이미 클라이언트가 떠나고 비어있는 방이므로 또 다시 1번 방을 배정받습니다.

* woo는 1번 방에 입장합니다.

* 게임이 시작됐고 bob이 기권하였습니다.

* 양 선수가 베팅하지 않아 잔액은 미동입니다.



---
#### 서버 단 출력
---
![](https://user-images.githubusercontent.com/53927414/170967011-a8e351ad-9b28-4af1-854d-a06bf04e195a.png)

* woo, bob, alice, trudy 모두가 게임을 종료한 상황입니다.

* 서버에 문구가 출력됩니다.




#### 파일 입출력 및 저장
![](https://user-images.githubusercontent.com/53927414/170967091-2122f36e-d60a-4b8e-add1-045edc02ad54.png)

* server.txt 입니다.

* 각각 서버 스레드가 주요 내역을 적어 넣습니다.
