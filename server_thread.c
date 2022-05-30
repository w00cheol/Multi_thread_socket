// 표준 입출력과 소켓 통신, 스레드를 사용하기 위해 헤더 파일을 가져옵니다. 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// 버퍼의 크기와 최대 클라이언트의 수, 이름의 최대 크기를 지정합니다.
#define BUF_SIZE 100
#define MAX_CLNT 256
#define NAME_SIZE 20

void * server(void * arg);
void error_handling(char * msg);

// 연결 소켓을 저장할 배열을 선언합니다.
// 서버 스레드 간 자원을 공유하기 위한 배열을 선언합니다.
int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
int room[MAX_CLNT/2][2];
int share[MAX_CLNT/2][2];

// 자원의 공유를 제한하기 위해 mutex를 선언합니다.
pthread_mutex_t mutx;
FILE *fp;

// 시작 시에 포트번호를 인자로 받습니다.
int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_addr, clnt_addr;
	int clnt_addr_size;
	pthread_t t_id;

	//만약 포트번호를 입력하지 않았다면 예외처리합니다.
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	// 스레드 뮤텍스를 초기화합니다.
	// 뮤텍스는 스레드 간 자원의 접근 방지를 위해 사용됩니다.
	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

	// 서버의 주소와 포트번호를 설정합니다.
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET; 
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));

	// 클라이언트의 응답을 기다립니다.
	if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
   
	while(1)
	{
		// 클라이이언트와의 소켓 통신을 기다립니다.
		clnt_addr_size=sizeof(clnt_addr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr,&clnt_addr_size);

		// 클라이언트의 소켓을 담을 배열에 대입하여 저장합니다.
		// 이 기능을 수행하는 동안의 다른 스레드의 자원 접근을 방지합니다.
		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++]=clnt_sock;
		pthread_mutex_unlock(&mutx);

		// 스레드를 생성하고 해당 소켓과 함께 수행할 함수를 전달합니다.
		pthread_create(&t_id, NULL, server, (void*)&clnt_sock);
		pthread_detach(t_id);

		// 클라이언트의 IP 주소를 출력합니다.
		printf("Connected client IP: %s\n", inet_ntoa(clnt_addr.sin_addr));
	}
	close(serv_sock);
	return 0;
}

