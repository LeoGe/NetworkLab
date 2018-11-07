#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netdb.h>
#include <libgen.h>

#define CHECK_ERR(d) if(d == -1) { printf("Error: %s\n", strerror(errno)); exit(1); }

#define MY_IP "127.0.0.1"
#define MY_IP6 "::"


struct sockaddr_in create_addr4(char* addr_str, unsigned short port) {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, addr_str, &addr.sin_addr);

    return addr;
}

struct sockaddr_in6 create_addr6(char* addr_str, unsigned short port) {
    struct sockaddr_in6 addr;

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, addr_str, &addr.sin6_addr);

    return addr;
}

void print_dns(const struct sockaddr* addr, socklen_t length) {
    char host[1024];
    char myhost[256];
    
    if(getnameinfo(addr, length, host, 1024, NULL, 0, 0)) {
        printf("Could not determine peer name!\n");
    } else {
        gethostname(myhost, 256);

        printf("Connection from %s to %s\n", host, myhost);
    }
}

int read_header(char* buf_in, int* type) {
    // position in the complete buffer
    int pos_in = 0;

    // position in the state buffer
    int pos = 0;
    // state buffer
    char buf[256];

    // counted spaces
    int space = 0;

    // return (either fd of the file or error number)
    int fp_ret = 0;

    while(space < 3) {
        // read in the next character
        char c = buf_in[pos_in];

        if(c == ' ' && space == 0) {
            buf[pos] = '\0';
            if(strcmp(buf, "GET") != 0) return -1;

            // reset state buffer position
            pos = 0;
            space += 1;
        } else if(c == ' ' && space == 1) {
            buf[pos] = '\0';

            // handle case "/"
            char file[256];
            if(pos == 1) strcpy(file, "index.html"); 
            else strcpy(file, basename(&buf[1]));

            // create a copy because strtok modifies the original buffer
            char file_copy[256];
            strcpy(&file_copy[0], file);

            // determine the file ending
            char* elms = strtok(file, ".");
            elms = strtok(NULL, ".");

            if(elms == NULL) return -4;
            else if(strcmp(elms, "html") == 0) *type = 0;
            else if(strcmp(elms, "txt") == 0) *type = 1;
            else if(strcmp(elms, "png") == 0) *type = 2;
            else return -4;

            // open the file
            FILE* fp = fopen(file_copy, "r");
            if(fp == NULL) return -2;
            fp_ret = fileno(fp);

            pos = 0;
            space += 1;
        } else if(c == '\n' && space == 2) {
            // check whether the character before the last one is a '\r'
            if(buf[pos-1] == '\r') {
                buf[pos-1] = '\0';
            }

            // we only support HTTP/1.0 and HTTP/1.1
            if(strcmp(buf, "HTTP/1.0") != 0 && strcmp(buf, "HTTP/1.1") != 0) return -3;
            else return fp_ret;
        } else {
            // no state transition, therefore save the character
            buf[pos] = c;
            pos += 1;
        }

        // increment the read in buffer position
        pos_in += 1;
    }
    
}

int process(char* buf_in, char* buf_header, char** buf_file) {
    int type = 0;
    int fd = read_header(buf_in, &type);
    
    // Write the first line
    strcat(buf_header, "HTTP/1.1 ");
    switch(fd) {
        case -1: 
            strcat(buf_header, "400 Bad Request\n");
            break;
        case -2: 
            strcat(buf_header, "404 Not Found\n");
            break;
        case -3: 
            strcat(buf_header, "400 Bad Request\n");
            break;
        case -4:
            strcat(buf_header, "400 Bad Request\n");
            break;
        default:
            strcat(buf_header, "200 OK\n");
    }

    // write the content-type, in case of an error we will send a HTML file (404)
    switch(type) {
        case 0: 
            strcat(buf_header, "Content-Type: text/html\n");
            break;
        case 1:
            strcat(buf_header, "Content-Type: text/plain\n");
            break;
        case 2:
            strcat(buf_header, "Content-Type: image/png\n");
            break;
    }

    // send the 404 error page in case of an error
    if(fd < 0)
        fd = fileno(fopen("./404.html", "r"));

    /* Go to the end of the file. */
    if (fseek(fdopen(fd, "r"), 0L, SEEK_END) != 0) fd = -2;

    /* Get the size of the file. */
    long bufsize = ftell(fdopen(fd, "r"));
    if (bufsize == -1) {
        fd = -2;
    }
    /* Go back to the start of the file. */
    if (fseek(fdopen(fd, "r"), 0L, SEEK_SET) != 0) {
        fd = -2;
    }

    strcat(buf_header, "Content-Length: ");

    char tmp[40];
    sprintf(tmp, "%d", bufsize);

    strcat(buf_header, tmp);
    strcat(buf_header, "\nConnection: close\n\n");

    /* Read the entire file into memory. */
    *buf_file = (char*)malloc(bufsize);
    
    size_t newLen = fread((void*) *buf_file, sizeof(char), bufsize, fdopen(fd, "r"));
    close(fd);

    return bufsize;
}

