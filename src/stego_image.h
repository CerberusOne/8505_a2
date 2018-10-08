/*
 * =====================================================================================
 *
 *       Filename:  stego_image.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/07/2018 07:29:14 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef STEGO_IMAGE_H
#define STEGO_IMAGE_H

#include "EasyBMP.h"

using namespace std;
char ExtractChar( BMP& Image, int i, int j);
void InitializeFromImage( BMP& Image );
void InitializeFromFile( char* input, int BMPwidth, int BMPheight );

#endif
