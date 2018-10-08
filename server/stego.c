//#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include "../src/epoll.h"
#include "../src/socketwrappers.h"
#include "../src/stego_utils.h"
#include "../src/encrypt_utils.h"
#include "../src/stego_image.h"

static struct option long_options[] = {
    {"port",             required_argument, 0, 'p'},
    {"secret",           required_argument, 0, 'x'},
    {0,                  0,                 0,   0}
};

#define print_usage() \
    do {\
        printf("Usage options:\n" \
                "\t[d]estination    - destination ip address\n"\                                   "\t[s]ource         - source ip address\n"\
                "\t[p]ort           - port to be used\n"\
                "\t[f]ile           - file to be transfered and encrypted\n"\
                "\t[s]erver         - default is client\n"\
                "");\
    }while(0)

#define BUFFLEN 16

#define INPUTSIZE 1024
#define MAXEVENTS 60
#define DEFAULT_PORT "22"
#define DEFAULT_SECRETFILE "secret.txt"

int main(int argc, char **argv){
    struct epoll_event event;
    struct epoll_event *events;
    int arg, listensocket, epollfd;
    char port[INPUTSIZE] = "";
    char secretfile[INPUTSIZE] = "";
    int pipefd[2], outputfd;
    char encrypt_tmp[INPUTSIZE];        //decode function will name file to be decrypted
    char stego_bmp[INPUTSIZE] = "server_stego.bmp";

    unsigned char *key = (unsigned char *)"01234567890123456789012345678901"; //Key
    unsigned char *iv = (unsigned char*)"0123456789012345"; //IV

    while(1){
        int option_index = 0;
        arg = getopt_long(argc, argv, "p:x:", long_options, &option_index);
        if(arg == -1){
            break;
        }
        switch(arg){
            case 'p':
                strncpy(port, optarg, INPUTSIZE);
                printf("Port: %s\n", port);
                break;
            case 'x':
                strncpy(secretfile, optarg, INPUTSIZE);
                printf("secretfile:%s\n", secretfile);
                break;
            default:
                //print_usage();
                return EXIT_SUCCESS;
        }
    }

    if(strcmp(port, "") == 0) {
        printf("No port specified, using default port %s\n", DEFAULT_PORT);
        strncpy(port, DEFAULT_PORT, INPUTSIZE);
    }

    if(strcmp(secretfile, "") == 0) {
        printf("No secret file specified, using default secret file %s\n", DEFAULT_SECRETFILE);
        strncpy(secretfile, DEFAULT_SECRETFILE, INPUTSIZE);
    }

    if(pipe(pipefd) == -1) {
        perror("creating pipe");
    }

    listensocket = makeBind(port);
    setNonBlocking(listensocket);
    epollfd = createEpollFd();
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
    event.data.fd = listensocket;
    addEpollSocket(epollfd, listensocket, &event);
    setListen(listensocket);
    events = calloc(MAXEVENTS, sizeof(event));

    while(1){
        int fd, i;
        fd = waitForEpollEvent(epollfd, events);

        for(i = 0; i < fd; i++){
            if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)){
                close(events[i].data.fd);
                continue;
            } else if(events[i].data.fd == listensocket){
                NewConnection(events[i].data.fd, epollfd);
                printf("New Connection on the server!\n");
            } else if(events[i].events & EPOLLIN){
                if((outputfd = open(stego_bmp, O_WRONLY | O_CREAT )) < 0) {
                    perror("file can't be opened");
                    exit(1);
                }

                printf("New data!\n");
                spliceTo(events[i].data.fd, outputfd, pipefd);  //get the cover image
                close(outputfd);
                int result = decode(stego_bmp, encrypt_tmp); //de-stego the cover image and get encrypted file
                printf("Server: Found %s\n", encrypt_tmp);
                crypto(encrypt_tmp, secretfile , key, iv, false); //decrypt file
            }
        }
    }
    return 0;
}
