// Last updated 2008/12/10 17:45

// From: http://www.imagemagick.org/discourse-server/viewtopic.php?f=1&t=12640
/*
    Read the logo: image and use PixelIterators to produce two new images.
    Convert the image to the HSB and HSL colourspaces, cut the Lightness/Brightness
    by half and then write the images as logo_hsb.jpg and logo_hsl.jpg
    Note the colour distortion in the HSL result whereas the HSB image is what
    we'd expect to see when reducing an image's brightness by 50%.
    
    As with my other examples, there is no error checking in this code.
    If you adapt this to your own use, you should add error checking to ensure
    that, for example, MagickReadImage succeeds and that the width and height
    of the image in mw are reasonable values.
*/
#include <windows.h>
#include <wand/magick_wand.h>
void test_wand(void) {
    MagickWand *mw,*mwl,*mwb;
    PixelIterator *imw,*imwl,*imwb;
    PixelWand **pmw,**pmwl,**pmwb;

    unsigned long y;
    register long x;
    unsigned long width,height;
    Quantum qr,qg,qb;
    Quantum qrb,qgb,qbb;
    Quantum qrl,qgl,qbl;
    double lh,ls,ll;
    double bh,bs,bb;

    MagickWandGenesis();
    mw = NewMagickWand();

    MagickReadImage(mw, "logo:");
    width = MagickGetImageWidth(mw);
    height = MagickGetImageHeight(mw);

    mwl = NewMagickWand();
    mwb = NewMagickWand();

    // Set the hsl and hsb images to the same size as the input image
    MagickSetSize(mwl,width,height);
    MagickSetSize(mwb,width,height);
    // Even though we won't be reading these images they must be initialized
    // to something
    MagickReadImage(mwb,"xc:none");
    MagickReadImage(mwl,"xc:none");

    // Create iterators for each image
    imw = NewPixelIterator(mw);
    imwl = NewPixelIterator(mwl);
    imwb = NewPixelIterator(mwb);
    for (y=0; y < height; y++) {
        // Get the next row from each image
        pmw = PixelGetNextIteratorRow(imw, &width);
        pmwl = PixelGetNextIteratorRow(imwl, &width);
        pmwb = PixelGetNextIteratorRow(imwb, &width);
        for (x=0; x < (long) width; x++) {
            // Get the RGB quanta from the source image
            qr = PixelGetRedQuantum(pmw[x]);
            qg = PixelGetGreenQuantum(pmw[x]);
            qb = PixelGetBlueQuantum(pmw[x]);

            // Convert the source quanta to HSB
            ConvertRGBToHSB(qr,qg,qb,&bh,&bs,&bb);
            bb *= 0.5;
            ConvertHSBToRGB(bh,bs,bb,&qrb,&qgb,&qbb);
            // Set the pixel in the HSB output image
            PixelSetRedQuantum(pmwb[x],qrb);
            PixelSetGreenQuantum(pmwb[x],qgb);
            PixelSetBlueQuantum(pmwb[x],qbb);

            // Convert the source quanta to HSL
            ConvertRGBToHSL(qr,qg,qb,&lh,&ls,&ll);
            ll *= 0.5;
            ConvertHSLToRGB(lh,ls,ll,&qrl,&qgl,&qbl);
            // Set the pixel in the HSL output image
            PixelSetRedQuantum(pmwl[x],qrl);
            PixelSetGreenQuantum(pmwl[x],qgl);
            PixelSetBlueQuantum(pmwl[x],qbl);
        }
        // Sync writes the pixels back to the magick wands
        PixelSyncIterator(imwl);
        PixelSyncIterator(imwb);
    }
    // write the results
    MagickWriteImage(mwb,"logo_hsb.jpg");
    MagickWriteImage(mwl,"logo_hsl.jpg");

    // Clean up the iterators and magick wands
    imw = DestroyPixelIterator(imw);
    imwl = DestroyPixelIterator(imwl);
    imwb = DestroyPixelIterator(imwb);
    mw = DestroyMagickWand(mw);
    mwl = DestroyMagickWand(mwl);
    mwb = DestroyMagickWand(mwb);
    MagickWandTerminus();
}
