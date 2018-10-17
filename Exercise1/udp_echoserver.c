#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CHECK_ERR(e) if(e==-1) printf("Error: %s\n",strerror(errno))
#define MY_IP "192.168.43.231"
#define YOUR_IP "192.168.43.42"

void main() {
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_ERR(sd);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    inet_pton(AF_INET, MY_IP , &addr.sin_addr);

    int bind_err = bind(sd, (struct sockaddr*) &addr, sizeof(addr));
    CHECK_ERR(bind_err);
    
    char buf[256];
    
    struct sockaddr_in addr_peer;
    addr_peer.sin_family = AF_INET;
    addr_peer.sin_port = htons(8000);
    inet_pton(AF_INET, YOUR_IP, &addr_peer.sin_addr);
    while(1){
        int length = recv(sd, (void*) &buf, sizeof(buf),0);
        printf("Got %i bytes: %s\n", length, buf);
        if(strcmp(buf, "/q")==0) break;

        int send_err = sendto(sd, (void*) buf, length, 0, (struct sockaddr*)&addr_peer, sizeof(addr_peer));
        CHECK_ERR(send_err);
    }
    close(sd);
}

     
