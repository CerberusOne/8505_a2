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
#include "../src/stego_image.h"
#include "../src/encrypt_utils.h"

static struct option long_options[] = {
    {"address",          required_argument, 0, 'a'},
    {"port",             required_argument, 0, 'p'},
    {"encrypt",          no_argument,       0, 'e'},
    {"server",           no_argument,       0, 's'},
    {"stest",            no_argument,       0, 'd'},
    {"ctest",            no_argument,       0, 'c'},
    {"input",            required_argument, 0, 'i'},
    {"secret",           required_argument, 0, 'x'},
    {"output",           required_argument, 0, 'o'},
    {0,                  0,                 0,   0}
};

#define print_usage() \
    do {\
        printf("Usage options:\n" \
                "\t[a]ddress        - ip address\n"\
                "\t[p]ort           - port to be used\n"\
                "\t[i]nput          - covert image to hide secret within\n"\
                "\t[x]secret        - secret file to hide within cover image\n"\
                "\t[o]utput         - resulting image with secret embedded file\n"\
                "");\
    }while(0)

#define INPUTSIZE 1024
#define MAXEVENTS 60
#define DEFAULT_ADDR "127.0.0.1"
#define DEFAULT_PORT "22"
#define DEFAULT_INPUTFILE "input.bmp"
#define DEFAULT_SECRETFILE "secret.txt"
#define DEFAULT_OUTPUT "output.bmp"



int main(int argc, char **argv){
    int arg;
    int pipefd[2];
    int encrypt = true;
    int serversocket, sendfd, result;

    //user input variables
    char address[INPUTSIZE] = "";
    char port[INPUTSIZE] = "";
    char inputfile[INPUTSIZE] = "";
    char secretfile[INPUTSIZE] = "";
    char outputfile[INPUTSIZE] = "";
    char encrypt_tmp[INPUTSIZE] = "client_encrypt_tmp";

    unsigned char *key = (unsigned char *)"01234567890123456789012345678901"; //Key
    unsigned char *iv = (unsigned char*)"0123456789012345"; //IV

    while(1){
        int option_index = 0;
        arg = getopt_long(argc, argv, "a:p:f:e:s:d:c:i:x:o:", long_options, &option_index);
        if(arg == -1){
            break;
        }
        switch(arg){
            case 'a':
                strncpy(address, optarg, INPUTSIZE);
                printf("Destination Address: %s\n", address);
                break;
            case 'p':
                strncpy(port, optarg, INPUTSIZE);
                printf("Port: %s\n", port);
                break;
            case 'i':
                strncpy(inputfile, optarg, INPUTSIZE);
                printf("inputfile:%s\n", inputfile);
                break;
            case 'x':
                strncpy(secretfile, optarg, INPUTSIZE);
                printf("secretfile:%s\n", secretfile);
                break;
            case 'o':
                strncpy(outputfile, optarg, INPUTSIZE);
                printf("outputfile:%s\n", outputfile);
                break;
            case 'h':
                print_usage();
                break;
            default:
                //print_usage();
                return EXIT_SUCCESS;
        }
    }


    //if there is no user specification for address and port
    if(strcmp(address, "") == 0) {
        printf("No address or port specified, using default: 127.0.0.1:22\n");
        strncpy(address, DEFAULT_ADDR, INPUTSIZE);
        strncpy(port, DEFAULT_PORT, INPUTSIZE);
    }

    if(strcmp(inputfile, "") == 0) {
        printf("No input file specified, using default: \"inputfile\"\n");
        strncpy(inputfile, DEFAULT_INPUTFILE, INPUTSIZE);
    }

    if(strcmp(secretfile, "") == 0) {
        printf("No secret file specified, using default: \"secretfile\"\n");
        strncpy(secretfile, DEFAULT_SECRETFILE, INPUTSIZE);
    }

    if(strcmp(outputfile, "") == 0) {
        printf("No output file specified, using default: \"outputfile\"\n");
        strncpy(outputfile, DEFAULT_OUTPUT, INPUTSIZE);
    }

    //create pipe for splicing from file to network socket
    if(pipe(pipefd) == -1) {
        perror("creating pipe");
    }

    //encrypt secret file
    if(encrypt){
        crypto(secretfile, encrypt_tmp, key, iv, true);         //encrypt the secret
    }

    //hide the encrypted file in the cover image to make a new stego image
    result = encode(inputfile, encrypt_tmp, outputfile);    //stego the encrypted secret into an image

    //open the newly created stego image
    if((sendfd = open(outputfile, O_RDONLY)) < 0) {
        perror("file can't be opened");
        exit(1);
    }

    //send the stego image to the server
    serversocket = makeConnect(address, port);
    spliceTo(sendfd, serversocket, pipefd);                  //send the stego imamge to server

    close(sendfd);
    return 0;
}
