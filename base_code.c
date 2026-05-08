///////////////////////////////////////////////////////////////////////
// baseline, use only for loops and not any other optimization
// 2D conv on a 24-bit RGP .bmp image
// 
// different kernels:
// no idea yet for the images
//
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#pragma pack(push, 1)
// Bitmap File Header:
typedef struct {
    /* File header (14 bytes) */
    uint16_t signature;         /* offset  0: "BM" = 0x4D42                  */
    uint32_t fileSize;          /* offset  2: total file size in bytes        */
    uint32_t reserved;          /* offset  6: unused, must be 0               */
    uint32_t dataOffset;        /* offset 10: byte offset to pixel data       */
    /* DIB header / BITMAPINFOHEADER (40 bytes) */
    uint32_t headerSize;        /* offset 14: size of this header (40)        */
    int32_t  width;             /* offset 18: image width in pixels           */
    int32_t  height;            /* offset 22: image height (negative=top-down)*/
    uint16_t planes;            /* offset 26: must be 1                       */
    uint16_t bitsPerPixel;      /* offset 28: 24 for RGB                      */
    uint32_t compression;       /* offset 30: 0 = BI_RGB (uncompressed)       */
    uint32_t imageSize;         /* offset 34: size of pixel data              */
    int32_t  xPixelsPerMeter;   /* offset 38: horizontal resolution           */
    int32_t  yPixelsPerMeter;   /* offset 42: vertical resolution             */
    uint32_t colorsUsed;        /* offset 46: colors in color table           */
    uint32_t colorsImportant;   /* offset 50: important colors                */
} BMPHeader;                    /* total: 54 bytes                            */
#pragma pack(pop)

// Image Struct
// RGB images have 3 channels, so divide to r,g, and b arrays
// For every (x,y) pixel, index[y * width + x] in each array
typedef struct{
    int width, height;
    uint8_t *r, *g, *b; // split to one array per channel
} Img;

// Load the BMP

Img *load_bmp(const char *filename){
    FILE *f = fopen(filename, "rb"); // rb means read binary
    if (!f) {
        perror("fopen");
        return NULL;
    }

    BMPHeader bmp_hdr;
    fread(&bmp_hdr, sizeof(bmp_hdr), 1, f);
    if(bmp_hdr.signature != 0x424D || bmp_hdr.bitsPerPixel != 24){
        // specs say that header field is us 0x42 0x4D in hex 
        fprintf(stderr, "Need a 24-bit BMP .\n");
        fclose(f);
        return NULL;
    }

}
