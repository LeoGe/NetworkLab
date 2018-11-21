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
#include <net/if.h>
#include <sys/ioctl.h>

#define CHECK_ERR(d) if(d == -1) { printf("Error: %s\n", strerror(errno)); exit(1); }

void print_ip(uint8_t* addr)
{
    printf("%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
}

void print_hex(uint8_t* buf, size_t length) {
    printf("Received packet with length %i\n", length);
    for(int i = 0; i < length; i++) {
        if(i % 8 == 0)
            printf("\n");

        printf("%.2X", buf[i]);
    }
    printf("\n\n");
}

void print_packet(uint8_t* buf, size_t length) {
    printf("Got new packet with length %i\n", length);
    printf("\tEthernet\t%.2X:%.2X:%.2X:%.2X:%.2X:%.2X to %.2X:%.2X:%.2X:%.2X:%.2X:%.2X with %s", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], (buf[12] == 0x08 ? "IPV4":"IPV6"));

    uint8_t* ip_header = buf + 14;

    printf("\n\tInternet\ttime to live: %i", ip_header[8]);

    if(ip_header[9] == 6) printf(" protocol: tcp");
    else if(ip_header[9] == 17) printf(" protocol: udp");
    else if(ip_header[9] == 1) printf(" protocol: icmp");
    else printf(" protocol: %i", ip_header[9]);

    printf(" source: ");
    print_ip(ip_header + 12);
    printf(" dest: ");
    print_ip(ip_header + 16);

    printf("\n\n");
}

int main(int argc, char** argv) {
    if(argc == 2) {
        printf("Please give me a card!");
        return -1;
    }

    int sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    CHECK_ERR(sd);

    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, argv[1], IFNAMSIZ-1);
    if (ioctl(sd, SIOCGIFINDEX, &if_idx) < 0) {
        printf("Error");
        return -1;
    }

    struct sockaddr_ll addr;
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = 0;
    addr.sll_ifindex = if_idx.ifr_ifindex;

    CHECK_ERR(bind(sd, (struct sockaddr*)&addr, sizeof(addr)));

    if(strcmp(argv[2], "y") == 0) {
        if_idx.ifr_flags |= IFF_PROMISC;
        CHECK_ERR(ioctl(sd, SIOCSIFFLAGS, &if_idx));
    }

    uint8_t buf[1024];
    while(1) {
        int nread = read(sd, &buf, 1024);

        print_packet(buf, nread);
    }
    
    return 0;
}