// 서버 스레드가 수행할 함수입니다.
void * server(void * arg)
{

	// 파일 입출력을 위해 바탕화면의 server.txt를 가리키도록 설정합니다.
	// 파일 접근의 목적은 이어쓰기입니다.
	// 만약 해당 파일이 없다면, 해당 위치에 파일을 생성합니다.
	fp = fopen("server.txt", "a");
	if (fp == NULL) {
		perror("File open error: ");
		exit(0);
	}

	int clnt_sock=*((int*)arg);
	int str_len=0, i;
   	char msg[BUF_SIZE];
   	char name[NAME_SIZE] = "anony";
   	int menu;

	// 클라이언트 소켓으로부터 메시지를 전달받습니다.
	// 이 메시지는 메뉴를 뜻하는 정수형 변수입니다. 메시지를 menu에 저장합니다. 
   	while((str_len=read(clnt_sock, &menu, sizeof(menu)))!=0)
	{
		// 클라이언트로부터 사용자의 이름을 전달받습니다.
		recv(clnt_sock, name, sizeof(name), 0);

		// menu가 1이라면 클라이언트가 방 생성을 선택한 것입니다.
		// 클라이언트가 사용하고 있지 않는 방 중 최솟값을 클라이언트에게 전송해 줍니다.
		if(menu==1) {
			int check=0, find;
			for(find=0;find<MAX_CLNT/2&&!check;find++) {
				if(room[find][0]==NULL) {
					room[find][0] = clnt_sock;
					send(clnt_sock, &find, sizeof(find), 0);
					fprintf(fp, "Notice: %s create room number %d\n", name, find+1);
					check++;
					
					// 이 클라이언트를 host라고 표현합니다.
					int host_say = 0, guest_say = 0;

					// 클라이언트로부터 게임 종료 코드인 ‘-3941’을 받을 때까지 클라이언트로부터 메시지를 전달받습니다.
					while(host_say!=-3941&&guest_say!=-3941) {
						recv(clnt_sock, &host_say, sizeof(host_say), 0);
						
						// 상대방 클라이언트와 연결 중인 서버 스레드와 자원을 공유하기 위해 share 배열에 클라이언트로부터 받은 메시지를 저장합니다.
						share[find][0] = host_say;

						if(host_say==-3941)
							break;

						// 서버 스레드 간 공유된 자원(share)을 이용하여 자신과 연결중인 클라이언트에게 전달합니다.
						while(share[find][1]==NULL) {
							sleep(0.1);
						}
						guest_say = share[find][1];
						share[find][1]=NULL;

						if(guest_say!=-3941) {
							send(clnt_sock, &guest_say, sizeof(guest_say), 0);
						}
						else {
							recv(clnt_sock, &host_say, sizeof(host_say), 0);
						}
					}

					// 게임이 종료됐다면 공유되었던 공간을 NULL 처리해줍니다.
					sleep(2);
					share[find][0]=NULL;
					share[find][1]=NULL;
					room[find][0]=NULL;
					room[find][1]=NULL;

					// 게임이 종료됐음을 server.txt 파일에 해당 내용을 기입합니다.
					fprintf(fp, "Notice: room number %d is  destroyed\n", find+1);
					break;
				}
			}

			// 동시 운영되는 방의 개수가 최대제한에 도달했다면 666 코드로 예외처리합니다.
			if(check==0) {
				check==666;
				send(clnt_sock, &check, sizeof(check), 0);
			}
		}
		// 클라이언트가 방 입장을 선택했다면, 이 클라이언트를 guest라고 표현합니다.
		else if(menu==2) {
			int find_room_number, fail=666;

			// 클라이언트로부터 방 번호를 전달받습니다.
			recv(clnt_sock, &find_room_number, sizeof(find_room_number), 0);

			// 클라이언트가 원하는 번호의 방이 존재하는지 체크합니다.
			// 만약 해당 방에서 대기 중인 host가 있다면 게임을 매칭시킵니다.
			if(find_room_number>0&&find_room_number<=MAX_CLNT/2&&room[find_room_number-1][0]!=NULL&&room[find_room_number-1][1]==NULL) {
				room[find_room_number-1][1] = clnt_sock;
				send(clnt_sock, &find_room_number, sizeof(find_room_number), 0);
				fprintf(fp, "Notice: %s join room number %d\n", name, find_room_number);

				send(room[find_room_number-1][0], &menu, sizeof(menu), 0);
				send(room[find_room_number-1][1], &menu, sizeof(menu), 0);
				
				fprintf(fp, "Notice: room number %d game start.\n", find_room_number);
				int host_say = 0, guest_say = 0;
				while(host_say!=-3941&&guest_say!=-3941) {
					while(share[find_room_number-1][0]==NULL) {
						sleep(0.1);
					}
					host_say = share[find_room_number-1][0];
					share[find_room_number-1][0]=NULL;

					if(host_say!=-3941) {
						send(clnt_sock, &host_say, sizeof(host_say), 0);
						recv(clnt_sock, &guest_say, sizeof(guest_say), 0);
						share[find_room_number-1][1] = guest_say;
					}
					else {
						recv(clnt_sock, &guest_say, sizeof(guest_say), 0);
					}
				}
			}
			// 해당 방이 존재하지 않거나, 이미 게임이 진행되고 있는 방이라면, 예외처리합니다.
			else {
				send(clnt_sock, &fail, sizeof(fail), 0);
			}
		}
	}

	// 클라이언트가 퇴장한다면 clnt_sock를 확인하여 이후부터의 소켓들을 왼쪽 배열로 당겨 저장합니다.
	// 이 기능을 수행하는동안 다른 스레드들의 자원 접근을 방지하기 위해 뮤텍스를 이용합니다.
	pthread_mutex_lock(&mutx);
   	for(i=0; i<clnt_cnt; i++) {
		if(clnt_sock==clnt_socks[i]) {
			printf("%s exit.\n", name);
			fprintf(fp, "%s exit.\n", name);
			for(;i<clnt_cnt;i++) {
				clnt_socks[i]=clnt_socks[i+1];
			}

			break;
   		}
	}

	// 배열 내에서 삭제가 끝났으니 클라이언트의 숫자인 clnt_cnt를 1 감소시키고, 뮤텍스를 unlock합니다.
   	clnt_cnt--;
   	pthread_mutex_unlock(&mutx);

	// 해당 클라이언트 소켓과의 연결을 해제합니다.
   	close(clnt_sock);

	// 다른 스레드들이 남아있다면, 즉 사용자가 서버에 접속 중이라면 파일을 닫지 않고 함수를 종료합니다.
	for(i=0; i<clnt_cnt; i++) {
		if (clnt_socks[i]){
   			return NULL;
		}
	}

	// 자신이 마지막 스레드인 경우 파일 연결을 해제하고 함수를 종료합니다.
	fclose(fp);
	return NULL;
}

// 에러가 발생할 때 에러를 출력할 함수입니다.
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
