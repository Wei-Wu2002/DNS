// 2023.5.20 v1，目标：实现wireshark可成功解析的DNS报文
#include <stdio.h>      /*for printf()*/
#include <sys/socket.h> /*for socket(), sendto() and recvfrom()*/
#include <netinet/in.h>
#include "dns_protocol.h"
#include <arpa/inet.h> /*for sockaddr_in and inet_addr()*/
#include <stdlib.h>    /*for atoi() and exit()*/
#include <string.h>    /*for memset() and strlen()*/
#include <unistd.h>    /*for close()*/
#define ECHOMAX 255    /*Longest string to echo*/
#define SERVER_PORT = 53; /*port number */

int main(int argc, char *agrv) {
	
    // 定义UDP结构体（参考lab4）
    
    int sock; /*socket descriptor*/
    struct sockaddr_in serverAddress;
    char *serverIpAddress;
    int StringLen;
    
    /*get IP address and Data*/
    
    if (argc < 2)
    {
        printf("User: %s <Server IP> <data> ... <data>\n", argv[0]);
        exit(1);
    }
    
    /*save IP address and Data*/
    
    serverIpAddress = argv[1];
    
    /*create a datagram | UDP socket*/
    
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("socket() failed.\n");
        exit(1);
    }
    /*construct the server address structure*/
    
    memset(&serverAddress, 0, sizeof(serverAddress));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(serverIpAddress);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0)
	{
        perror("Invalid server address");
        exit(1);
    }
    
   	
	//报文相关
   	
   	
    // 向DNS服务器发送报文请求     
    
    //send test(lab 4)
    int i = 2;
    while (argv[i] != NULL)
    {
        StringLen = strlen(argv[i]);
        if (StringLen > ECHOMAX)
        {
            printf("The message %s exit the length, we will ignore it\n", argv[i]);
            continue;
        }
        else
        {
            if ((sendto(sock, argv[i], StringLen, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) != StringLen)
            {
                printf("sendto() sent a different number of bytes than expected.\n");
            }
            else {
                printf("%s successfully send data to %s : %s\n", argv[0], serverIpAddress, argv[i]);
            }
        }
        i++;
    }
    
    //close the socket
    
    close(sock);
    exit(0);

}