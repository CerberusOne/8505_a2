#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

/*static struct option long_options[] = {
     {"dest",    required_argument, 0, 'd'},
     {"src",     required_argument, 0, 's'},
     {"port",    required_argument, 0, 'p'},
     {"file",    required_argument, 0, 'f'},
     {0,         0,                 0, 0}
     };
*/
#define print_usage() \
     do {\
         printf("Usage options:\n" \
                          "\t[d]estination    - destination ip address\n"\                                    "\t[s]ource         - source ip address\n"\
                    "\t[p]ort           - port to be used\n"\
                    "\t[f]ile           - file to be transfered and encrypted\n"\
                    "");\
        }while(0)

#define BUFFLEN 1024

int decrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
int encrypt(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
void HandleErrors(void);

int main(int argc, char **argv){
    FILE* input;
    FILE* output;
    //int c;
    bool server;
    //unsigned int dest, src;
    //char *filename;
    //short port;

    //initilize key
    unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
    //initialization vector
    unsigned char *iv = (unsigned char*)"0123456789012345";


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
    }

    /*while(1){
        int option_index = 0;
        c = getopt_long(argc, argv, "d:s:p:f", long_options, &option_index);

        if(c == -1){
            break;
        }
        switch(c){
            case 'd':
                dest = inet_addr(optarg);
                printf("Destination Address: %i\n", dest);
                break;
            case 's':
                src = inet_addr(src);
                printf("Source Address: %i\n", src);
                break;
            case 'p':
                port = atoi(optarg);
                printf("Port: %d\n", port);
                break;
            case'f':
                strcpy(filename, optarg);
                printf("Filename: %s\n", filename);
                break;
            case '?':

            default:
                //print_usage();
                return EXIT_SUCCESS;
        }
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

