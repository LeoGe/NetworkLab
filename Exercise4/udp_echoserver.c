#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define CHECK_ERR(e) if(e==-1) printf("Error: %s\n",strerror(errno))

#define MY_IP "134.130.223.87"
#define OTHER_IP "134.130.223.132"

#define BUF_SIZE 256

void sig_handler(int signal);

int sd;
char buf[BUF_SIZE];

void main() {
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_ERR(sd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    inet_pton(AF_INET, MY_IP , &addr.sin_addr);

    int bind_err = bind(sd, (struct sockaddr*) &addr, sizeof(addr));
    CHECK_ERR(bind_err);
    
    // register function for signals
    signal(SIGIO, sig_handler);
    
    int f = fcntl(sd, F_SETOWN, getpid());
    CHECK_ERR(f);

    f = fcntl(sd, F_SETFL, FASYNC);
    CHECK_ERR(f);
    
    while(1) {
        
    }
}

void sig_handler(int signal) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    inet_pton(AF_INET, OTHER_IP , &addr.sin_addr);

    int length = recv(sd, (void*) &buf, 256, 0);

    printf("Got %i bytes: %s\n", length, buf);

    int send_err = sendto(sd, (void*) buf, BUF_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
    CHECK_ERR(send_err);
}

     