void main(int argc, char** argv) {
    // read in the port number as the first argument
    int port;
    if(argc > 1 && (port = atoi(argv[1])) != 0) {}
    else port = 8000;

    int sd = socket(AF_INET6, SOCK_STREAM, 0);
    CHECK_ERR(sd);

    int enable = 1;
    int setsock = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    CHECK_ERR(setsock);

    //struct sockaddr_in myaddr = create_addr4(MY_IP, port);
    struct sockaddr_in6 myaddr = create_addr6(MY_IP6, port);

    int bind_sd = bind(sd, (struct sockaddr*)&myaddr, sizeof myaddr);
    CHECK_ERR(bind_sd);

    int listen_sd = listen(sd, 5);
    CHECK_ERR(listen_sd);

    printf("Server listens on %s:%i\n", MY_IP, port);

    struct sockaddr addr;
    socklen_t length;
    
    fd_set all_read, local_read;
    FD_ZERO(&all_read);
    FD_SET(sd, &all_read);
    FD_SET(0, &all_read);

    int is_running = 1;
    char buf[2048];
    char buf_header[1024];
    while(is_running == 1) {
        local_read = all_read;

        // select -> fds ..
        int sret = select(FD_SETSIZE, &local_read, NULL, NULL, NULL);
        CHECK_ERR(sret);

        // process
        if(FD_ISSET(sd, &local_read)) {
            // accept a new client, we don't need the peer address
            int sd_client = accept(sd, &addr, &length);
            if(sd_client < 0) {
                printf("Got a malfunctional client!\n");
                continue;
            }
            FD_SET(sd_client, &all_read);

            // print new connection to console
            print_dns(&addr, length);
        }
        else if(FD_ISSET(0, &local_read)){
            char character = 0;

            int nread = read(0, &character, 1);
            CHECK_ERR(nread);

            if(nread == 1){
                switch(character) {
                    case 'q': 
                        printf("Close!!\n"); 
                        is_running = 0;
                        break;
                    case 's':
                        if(FD_ISSET(sd, &all_read))
                            FD_CLR(sd, &all_read);
                        else
                            FD_SET(sd, &all_read);
                        break;
                }
            }
        }

        int i = 1;
        for(i;i<FD_SETSIZE;i++){
            if(i != sd && FD_ISSET(i, &local_read)) {
                int sd_recv = recv(i, (void*) &buf, 2048, 0);
                if(sd_recv == 0) {
                    printf("Client disconnected!\n");
                    FD_CLR(i, &all_read);
                    close(i);

                    continue;
                } else CHECK_ERR(sd_recv);
                
                if(strstr(buf, "GET") != NULL) {
                    char* buf_file;
                
                    // process request
                    memset(&buf_header, '\0', 1024);
                    int length = process(buf, buf_header, &buf_file);
                
                    // write out the answer
                    int written = write(i, (void*) &buf_header, strlen(buf_header));
                    written = write(i, (void*) buf_file, length);

                    printf("Answer (%i playload):\n%s", written, buf_header);
                
                    // deallocate buffers
                    free(buf_file);
                }
            }
        }
    }

    close(sd);
}
