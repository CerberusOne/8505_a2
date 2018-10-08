/*
 * =====================================================================================
 *
 *       Filename:  stego_image.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  10/07/2018 07:28:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include "stego_image.h"

char ExtractChar( BMP& Image, int i, int j)
{
    RGBApixel Pixel1 = *Image(i,j);
    i++;
    if( i == Image.TellWidth() ){ i=0; j++; }

    RGBApixel Pixel2 = *Image(i,j);

    // convert the two pixels to a character

    unsigned int T = 0;
    T += (Pixel1.Red%2);
    T += (2* (Pixel1.Green%2));
    T += (4* (Pixel1.Blue%2));
    T += (8* (Pixel1.Alpha%2));

    T += (16*  (Pixel2.Red%2));
    T += (32*  (Pixel2.Green%2));
    T += (64*  (Pixel2.Blue%2));
    T += (128* (Pixel2.Alpha%2));

    char c = (char) T;
    return c;
}
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
void EasyBMPstegoInternalHeader::InitializeFromImage( BMP& Image )
{
    if( Image.TellWidth()*Image.TellHeight() < 2*(12+2+4) )
    {
        CharsToEncode = NULL;
        NumberOfCharsToEncode = 0;
        FileName = NULL;
        FileNameSize = 0;
        FileSize = 0;
        return;
    }

    // extract the first few purported characters to see if this thing
    // has hidden data

    char* StegoIdentifierString = "EasyBMPstego";
    char ComparisonString [strlen(StegoIdentifierString)+1];

    int i=0;
    int j=0;
    int k=0;

    while( k < strlen(StegoIdentifierString) )
    {
        ComparisonString[k] = ExtractChar( Image, i, j);
        i += 2;
        while( i >= Image.TellWidth() )
        { i -= Image.TellWidth(); j++; }
        k++;
    }
    ComparisonString[k] = '\0';

    if( strcmp( StegoIdentifierString , ComparisonString ) )
    {
        cout << "Error: No (compatible) hidden data found in image!\n";
        FileSize = 0;
        FileNameSize = 0;
        return;
    }

    // get the next two characters to determine file size
    unsigned char C1 = (unsigned char) ExtractChar( Image, i,j );
    i += 2;
    while( i >= Image.TellWidth() )
    { i -= Image.TellWidth(); j++; }
    unsigned char C2 = (unsigned char) ExtractChar( Image, i,j );
    i += 2;
    while( i >= Image.TellWidth() )
    { i -= Image.TellWidth(); j++; }

    FileNameSize = C1 + 256*C2;
    FileName = new char[ FileNameSize+1];

    // read the filename

    k=0;
    while( k < FileNameSize )
    {
        FileName[k] = ExtractChar( Image, i, j);
        i += 2;
        while( i >= Image.TellWidth() )
        { i -= Image.TellWidth(); j++; }
        k++;
    }
    FileName[k] = '\0';

    // find the actual data size

    C1 = (unsigned char) ExtractChar( Image, i,j );
    i += 2;
    while( i >= Image.TellWidth() )
    { i -= Image.TellWidth(); j++; }
    C2 = (unsigned char) ExtractChar( Image, i,j );
    i += 2;
    while( i >= Image.TellWidth() )
    { i -= Image.TellWidth(); j++; }
    unsigned char C3 = (unsigned char) ExtractChar( Image, i,j );
    i += 2;
    while( i >= Image.TellWidth() )
    { i -= Image.TellWidth(); j++; }
    unsigned char C4 = (unsigned char) ExtractChar( Image, i,j );
    i += 2;
    while( i >= Image.TellWidth() )
    { i -= Image.TellWidth(); j++; }

    FileSize = C1 + 256*C2 + 65536*C3 + 16777216*C4;
    NumberOfCharsToEncode = FileNameSize + 2 + 4
        + strlen(StegoIdentifierString);
    return;
}

void EasyBMPstegoInternalHeader::InitializeFromFile( char* input, int BMPwidth, int BMPheight )
{
    FileNameSize = strlen( input ) +1;
    FileName = new char [FileNameSize];
    strcpy( FileName , input );
    FileNameSize--;

    int SPACE = 32;

    char* StegoIdentifierString = "EasyBMPstego";

    NumberOfCharsToEncode = FileNameSize
        + strlen(StegoIdentifierString)
        + 2 // indicate length of filename
        + 4; // indicate length of data

    int MaxCharsToEncode = (int) ( 0.5 * BMPwidth * BMPheight );
    if( NumberOfCharsToEncode >  MaxCharsToEncode )
    {
        cout << "Error: File is too small to even encode file information!\n"
            << "       Terminating encoding.\n";
        FileSize = 0;
        CharsToEncode = NULL;
        NumberOfCharsToEncode = 0;
        return;
    }

    CharsToEncode = new unsigned char [NumberOfCharsToEncode];

    FILE* fp;
    fp = fopen( FileName , "rb" );
    if( !fp )
    {
        FileSize = 0;

        return;
    }

    // determine the number of actual data bytes to encode

    FileSize = 0;
    while( !feof( fp ) )
    {
        char c;
        fread( &c, 1, 1, fp );
        FileSize++;
    }
    FileSize--;

    MaxCharsToEncode -= NumberOfCharsToEncode;
    if( FileSize > MaxCharsToEncode )
    {
        FileSize = MaxCharsToEncode;
        cout << "Warning: Input file exceeds encoding capacity of the image\n"
            << "         File will be truncated.\n";
    }
    fclose( fp );

    // create this "file header" string

    int k = 0;

    // this part gives the length of the filename
    while( k < strlen(StegoIdentifierString) )
    { CharsToEncode[k] = StegoIdentifierString[k]; k++; }
    int TempInt = FileNameSize % 256;
    CharsToEncode[k] = (unsigned char) TempInt; k++;
    TempInt = FileNameSize - TempInt;
    if( TempInt < 0 ){ TempInt = 0; }
    TempInt = TempInt / 256;
    CharsToEncode[k] = (unsigned char) TempInt; k++;

    // this part hides the filename
    int j=0;
    while( j < FileNameSize )
    { CharsToEncode[k] = FileName[j]; k++; j++; }

    // this part gives the length of the hidden data
    int TempIntOriginal = FileSize;
    TempInt = FileSize % 256;
    CharsToEncode[k] = (unsigned char) TempInt; k++;
    TempIntOriginal -= TempInt;

    if( TempIntOriginal > 0 )
    {
        TempInt = TempIntOriginal % 65536;
        CharsToEncode[k] = (unsigned char) (TempInt/256); k++;
        TempIntOriginal -= TempInt*256;
    }
    else
    { CharsToEncode[k] = 0; k++; }

    if( TempIntOriginal > 0 )
    {
        TempInt = TempIntOriginal % 16777216;
        CharsToEncode[k] = (unsigned char) (TempInt/65536); k++;
        TempIntOriginal -= TempInt*65536;
    }
    else
    { CharsToEncode[k] = 0; k++; }

    if( TempIntOriginal > 0 )
    {
        TempInt = TempIntOriginal % 4294967296;
        CharsToEncode[k] = (unsigned char) (TempInt/1677216); k++;
        TempIntOriginal -= TempInt*16777216;
    }
    else
    { CharsToEncode[k] = 0; k++; }

    return;
}
