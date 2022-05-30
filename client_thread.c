// 표준 입출력과 소켓 통신, 스레드를 사용하기 위해 헤더 파일을 가져옵니다. 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

// 버퍼의 크기와 이름의 최대 크기를 지정합니다.
#define BUF_SIZE 100
#define NAME_SIZE 20

void * play(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
   
// 시작 시 접속할 서버의 IP 주소, 포트 번호, 사용자 이름을 인자로 받습니다.
int main(int argc, char *argv[])
{
	srand((unsigned int)time(NULL));

	int sock;
	struct sockaddr_in serv_addr;
	pthread_t thread_play;
	void * thread_return;

	// 만약 IP주소, 포트번호 또는 사용자 이름을 입력하지 않았다면 예외처리합니다.
	if(argc!=4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "%s", argv[3]);
	sock=socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	// 입력받은 서버의 IP 주소와 포트번호를 이용하여 서버와 소켓 통신합니다.
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) {
		error_handling("connect() error");
	}

	// 새로운 스레드를 생성하면서 스레드가 수행할 함수를 전달합니다.
	pthread_create(&thread_play, NULL, play, (void*)&sock);
	pthread_join(thread_play, &thread_return);

   	close(sock);
   	return 0;
}
   
void * play(void * arg)
{
	int money = 100, finish_code=-3941;
	int serv_sock=*((int*)arg);
	while(1)
	{
		// 게임 서버에 접속하면 로비 화면을 출력합니다.
		int code = 0;
		printf("1. create room\n2. join room\n3. quit\n");
		printf("I'll choose.. ");
		scanf("%d", &code);
		
		// 선택한 메뉴와 사용자의 이름을 통신 중엔 서버 소켓에 전송합니다.
		send(serv_sock, &code, sizeof(code), 0);
		send(serv_sock, name, sizeof(name), 0);

		// 만약 클라이언트가 방 생성을 선택했다면, 서버로부터 메시지를 기다립니다.
		if(code==1) {
			recv(serv_sock, &code, sizeof(code), 0);
			
			// 해당 메시지가 에러 코드 666일 경우, 예외처리합니다.
			if(code==666) {
				printf("\n\tall room is full.\n\n");
			}
			// 서버로부터 받은 메시지는 클라이언트가 사용할 방의 번호입니다.
			else {
				printf("\n\t\troom number %d\n\n", code+1);
				printf("waiting...\n");

				// 상대방의 입장을 기다립니다.
				recv(serv_sock, &code, sizeof(code), 0);
				printf("game start\n");
				printf("your money : %d\n", money);

				// 초기 설정을 위해 자신의 자금과 상대 클라이언트의 자금 내역을 공개합니다.
				int maximum, bet=0, total_bet=0, guest_bet=0,guest_total_bet=0, guest_score, guest_money;
				send(serv_sock, &money, sizeof(money), 0);
				recv(serv_sock, &guest_money, sizeof(guest_money), 0);

				// 나와 상대방 중 더 작은 금액이 이번 판의 최대 베팅 금액입니다.
				if(money>guest_money)
					maximum = guest_money;
				else
					maximum = money;
				printf("maximum bet : %d\n", maximum);

				printf("your card : ");
				int a = rand()%10;
				int b = rand()%10;
				int score = (a+b)%10;
				printf("%d %d\n", a, b);
				printf("so your score is %d.\n", score);

				// 자신의 점수를 출력하고, 상대와 교환합니다.
				send(serv_sock, &score, sizeof(score), 0);
				recv(serv_sock, &guest_score, sizeof(guest_score), 0);
	
				// 게임은 host의 턴부터 시작됩니다.
				// host는 guest와 베팅 금액이 같아질 때까지 베팅하거나 -1 코드로 게임을 포기합니다.
				do {
					printf("\nyour turn.\n\t");
					HOST_BET:
						printf("more bet or die(-1): ");
						scanf("%d", &bet);
						if(bet==-1) {
							score=-1;
							send(serv_sock, &bet, sizeof(bet), 0);
							break;
						}
						// 만약 최대 베팅 금액을 넘을 경우 예외처리합니다.
						else if(total_bet+bet>money) {
							printf("\n\tcan not bet more than maximum.\n\n");
							goto HOST_BET;
						}
						// 만약 게스트보다 적은 베팅으로 턴을 넘기려 한다면 예외처리합니다.
						else if(total_bet+bet<guest_total_bet) {
							printf("your total : %d\n", total_bet);
							printf("guest total : %d\n", guest_total_bet);
							printf("\n\tcan not bet less than oppnent.\n\n");
							goto HOST_BET;
						}
						else {
							total_bet+=bet;
							send(serv_sock, &bet, sizeof(bet), 0);

							// 양 측의 베팅 금액이 같을 경우 게임을 종료합니다.
							if(total_bet==guest_total_bet){
								break;
							}
							// 아직 양 측의 베팅 금액이 같지 않다면 상대의 턴이 끝나기를 기다립니다.
							else {
								printf("wait turn...\n");

								// 상대의 베팅 금액을 전달받습니다.
								recv(serv_sock, &guest_bet, sizeof(guest_bet), 0);

								// 상대가 게임을 포기했다면, 게임을 종료합니다.
								if(guest_bet==-1) {
									score=10;
									break;
								}
								else {
									printf("guest more bet : %d\n", guest_bet);
									guest_total_bet+=guest_bet;
									printf("guest total bet : %d\n", guest_total_bet);
									printf("my total bet : %d\n", total_bet);
								}
							}
						}
				}while(total_bet!=guest_total_bet);

				// 게임을 종료하면 승패를 가립니다.
				// 이겼다면, 상대가 지금까지 건 베팅 금액을 회수합니다.
				if(guest_score<score) {
					printf("you win.\n");
					printf("you get %d\n\n", guest_total_bet);
					money+=guest_total_bet;
					printf("money : %d\n", money);
				}
				// 졌다면, 내가 지금까지 건 베팅 금액을 소모합니다.
				else if(guest_score>score) {
					printf("you lose.\n");
					printf("you lose %d\n\n", total_bet);
					money-=total_bet;
					printf("money : %d\n", money);
				}
				// 비겼다면, 금액에 영향을 주지 않습니다.
				else{
					printf("you draw.\n\n");
					printf("money : %d\n", money);
				}

				// 경기의 종료를 알리기 위한 코드를 서버 소켓에 전송합니다.
				send(serv_sock, &finish_code, sizeof(finish_code), 0);
			}
		}
		// 만약 클라이언트가 방 입장을 선택했다면, 방 번호를 선택합니다.
		else if(code==2)
		{
			int room_number;

			printf("what is the room number? ");
			scanf("%d", &room_number);

			// 방 번호를 서버에 전달하고, 서버로부터 메시지를 기다립니다.
			send(serv_sock, &room_number, sizeof(room_number), 0);
			recv(serv_sock, &room_number, sizeof(room_number), 0);

			// 에러코드 666 예외처리입니다.
			if(room_number==666) {
				printf("\n\tThere is no room number\n\n");
			}
			else {
				int code, maximum, bet=0, total_bet=0, host_bet=0, host_total_bet=0, host_money, host_score;

				// 자신이 선택한 방에 배정됩니다.
				printf("\n\t\troom number %d\n\n", room_number);
				recv(serv_sock, &code, sizeof(code), 0);
				printf("game start\n");
				printf("your money : %d\n", money);

				// 서버 소켓을 통해 상대 클라이언트와 자신의 자금 현황을 교환합니다.
				recv(serv_sock, &host_money, sizeof(host_money), 0);
				send(serv_sock, &money, sizeof(money), 0);

				// 더 작은 금액이 이번 판의 최대 베팅 금액이 됩니다.
				if(money>host_money)
					maximum = host_money;
				else
					maximum = money;
				printf("maximum bet : %d\n", maximum);

				// 자신의 카드와 이번 판 점수를 출력합니다.
				printf("your card : ");
				int c = rand()%10;
				int d = rand()%10;
				int score = (c+d)%10;
				printf("%d %d\n", c, d);

				printf("so your score is %d.\n", score);
				recv(serv_sock, &host_score, sizeof(host_score), 0);
				send(serv_sock, &score, sizeof(score), 0);
	
				// host의 턴부터 시작되기 때문에 상대방의 수를 기다립니다.
				printf("wait turn...\n");
				recv(serv_sock, &host_bet, sizeof(host_bet), 0);
				host_total_bet+=host_bet;

				// host client 코드와 같기 때문에 주석 생략합니다.
				do {
					if(host_bet==-1) {
						score=10;
						if(host_total_bet==-1) host_total_bet=0;
						break;
					}
					printf("host more bet %d\n", host_bet);
					printf("\nyour turn.\n\t");
					GUEST_BET:
						printf("more bet or die(-1): ");
						scanf("%d", &bet);
						if(bet==-1) {
							score=-1;
							send(serv_sock, &score, sizeof(score), 0);
							break;
						}
						else if(total_bet+bet>money) {
							printf("\n\tcan not bet more than maximum.\n\n");
							goto GUEST_BET;
						}
						else if(total_bet+bet<host_total_bet) {
							printf("your total : %d\n", total_bet);
							printf("host total : %d\n", host_total_bet);
							printf("\n\tcan not bet less than oppnent.\n\n");
							goto GUEST_BET;
						}
						else {
							total_bet+=bet;
							send(serv_sock, &bet, sizeof(bet), 0);
							if(total_bet==host_total_bet)
								break;
							else {
								printf("wait turn...\n");
								recv(serv_sock, &host_bet, sizeof(host_bet), 0);
								if(host_bet==-1) {
									score=10;
									break;
								}
								else {
									printf("host more bet : %d\n", host_bet);
									host_total_bet+=host_bet;
									printf("host total bet : %d\n", host_total_bet);
									printf("my total bet : %d\n", total_bet);
								}
							}
						}
				}while(total_bet!=host_total_bet);

				if(host_score<score) {
					printf("you win.\n");
					printf("you get %d\n\n", host_total_bet);
					money+=host_total_bet;
					printf("money : %d\n", money);
				}
				else if(host_score>score) {
					printf("you lose.\n");
					printf("you lose %d\n\n", total_bet);
					money-=total_bet;
					printf("money : %d\n", money);
				}
				else {
					printf("you draw.\n\n");
					printf("money : %d\n", money);
				}

				// 게임이 종료되었다면 서버 소켓에게 게임 종료를 알리는 코드를 전송합니다.
				send(serv_sock, &finish_code, sizeof(finish_code), 0);
			}
		}
		//만약 사용자가 퇴장을 선택할 경우 종료합니다.
		else if(code==3){
			exit(0);
		}

		// 게임이 종료된 후, 5초 소모 후 로비로 돌아갑니다.
		printf("\n\tgo to lobby...\n\n");
		sleep(5);
	}

	return NULL;
}

// 에러가 발생할 때 에러를 출력할 함수입니다.
void error_handling(char *msg)
{
   fputs(msg, stderr);
   fputc('\n', stderr);
   exit(1);
}
