#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>

#define BUFFER_SIZE 102400
#define PORT 80
int main() {
    char input[BUFFER_SIZE];
    printf("Please enter the URL:\n");
    scanf("%s",input);
    printf("========== Socket ==========\n");
    char host[BUFFER_SIZE];
    int hostnum;
    char pathname[BUFFER_SIZE]="/";
    int pathnum=0;
    int horp=0;
    char ip[BUFFER_SIZE];
    for(int i=0;i<strlen(input);i++){
        if(horp==0&&input[i]=='/'){
            hostnum=i;
            horp=1;
            pathnum++;
        }
        else if(horp==1){
            pathname[pathnum]=input[i];
            pathnum++;
        }
        else if(horp==0) host[i]=input[i];
    }
    int sockfd;
    struct addrinfo *addip = NULL;
    int err = getaddrinfo(host,NULL,NULL, &addip);
    if (err != 0) {
        fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }
    if (addip->ai_family == AF_INET) {
        struct sockaddr_in *psai = (struct sockaddr_in*)addip->ai_addr;
        if (inet_ntop(addip->ai_family, &(psai->sin_addr), ip, INET_ADDRSTRLEN) != NULL) {
        }
    } else if (addip->ai_family == AF_INET6) {
    struct sockaddr_in6 *psai = (struct sockaddr_in6*)addip->ai_addr;
    if (inet_ntop(addip->ai_family, &(psai->sin6_addr), ip, INET6_ADDRSTRLEN) != NULL) {
    }
    }

    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);
    char message[BUFFER_SIZE] = "GET ";
    unsigned char buffer[BUFFER_SIZE] = {'\0'};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(PORT);
    strcat(message,pathname);
    strcat(message," HTTP/1.1\r\n");
    strcat(message,"Host: ");
    strcat(message,host);
    strcat(message,"\r\nConnection: close\r\n\r\n");
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket()");
    exit(EXIT_FAILURE);
    }
    if (connect(sockfd, (struct sockaddr *)&server_addr, addrlen) == -1) {
    perror("connect()");
    fprintf(stderr, "Please start the server first\n");
    exit(EXIT_FAILURE);
    }
    send(sockfd, message, strlen(message), 0);
    printf("Sending HTTP request\n");
    // Consider using recv() in a loop for large data to ensure complete message reception
    if(recv(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL)!=0){
        printf("Receving the response\n");
    }
    else return 0;
    printf("======== Hyperlinks ========\n");
    int num=0;
    for(int i=0;i<strlen(buffer);i++){
        int t=0;
        if(buffer[i]=='<'&&buffer[i+1]=='a'){
            int j=0;
            char tmp[BUFFER_SIZE]="\0";
            while(buffer[i]!='>'){
                i++;
                if(buffer[i]=='h'&&buffer[i+1]=='r'&&buffer[i+2]=='e'&&buffer[i+3]=='f'&&buffer[i+4]=='='&&buffer[i+5]=='"'){
                    i+=6;
                    num++;
                    if(buffer[i]!='"')t=1;
                    while(buffer[i]!='"'){
                        tmp[j]=buffer[i];
                        j++;
                        i++;
                    }
                }
            }
            if(t==1){
                printf("%s\n",tmp);
            }
        }
    }
    //printf("%s\n", buffer);
    printf("============================\n");
    printf("We have found %d hyperlinks\n",num);
    close(sockfd);
    
    return 0;
}