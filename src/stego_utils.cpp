/*
 * =====================================================================================
 *
 *       Filename:  stego.cpp
 *
 *    Description:  Encoding and decoding functions for stegonography
 *
 *        Version:  1.0
 *        Created:  10/01/2018 08:17:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Aing Ragunathan
 *
 * =====================================================================================
 */
#include "stego_utils.h"
#include "stego_image.h"

//Encodes a secret image into the cover image and creates an output image
int encode(char* cover_image, char* secret_filename, char* output_filename) {
    BMP Image;
    Image.ReadFromFile(cover_image);
    int MaxNumberOfPixels = Image.TellWidth() * Image.TellHeight() - 2;
    int k=1;
    Image.SetBitDepth( 32 );

    FILE* secret_file;
    secret_file = fopen(secret_filename, "rb" );
    if( !secret_file )
    {
        cout << "Error: unable to read file " << secret_filename << " for text input!" << endl;
        return -1;
    }

    // figure out what we need to encode as an internal header
    EasyBMPstegoInternalHeader IH;
    IH.InitializeFromFile( secret_filename , Image.TellWidth() , Image.TellHeight() );
    if( IH.FileNameSize == 0 || IH.NumberOfCharsToEncode == 0 )
    { return -1; }

    k=0;
    int i=0;
    int j=0;

    //Loop through secret file and encode the header information in binary to the cover image's pixel LSBs
    while( !feof( secret_file ) && k < IH.NumberOfCharsToEncode )
    {
        unsigned int T = (unsigned int) IH.CharsToEncode[k];

        int R1 = T % 2;
        T = (T-R1)/2;
        int G1 = T % 2;
        T = (T-G1)/2;
        int B1 = T % 2;
        T = (T-B1)/2;
        int A1 = T % 2;
        T = (T-A1)/2;

        int R2 = T % 2;
        T = (T-R2)/2;
        int G2 = T % 2;
        T = (T-G2)/2;
        int B2 = T % 2;
        T = (T-B2)/2;
        int A2 = T % 2;
        T = (T-A2)/2;

        RGBApixel Pixel1 = *Image(i,j);
        Pixel1.Red += ( -Pixel1.Red%2 + R1 );
        Pixel1.Green += ( -Pixel1.Green%2 + G1 );
        Pixel1.Blue += ( -Pixel1.Blue%2 + B1 );
        Pixel1.Alpha += ( -Pixel1.Alpha%2 + A1 );
        *Image(i,j) = Pixel1;

        i++;
        if( i== Image.TellWidth() ){ i=0; j++; }

        RGBApixel Pixel2 = *Image(i,j);
        Pixel2.Red += ( -Pixel2.Red%2 + R2 );
        Pixel2.Green += ( -Pixel2.Green%2 + G2 );
        Pixel2.Blue += ( -Pixel2.Blue%2 + B2 );
        Pixel2.Alpha += ( -Pixel2.Alpha%2 + A2 );
        *Image(i,j) = Pixel2;

        i++; k++;
        if( i== Image.TellWidth() ){ i=0; j++; }
    }

    k=0;

    //encode the reset of the image using binary into the LSB of pixels in the cover image
    while( !feof( secret_file ) && k < 2*IH.FileSize )
    {
        char c;
        fread( &c , 1, 1, secret_file );

        unsigned int T = (unsigned int) c;

        int R1 = T % 2;
        T = (T-R1)/2;
        int G1 = T % 2;
        T = (T-G1)/2;
        int B1 = T % 2;
        T = (T-B1)/2;
        int A1 = T % 2;
        T = (T-A1)/2;

        int R2 = T % 2;
        T = (T-R2)/2;
        int G2 = T % 2;
        T = (T-G2)/2;
        int B2 = T % 2;
        T = (T-B2)/2;
        int A2 = T % 2;
        T = (T-A2)/2;

        RGBApixel Pixel1 = *Image(i,j);
        Pixel1.Red += ( -Pixel1.Red%2 + R1 );
        Pixel1.Green += ( -Pixel1.Green%2 + G1 );
        Pixel1.Blue += ( -Pixel1.Blue%2 + B1 );
        Pixel1.Alpha += ( -Pixel1.Alpha%2 + A1 );
        *Image(i,j) = Pixel1;

        i++; k++;
        if( i== Image.TellWidth() ){ i=0; j++; }

        RGBApixel Pixel2 = *Image(i,j);
        Pixel2.Red += ( -Pixel2.Red%2 + R2 );
        Pixel2.Green += ( -Pixel2.Green%2 + G2 );
        Pixel2.Blue += ( -Pixel2.Blue%2 + B2 );
        Pixel2.Alpha += ( -Pixel2.Alpha%2 + A2 );
        *Image(i,j) = Pixel2;


        i++; k++;
        if( i== Image.TellWidth() ){ i=0; j++; }
    }

    fclose( secret_file );
    Image.WriteToFile( output_filename);

    return 0;
}

//decodes an image with hidden data to create a secret image
int decode(char* stego_bmp, char* secret_filename) {
    BMP Image;

    Image.ReadFromFile( stego_bmp );
    if( Image.TellBitDepth() != 32 )
    {
        cout << "Error: File " << stego_bmp << " not encoded with this program." << endl;
        return 1;
    }

    EasyBMPstegoInternalHeader IH;
    IH.InitializeFromImage( Image );
    if( IH.FileSize == 0 || IH.FileNameSize == 0 || IH.NumberOfCharsToEncode == 0 )
    {
        cout << "No hiddent data detected. Exiting ... " << endl;
        return -1;
    }

    cout << "Hidden data detected! Outputting to file " << IH.FileName << " ... " << endl;

    snprintf(secret_filename, 1024, "%s", IH.FileName);

    FILE* fp;
    fp = fopen( IH.FileName , "wb" );
    if( !fp )
    {
        cout << "Error: Unable to open file " << IH.FileName << " for output!\n";
        return -1;
    }

    int MaxNumberOfPixels = Image.TellWidth() * Image.TellHeight();

    int k=0;
    int i=0;
    int j=0;

    // set the starting pixel to skip the internal header
    i = 2*IH.NumberOfCharsToEncode;
    while( i >= Image.TellWidth() )
    {
        i -= Image.TellWidth(); j++;
    }

    while( k < 2*IH.FileSize )
    {
        // read the two pixels

        RGBApixel Pixel1 = *Image(i,j);
        i++; k++;
        if( i == Image.TellWidth() ){ i=0; j++; }

        RGBApixel Pixel2 = *Image(i,j);
        i++; k++;
        if( i == Image.TellWidth() ){ i=0; j++; }

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

        fwrite( &c , 1 , 1 , fp );
    }
    fclose( fp );
    return 0;
}


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
