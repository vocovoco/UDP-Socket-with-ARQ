/*
	작성자: vocovoco
	컴파일 명령어: gcc sw_server.c -o sw_server
	실행 방법: ./sw_server <PORT #>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024 * 50
#define PAYLOAD_SIZE BUF_SIZE - 3 * sizeof(int)
#define FILE_NAME 1001
#define FILE_DATA 1002
#define FIN 1003
#define CONFIRM_FILE_NAME 2001
#define CONFIRM_FILE_DATA 2002

typedef struct _data{
    int header;
    int seq;
    int payload_size;
    char payload[PAYLOAD_SIZE];
}Data;

typedef struct _acknowledge{
    int header;
    int seq;
}Acknowledge;

int main(int argc, char *argv[]){
	int sock;
	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval start_point, end_point;
	double total_time;
	Data* packet = (Data*)malloc(sizeof(Data));
	Acknowledge ack;

	if(argc != 2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	// Socket setup
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if(bind(sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1){
		printf("bind() error\n");
		exit(1);
	}

	int name_flag = 0;
	int fin_flag = 1;
	int loop_flag = 1;
	int seq = 0;
	int strlen = 0;
	FILE* fp;
   	while(loop_flag){
      	int clnt_adr_size = sizeof(clnt_adr);
      	recvfrom(sock, packet, sizeof(Data), 0 , (struct sockaddr*)&clnt_adr, &clnt_adr_size);

      	switch(packet->header){
      		case FILE_NAME:{
      			ack.header = CONFIRM_FILE_NAME;
		      	ack.seq = 0;
		      	sendto(sock, (Acknowledge*)&ack, sizeof(Acknowledge), 0, (struct sockaddr*)&clnt_adr, sizeof(clnt_adr));
		      	if(!name_flag){
		      		printf("Initiates a file(%s) transfer.\n", packet->payload);
					gettimeofday(&start_point, NULL);

		      		fp = fopen(packet->payload,"wb");
		      		if(fp == NULL){
		      			printf("Fail to create file.\n");
						exit(1);
		      		}
		      		name_flag = 1;
		      		fin_flag = 0;
		      	}
		      	break;
      		}
      		case FILE_DATA:{
      			if(packet->seq == seq && !fin_flag){
      				fwrite(packet->payload, packet->payload_size, 1, fp);
      				ack.header = CONFIRM_FILE_DATA;
			      	ack.seq = seq;
			      	seq = (seq + 1) % 2;
      			}
      			sendto(sock, (Acknowledge*)&ack, sizeof(Acknowledge), 0, (struct sockaddr*)&clnt_adr, sizeof(clnt_adr));
      			break;
      		}
      		case FIN: {
      			ack.header = FIN;
			    ack.seq = 0;
      			sendto(sock, (Acknowledge*)&ack, sizeof(Acknowledge), 0, (struct sockaddr*)&clnt_adr, sizeof(clnt_adr));
		      	if(!fin_flag){
		      		name_flag = 0;
      				fin_flag = 1;
		      		seq = 0;
		      		fclose(fp);

			      	gettimeofday(&end_point, NULL);
			      	total_time = (double)(end_point.tv_sec)+(double)(end_point.tv_usec)/1000000.0-(double)(start_point.tv_sec)-(double)(start_point.tv_usec)/1000000.0;
			      	printf("Shut down the file transfer.\n");
			      	printf("Total time : %f sec\n", total_time);
		      	}
      			break;
      		}	
      	}
   	}
   	close(sock);
	return 0;
}