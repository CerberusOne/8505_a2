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
     {"ip",      required_argument, 0, 'i'},
     {"port",    required_argument, 0, 'p'},
     {"file",    required_argument, 0, 'f'},
     {"server",  no_argument, 0, 's'},
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

void sendFile(int serversocket, char *filename);
void NewConnection(int socket, int epollfd);
void NewData(int socket);
//int decrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
//int encrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
//void HandleErrors(void);

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
                printf("Server: true\n");
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
                    NewData(events[i].data.fd);
                    printf("New data!\n");
                }
            }
        }
    } else {
        int serversocket;

        serversocket = makeConnect(ip, port);
        sendFile(serversocket, filename);
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

void sendFile(int serversocket, char *filename){
    while(1){
        int bytesRead;
        char buff[BUFFLEN];
        int bytesSent;
        FILE *input;
        if((input = fopen("linux.bmp", "rb")) == NULL){
           perror("fopen");
        }

         /*memset(&buff, 0, BUFFLEN);
         bytesRead = fread(buff, sizeof(char), BUFFLEN, input);
         while(bytesRead){
            if(bytesRead == -1){
                perror("client:fread");
            }
            if((bytesSent = send(serversocket, buff, bytesRead, 0)) == -1){
                perror("client:send");
            }
            memset(&buff, 0, BUFFLEN);
            bytesRead = fread(buff, sizeof(char), BUFFLEN, input);
         }
         memset(&buff, 0, BUFFLEN);
         while((bytesRead = fread(buff, sizeof(char), BUFFLEN, input)) > 0){
            if((bytesSent = send(serversocket, buff, bytesRead, 0)) == -1){
                perror("client:send");
            }
            memset(&buff, 0, BUFFLEN);
         }*/

        memset(&buff, 0, BUFFLEN);
        bytesRead = fread(buff, sizeof(char), BUFFLEN, input);
        while(1){
            if(bytesRead == 0){
                break;
            }
            if(bytesRead == -1){
                perror("client:fread");
                exit(1);
            }
            if((bytesSent = send(serversocket, buff, sizeof(buff), 0)) == -1){
                    perror("client:send");
            }
            printf("Bytes sent: %i", bytesSent);
            bytesRead = fread(buff, sizeof(unsigned char), BUFFLEN, input);
        }
        fclose(input);
        close(serversocket);
        break;
     }
}

void NewConnection(int socket, int epollfd){
    struct epoll_event event;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    int clientsocket;
    while(1){
        sin_size = sizeof(struct sockaddr_storage);
        if((clientsocket = accept(socket, (struct sockaddr*)&their_addr, &sin_size)) == -1){
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
    }
}

void NewData(int socket){
         char buff[BUFFLEN];
         int bytesRead;
         int bytesWritten;
         FILE *output;
         if((output = fopen("output.bmp", "wb+")) == NULL){
            perror("fopen");
         }

         /*memset(&buff, 0, BUFFLEN);
         bytesRead = recv(socket, buff, BUFFLEN, 0);
         while(bytesRead){
            if(bytesRead == -1){
                perror("server:recv");
            }
            if((bytesWritten = fwrite(buff, sizeof(char), bytesRead, output)) == -1){
                perror("server:fwrite");
            }
            memset(&buff, 0, BUFFLEN);
            bytesRead = recv(socket, buff, BUFFLEN, 0);
         }

         memset(&buff, 0, BUFFLEN);

         while((bytesRead = recv(socket, buff, BUFFLEN, 0)) > 0){
            if((bytesWritten = fwrite(buff, sizeof(char), bytesRead, output)) < bytesRead){
                perror("server:fwrite");
            }
            memset(&buff, 0, BUFFLEN);
            if(bytesRead == 0 || bytesRead != BUFFLEN){
                break;
            }
            if(bytesRead < 0){
                if(errno = EAGAIN){
                    perror("recv timed out");
                } else {
                    perror("failed to errno");
                }
            }
         }*/

         //memset(&buff, 0, BUFFLEN);
         bytesRead = recvBytes(socket, buff);
         while(1){
            if(bytesRead == 0){
                break;
            }
            if(bytesRead == -1){
                perror("server:recv");
                exit(1);
            }
            if((bytesWritten = fwrite(buff, sizeof(unsigned char), sizeof(buff), output)) == -1){
                 perror("server:fwrite");
            }
            //fprintf(output, "%s", buff);
            bytesRead = recvBytes(socket, buff);
         }
         fclose(output);
         close(socket);
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

