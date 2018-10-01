#define _GNU_SOURCE
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
#include "./wrappers/epoll.h"
#include "./wrappers/socketwrappers.h"

static struct option long_options[] = {
    {"address",          required_argument, 0, 'a'},
    {"port",             required_argument, 0, 'p'},
    {"encrypt",          no_argument,       0, 'e'},
    {"server",           no_argument,       0, 's'},
    {"stest",            no_argument,       0, 'd'},
    {"ctest",            no_argument,       0, 'c'},
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

#define BUFFLEN 65536
#define MAXEVENTS 60


void crypto(char *input,char *output, unsigned char *key, unsigned char *iv, bool encryptfile);
int sendFile(int serversocket, int input, int pipefd[2]);
void NewConnection(int socket, int epollfd);
void spliceTo(int source, int destination, int pipefd[2]);
int decrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
int encrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
void HandleErrors(void);

int main(int argc, char **argv){
    unsigned char *key = (unsigned char *)"01234567890123456789012345678901"; //Key
    unsigned char *iv = (unsigned char*)"0123456789012345"; //IV
    struct epoll_event event;
    struct epoll_event *events;
    int arg, listensocket, epollfd;
    bool server = false;
    char *address = 0;
    char *port = 0;
    char *inputfile;
    char *outputfile;
    int pipefd[2];
    int servertest, clienttest;
    int encrypt = true;

    while(1){
        int option_index = 0;
        arg = getopt_long(argc, argv, "i:p:f:e:s:d:c", long_options, &option_index);
        if(arg == -1){
            break;
        }
        switch(arg){
            case 'i':
                address = optarg;
                printf("Destination Address: %s\n", address);
                break;
            case 'p':
                port = optarg;
                printf("Port: %s\n", port);
                break;
            case 'e':
                encrypt = false;
                printf("Encrypt: true\n");
                break;
            case 's':
                server = true;
                printf("Server: true\n");
                break;
            case 'd':
                servertest = true;
                break;
            case 'c':
                servertest = false;
                break;

            default:
                //print_usage();
                return EXIT_SUCCESS;
        }
    }


    if(pipe(pipefd) == -1) {
        perror("creating pipe");
    }

    if(server){
        inputfile = "output";
        outputfile = "output.bmp";
        int outputfd;


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
                    if((outputfd = open(inputfile, O_WRONLY)) < 0) {
                        perror("file can't be opened");
                        exit(1);
                    }
                    printf("New data!\n");
                    spliceTo(events[i].data.fd, outputfd, pipefd);
                    close(outputfd);
                    crypto(inputfile, outputfile , key, iv, false);
                }
            }
        }
    } else {
        int serversocket;
        int inputfd;
        inputfile = "input.bmp";
        outputfile = "input";


        if(encrypt){
            crypto(inputfile, outputfile, key, iv, true);
        } else {

        }

        if((inputfd = open(outputfile, O_RDONLY)) < 0) {
            perror("file can't be opened");
            exit(1);
        }

        serversocket = makeConnect(address, port);
        spliceTo(inputfd, serversocket, pipefd);

        close(inputfd);
    }
    return 0;
}

//splice from one file descriptor to another
void spliceTo(int source, int destination, int pipefd[2]){
    int getBytes;
    int writeBytes;

    while(1) {
        //move bytes from source fd to pipe
        if((getBytes = splice(source, 0, pipefd[1], 0, USHRT_MAX, SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK)) == -1 && errno != EAGAIN) {
            perror("getting splice error");
        }

        if(getBytes <= 0) {
            return;
        }

        printf("bytes read: %d\n", getBytes);

        //write bytes from pipe to destination fd
        do{
            writeBytes = splice(pipefd[0], 0, destination, 0, getBytes, SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);

            if(writeBytes <= 0) {
                if(writeBytes == -1 && errno != EAGAIN) {
                    perror("writing splice error");
                }
                break;
            }

            printf("wrote: %d\n", writeBytes);
            getBytes -= writeBytes;
        } while(getBytes);
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
void crypto(char *input,char *output, unsigned char *key, unsigned char *iv, bool encryptfile){
    FILE* cryptoinput;
    FILE* cryptooutput;
    if((cryptoinput = fopen(input, "rb")) == NULL){
       printf("Could not be open for reading \n");
       exit(1);
    }

    if((cryptooutput = fopen(output, "wb")) == NULL){
        printf("Could not be opened for writing");
        exit(1);
    }
    if(encryptfile){
        encrypt(cryptoinput, cryptooutput, key, iv);
    } else {
        decrypt(cryptoinput, cryptooutput, key, iv);
    }
    fclose(cryptoinput);
    fclose(cryptooutput);

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
    int result;
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
        if((result = EVP_DecryptUpdate(ctx, outbuff, &outLen, inbuff, bytesRead)) != 1){
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

