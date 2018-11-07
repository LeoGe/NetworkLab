#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHECK_ERR(d) if(d == -1) printf("Error: %s\n", strerror(errno))

#define MY_IP "134.130.223.87"
#define OTHER_IP "134.130.223.87"

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

    struct sockaddr_in myaddr = create_addr(MY_IP, 8000);

    int bind_sd = bind(sd, (struct sockaddr*)&myaddr, sizeof myaddr);
    CHECK_ERR(bind_sd);

    struct sockaddr_in addr = create_addr(OTHER_IP, 8000);

    char buf_in[256];
    char* b = buf_in;
    size_t buf_size = 256;
    size_t read_in = 0;

    while(1) {
        read_in = getline(&b, &buf_size, stdin);

        b[read_in-1] = '\0';

        sendto(sd, (void*) buf_in, read_in, 0, (struct sockaddr*)&addr, sizeof addr);

        if(strcmp(buf_in, "/q") == 0)
            break;

        int nread = recv(sd, (void*) &buf_in, buf_size, 0);

        printf("Got %i bytes: %s\n", nread, buf_in);
    }

    close(sd);
}
