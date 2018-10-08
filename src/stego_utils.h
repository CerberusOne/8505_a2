/*
 * =====================================================================================
 *
 *       Filename:  stego_utils.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  10/01/2018 08:43:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef STEGO_UTILS_H
#define STEGO_UTILS_H

//#include "stego_image.h"
#include "EasyBMP.h"


class EasyBMPstegoInternalHeader
{
    public:
        char* FileName;
        int FileNameSize;

        int FileSize;

        unsigned char* CharsToEncode;
        int NumberOfCharsToEncode;

        void InitializeFromFile( char* input , int BMPwidth, int BMPheight );
        void InitializeFromImage( BMP& Input );
};

char ExtractChar( BMP& Image, int i, int j);
void InitializeFromImage( BMP& Image );
void InitializeFromFile( char* input, int BMPwidth, int BMPheight );
int encode(char* cover_image, char* secret_file, char* output_file);
int decode(char* stego_bmp, char* secret_file);

#endif
