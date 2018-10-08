/*
 * =====================================================================================
 *
 *       Filename:  encrypt_utils.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/07/2018 06:56:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#define BUFFLEN 16

void crypto(char *input,char *output, unsigned char *key, unsigned char *iv, bool encryptfile);
int decryptFile(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
int encryptFile(FILE *input, FILE *output, unsigned char *key, unsigned char *iv);
void HandleErrors(void);
