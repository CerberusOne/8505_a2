#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
//
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char*plaintext);
int main(){
    //EVP key
    unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
    //initialization vector
    unsigned char *iv = (unsigned char*)"0123456789012345";
    //message being encrypted
    unsigned char *plaintext = (unsigned char*)"hello this is benedict lo";
    //buffer for the cypher
    //ciphertext may be longer than the plaintext if padding is being used
    unsigned char ciphertext[128];
    //buffer for decrypted data
    unsigned char decryptedtext[128];
    int decryptedtext_len, ciphertext_len;
    //encrypt plaintext
    ciphertext_len = encrypt(plaintext, strlen((char*)plaintext), key, iv, ciphertext);

    printf("ciphertext is\n");
    BIO_dump_fp(stdout, (const char*)ciphertext, ciphertext_len);
    //decrypt the cyphertext
    decryptedtext_len = decrypt(ciphertext, ciphertext_len, key,iv, decryptedtext);
    //add a null terminator
    decryptedtext[decryptedtext_len] = '\0';

    printf("Decrypted text is \n %s \n", decryptedtext);
    return 0;
}

void handleErrors(void)
{
    //dumps any error messages from the OpenSSL error stack to the screen
  ERR_print_errors_fp(stderr);
  abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext){
  EVP_CIPHER_CTX *ctx;
  int len;
  int ciphertext_len;
    //setup a context
    //initialise the encryption operation
    //providing plaintext bytes to be encrypted
    //finalising the enryption operation


  /* Create and initialise a cipher context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
    // ctx must be initilized first
    // type is normally supplied by EVP_aes_256_cbc()
    // if impl is NULL then default
    // key is the symmetric key
    // iv is the IV
    // EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, ENGINE *impl, unsined char *key, unsigned char *iv);
  if(EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1){
    handleErrors();
  }
  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary*/
  //intl - encrypts inl bytes from the buffer "in" and writes the encrypted version to "out"
  //this can be called multiple times to encrypt sucessive blocks of data
  //outl - number of bytes written
  //EVPEncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, unsigned char *in, int inl);
  if(EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1){
    handleErrors();
    ciphertext_len = len;
  }
  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.*/
  //If padding is enabled this will encrypt the final data reamining in the block
  //written to "out"
  //EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
  if(EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1){
      handleErrors();
      ciphertext_len += len;
  }

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext){
  EVP_CIPHER_CTX *ctx;
  int len;
  int plaintext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}
