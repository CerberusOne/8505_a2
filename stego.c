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
#include "./wrappers/epoll.h"
#include "./wrappers/socketwrappers.h"

static struct option long_options[] = {
     {"ip",    required_argument, 0, 'i'},
     {"port",    required_argument, 0, 'p'},
     {"file",    required_argument, 0, 'f'},
     {"server",  required_argument, 0, 's'},
     {0,         0,                 0, 0}
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

#define BUFFLEN 1024
#define MAXEVENTS 60

int decrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
int encrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
void HandleErrors(void);

int main(int argc, char **argv){
    //FILE* input;
    //FILE* output;
    struct epoll_event event;
    struct epoll_event *events;
    int c, listensocket, epollfd;
    bool server = false;
    char * ip = 0;
    char * port = 0;
    char *filename;

    while(1){
        int option_index = 0;
        c = getopt_long(argc, argv, "i:p:s:f", long_options, &option_index);
        if(c == -1){
            break;
        }
        switch(c){
            case 'i':
                ip = optarg;
                printf("Destination Address: %s\n", ip);
                break;
            case 'p':
                port = optarg;
                printf("Port: %s\n", port);
                break;
            case'f':
                filename = optarg;
                printf("Filename: %s\n", filename);
                break;
            case's':
                server = true;
                printf("Server: true");
                break;
            case '?':

            default:
                //print_usage();
                return EXIT_SUCCESS;
        }
    }

    //initilize key
    //unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
    //initialization vector
    //unsigned char *iv = (unsigned char*)"0123456789012345";

    /*
    if((input = fopen("Encrypted_file.txt", "rb")) == NULL){
        printf("Could not be open for reading \n");
        exit(1);
    }

    if((output = fopen("Decrypted_file.txt", "wb")) == NULL){
        printf("Encrypted_file could not be opened for writing");
        exit(1);
    }
    server = 1;
    if(server){
        decrypt(input, output, key, iv);
    } else{
        encrypt(input, output, key, iv);
    }*/

    if(server){
    listensocket = makeBind(port);

    setNonBlocking(listensocket);

    setListen(listensocket);

    epollfd = createEpollFd();
    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    addEpollSocket(epollfd, listensocket, &event);


    events = calloc(MAXEVENTS, sizeof(event));
    while(1){
        socklen_t sin_size;
        int fd, i;
        struct sockaddr_storage their_addr;
        fd = waitForEpollEvent(epollfd, events);

        for(i = 0; i < fd; i++){
            if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)){
                close(events[i].data.fd);
                continue;
            } else if((events[i].events & EPOLLIN)){
                int clientsocket;
                while(1){
                        sin_size = sizeof(struct sockaddr_storage);
                        if((clientsocket = accept(listensocket, (struct sockaddr*)&their_addr, &sin_size)) == -1){
                        if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                            break;
                            //no more connections
                        } else {
                            perror("accept");
                            break;
                        }
                    }
                     setNonBlocking(clientsocket);
                     event.events = EPOLLIN | EPOLLET | EPOLLOUT;
                     event.data.fd = clientsocket;
                     addEpollSocket(epollfd, clientsocket, &event);
                     char buff[BUFFLEN];
                     int bytesRead;
                     fflush(stdout);
                     bytesRead = read(events[i].data.fd, buff, sizeof(buff));
                     printf("%s", buff);
                     close(events[i].data.fd);
                    }
                }
            }
        }
    } else {
        int clientsocket;

        clientsocket = makeConnect(ip, port);
        while(1){
            //send file
        }

    }
    //initilize key
    //unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
    //initialization vector
    //unsigned char *iv = (unsigned char*)"0123456789012345";

    /*
    if((input = fopen("Encrypted_file.txt", "rb")) == NULL){
        printf("Could not be open for reading \n");
        exit(1);
    }

    if((output = fopen("Decrypted_file.txt", "wb")) == NULL){
        printf("Encrypted_file could not be opened for writing");
        exit(1);
    }
    server = 1;
    if(server){
        decrypt(input, output, key, iv);
    } else{
        encrypt(input, output, key, iv);
    }*/

    return 0;
}

void handleErrors(void){
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv){
    EVP_CIPHER_CTX *ctx;
    unsigned char inbuff[BUFFLEN];
    unsigned char outbuff[BUFFLEN];
    int bytesRead, bytesWritten, outLen;
    //initilize context
    if(!(ctx = EVP_CIPHER_CTX_new())){
        handleErrors();
    }

    if(!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)){
        handleErrors();
    }

    while(1){
        if((bytesRead = fread(inbuff, sizeof(unsigned char), BUFFLEN, input)) == -1){
            printf("Could not read file");
            exit(1);
        }
        if((EVP_EncryptUpdate(ctx, outbuff, &outLen, inbuff, bytesRead)) != 1){
            handleErrors();
        }

        if((bytesWritten = fwrite(outbuff, sizeof(unsigned char), outLen, output)) == -1){
        printf("Could not write to file");
        }
        if(bytesRead < BUFFLEN){
            break;
            //encrypt the last block of data
        }
    }
    if(!EVP_EncryptFinal_ex(ctx, outbuff, &outLen)){
        printf("Error on the last block of data");
        handleErrors();
    }
    if((bytesWritten = fwrite(outbuff, sizeof(unsigned char), outLen, output)) == -1){
        printf("Could not write the last block to file");
    }
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}


int decrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv){
    EVP_CIPHER_CTX *ctx;
    unsigned char inbuff[BUFFLEN];
    unsigned char outbuff[BUFFLEN];
    int bytesRead, bytesWritten, outLen;
    //initilize context
    if(!(ctx = EVP_CIPHER_CTX_new())){
        handleErrors();
    }

    if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)){
        handleErrors();
    }

    while(1){
        if((bytesRead = fread(inbuff, sizeof(unsigned char), BUFFLEN, input)) == -1){
            printf("Could not read file");
            exit(1);
        }
        if((EVP_DecryptUpdate(ctx, outbuff, &outLen, inbuff, bytesRead)) != 1){
            handleErrors();
        }

        if((bytesWritten = fwrite(outbuff, sizeof(unsigned char), outLen, output)) == -1){
        printf("Could not write to file");
        }
        if(bytesRead < BUFFLEN){
            break;
            //encrypt the last block of data
        }
    }
    if(!EVP_DecryptFinal_ex(ctx, outbuff, &outLen)){
        printf("Error on the last block of data");
        handleErrors();
    }
    if((bytesWritten = fwrite(outbuff, sizeof(unsigned char), outLen, output)) == -1){
        printf("Could not write the last block to file");
    }
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

