#include "lab.h"
size_t getFileSize(FILE *fd) {
    fseek(fd, 0, SEEK_END);
    size_t size = ftell(fd);
    return size;
}

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *child(){
    size_t filesize = getFileSize(fd);
    totalpackage=filesize/1024;
    if(filesize%1024==0)totalpackage--;
    while(true){
        Packet send;
        memset(&send, 0, sizeof(send));
        if(ack[totalpackage]&&ack[totalpackage-3]&&ack[totalpackage-1]&&ack[totalpackage-2]) break;
        for(int i=0;i<4;i++){
            if(ack[window]==1&&window<totalpackage-3){
                window++;
                break;
            }
            if(time1[window+i]==0){
                pthread_mutex_lock( &mutex1 );
                time1[window+i]= clock() * 1000 / CLOCKS_PER_SEC;
                timeout[(window+i)]=false;
                pthread_mutex_unlock( &mutex1 );
                fseek(fd,(window+i)*1024,SEEK_SET);
                fread(send.data, sizeof(char), 1024, fd);
                if((window+i)*1024+1024<=filesize){
                    send.header.isLast=false;
                    send.header.size=1024;
                    send.header.seq=(window+i);
                    sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&clientInfo, sizeof(struct sockaddr_in));
                    printf("Send SEQ = %u\n", send.header.seq);
                }
                else{
                    send.header.isLast=true;
                    send.header.size=filesize%1024;
                    send.header.seq=(window+i);
                    sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&clientInfo, sizeof(struct sockaddr_in));
                    printf("Send SEQ = %u\n", send.header.seq);
                }
            }
            else if((clock() * 1000 / CLOCKS_PER_SEC)-time1[window+i]>=100&&timeout[(window+i)]&&ack[window+i]==false){
                printf("Timeout! Resend!\n");
                pthread_mutex_lock( &mutex1 );
                time1[window+i]= clock() * 1000 / CLOCKS_PER_SEC;
                timeout[(window+i)]=false;
                pthread_mutex_unlock( &mutex1 );
                fseek(fd,(window+i)*1024,SEEK_SET);
                fread(send.data, sizeof(char), 1024, fd);
                if((window+i)*1024+1024<=filesize){
                    send.header.isLast=false;
                    send.header.size=1024;
                    send.header.seq=(window+i);
                    sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&clientInfo, sizeof(struct sockaddr_in));
                    printf("ReSend SEQ = %u\n", send.header.seq);
                }
                else{
                    send.header.isLast=true;
                    send.header.size=filesize%1024;
                    send.header.seq=(window+i);
                    sendto(sockfd, &send, sizeof(send), 0, (struct sockaddr *)&clientInfo, sizeof(struct sockaddr_in));
                    printf("ReSend SEQ = %u\n", send.header.seq);
                }
            }
        }
    }

}
void* timeoutf(){
    size_t filesize=getFileSize(fd);
    while(true){
        if(ack[totalpackage-3]&&ack[totalpackage]&&ack[totalpackage-1]&&ack[totalpackage-2]) break;
        for(int i=0;i<4;i++){
            if(((clock() * 1000 / CLOCKS_PER_SEC)-time1[window+i])>=100&&ack[window+i]==false){
                pthread_mutex_lock( &mutex1 );
                timeout[window+i]=true;
                pthread_mutex_unlock( &mutex1 );
            }
        }
    }
}
void* recv_ack(){
    size_t filesize=getFileSize(fd);
    while(true){
        if(ack[totalpackage-3]&&ack[totalpackage]&&ack[totalpackage-1]&&ack[totalpackage-2]) break;
        Packet recv;
        memset(&recv, 0, sizeof(recv));
        recvfrom(sockfd, &recv, sizeof(recv), 0, (struct sockaddr *)&clientInfo, (socklen_t *)&addrlen);
        printf("Received ACK = %u\n", recv.header.ack);
        pthread_mutex_lock( &mutex2 );
        ack[recv.header.ack]=1;
        pthread_mutex_unlock( &mutex2 );
    }

}
void printServerInfo(unsigned short port) {
    printf("═══════ Server ═══════\n");
    printf("Server IP is 127.0.0.1\n");
    printf("Listening on port %hu\n", port);
    printf("══════════════════════\n");
}


void sendMessage(char *message) {
    Packet packet;
    memset(&packet, 0, sizeof(packet));

    packet.header.size = strlen(message);
    packet.header.isLast = true;
    strcpy((char *)packet.data, message);

    if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&clientInfo, sizeof(struct sockaddr_in)) == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }
}

void recvCommand(char *command) {
    Packet packet;
    memset(&packet, 0, sizeof(packet));

    if (recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&clientInfo, (socklen_t *)&addrlen) == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }

    strncpy(command, (char *)packet.data, packet.header.size);
}

FILE* getFile(char *filename) {
    fd = fopen(filename, "rb");
    return fd;
}


void sendFile(FILE *fd) {
    size_t filesize = getFileSize(fd);
    pthread_t t1,t2,t3;
    pthread_create(&t1, NULL, child,NULL);
    pthread_create(&t2, NULL, recv_ack,NULL);
    pthread_create(&t3, NULL, timeoutf,NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    unsigned short port = atoi(argv[1]);

    setServerInfo(INADDR_ANY, port);
    printServerInfo(port);
    setClientInfo();
    createSocket();
    bindSocket();

    FILE *fd;
    char command[96];
    char message[64];

    while (true) {
        memset(command, '\0', sizeof(command));
        memset(message, '\0', sizeof(message));

        printf("Server is waiting...\n");
        recvCommand(command);

        printf("Processing command...\n");
        char *str = strtok(command, " ");

        if (strcmp(str, "download") == 0) {
            str = strtok(NULL, "");
            printf("Filename is %s\n", str);

            if ((fd = getFile(str))) {
                snprintf(message, sizeof(message) - 1, "FILE_SIZE=%zu", getFileSize(fd));
                sendMessage(message);

                printf("══════ Sending ═══════\n");
                sendFile(fd);
                printf("══════════════════════\n");

                fclose(fd);
                fd = NULL;
            } else {
                printf("File does not exist\n");
                sendMessage("NOT_FOUND");
            }
            continue;
        }
        printf("Invalid command\n");
    }
    return 0;
}
