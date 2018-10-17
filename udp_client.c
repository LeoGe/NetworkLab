#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHECK_ERR(d) if(d == -1) printf("Error: %s\n", strerror(errno))

struct sockaddr_in create_addr(char* addr_str, unsigned short port) {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, addr_str, &addr.sin_addr);

    return addr;
}

void main() {
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_ERR(sd);

    struct sockaddr_in myaddr = create_addr("192.168.1.9", 8000);

    int bind_sd = bind(sd, (struct sockaddr*)&myaddr, sizeof myaddr);
    CHECK_ERR(bind_sd);

    int buf[1];
    buf[0] = 42;

    struct sockaddr_in addr = create_addr("192.168.1.7", 8000);

    char buf_in[256];
    char* b = buf_in;
    size_t buf_size = 256;
    size_t read_in = 0;

    while(1) {
        read_in = getline(&b, &buf_size, stdin);

        b[read_in-1] = '\0';

        printf("%s\n", b);

        sendto(sd, (void*) buf_in, read_in, 0, (struct sockaddr*)&addr, sizeof addr);

        int nread = recv(sd, (void*) &buf, buf_size, 0);

        printf("Got %i bytes: %s\n", nread, buf);
    }


    close(sd);
}
