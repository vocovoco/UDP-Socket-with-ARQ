/*
    작성자: vocovoco
    컴파일 명령어: gcc sw_client.c -o sw_client
    실행 방법: ./sw_client <IP Address> <PORT #> <File Name>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
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

int main(int argc, char* argv[]){
    int sock;
    Data packet;
    Acknowledge* ack = (Acknowledge*)malloc(sizeof(Acknowledge));
    struct sockaddr_in serv_adr;
    if(argc != 4){
        printf("Usage : %s <IP> <PORT> <File Name>\n", argv[0]);
        exit(1);
    }

    // Socket setup
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    // Setup for receiver timeout
    fd_set read, temp;
    struct timeval timeout;
    FD_ZERO(&read);
    FD_SET(sock, &read);

    // File open
    FILE* fp;
    fp = fopen(argv[3], "rb");
    if(fp == NULL){
        printf("Can't open file.\n");
        exit(1);
    }

    // Build file name packet 
    packet.header = FILE_NAME;
    strcpy(packet.payload, argv[3]);

    // Transfer file name
    int ret_count = 0;
    while(1){
        // Timeout setting
        int result;
        temp = read;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        // Retransmit 5 times. After than terminate program
        if(ret_count < 5){
            sendto(sock, (Data*)&packet, sizeof(Data), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
            ret_count++;
        }else{
            printf("The network is too congested.\n");
            exit(1);
        }

        // Wait 0.5 seconds 
        result = select(sock + 1, &temp, 0, 0, &timeout);
        if(result > 0){
            int serv_adr_size = sizeof(serv_adr);
            recvfrom(sock, ack, sizeof(Acknowledge), 0, (struct sockaddr*)&serv_adr, &serv_adr_size);
            if(ack->header == CONFIRM_FILE_NAME){
                // Well connected
                break;
            }
        }
    }
    
    // Transfer file data
    int seq = 0;
    int flag = 1;
    while(flag){
        // Build file data packet
        if((packet.payload_size = fread(packet.payload, 1, PAYLOAD_SIZE, fp)) > 0){
            packet.header = FILE_DATA;
            packet.seq = seq;
        }else{
            packet.header = FIN;
            packet.seq = seq;
        }

        while(1){
            // Timeout setting
            int result;
            temp = read;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;

            sendto(sock, (Data*)&packet, sizeof(Data), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

            // Wait 1 seconds 
            result = select(sock + 1, &temp, 0, 0, &timeout);
            if(result > 0){
                int serv_adr_size = sizeof(serv_adr);
                recvfrom(sock, ack, sizeof(Acknowledge), 0, (struct sockaddr*)&serv_adr, &serv_adr_size);
                if(ack->header == CONFIRM_FILE_DATA && ack->seq == seq){
                    // Well received
                    seq = (seq + 1) % 2;
                    break;
                }else if(ack->header == FIN){
                    // Ending condition
                    flag = 0;
                    break;
                }
            }
        }
    }

    fclose(fp);
    close(sock);
    return 0;
}