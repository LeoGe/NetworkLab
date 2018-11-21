#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <arpa/inet.h>
#include<netdb.h>	//hostent


#define CHECK_ERR(d) if(d == -1) { printf("Error: %s\n", strerror(errno)); exit(1); }


unsigned short checksum(void *b, int len)
{   unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}


int main(int argc, char** argv) {
    // parse ping addr
    if(argc < 2) {
        printf("Please feed me with an IP address!!\n");
        return -1;
    }

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    if(inet_pton(AF_INET, argv[1], &addr.sin_addr) == 0) {
        struct addrinfo hints, *servinfo, *p;
        int rv;
        struct sockaddr_in *h;

        if ( (rv = getaddrinfo( argv[1] , "http" , &hints , &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        for(p = servinfo; p != NULL; p = p->ai_next)
        {
            h = (struct sockaddr_in *) p->ai_addr;
            memcpy(&addr.sin_addr, &h->sin_addr, sizeof(addr.sin_addr));
            printf("Got %s\n", inet_ntoa( h->sin_addr ));
            break;
        }
    }
    
    int sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    CHECK_ERR(sd);

    uint8_t recv_buf[28];
    struct timespec start, end;

    struct sockaddr_in recv_from;
    int recv_from_length = 0;
    int num = 0;
    while(1) {
        uint8_t buf[8] = {
            0x08, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, num
        };

        uint16_t a = checksum((void*)buf, 8);

        buf[2] = (uint8_t)(a);
        buf[3] = (uint8_t)(a >> 8);

        printf("Ping .. ");
        CHECK_ERR(sendto(sd, &buf, 8, 0, (struct sockaddr*) &addr, sizeof(addr)));
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        
        CHECK_ERR(recvfrom(sd, (void*) &recv_buf, 28, 0, (struct sockaddr*)&recv_from, &recv_from_length));

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;


        if(recv_buf[20] == 0) {
            printf("pong from %s: icmp_seg=%i ttl=%i time=%iμs :)\n", inet_ntoa( ((struct sockaddr_in*)&recv_from)->sin_addr ), recv_buf[27], recv_buf[8], delta_us);
        } else {
            printf("nobody is home\n");
        }

        sleep(1);

        num += 1;
    }


    
    return 0;
}

