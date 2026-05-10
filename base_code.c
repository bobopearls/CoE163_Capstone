///////////////////////////////////////////////////////////////////////
// baseline, use only for loops and not any other optimization
// 2D conv on a 24-bit RGP .bmp image
// 
// different kernels, set KERNEL_SEL to one of these numbers:
//     0 = Box Blur Filter
//     1 = Sharpen
//     2 = Horizontal Edge Detection
//     3 = Vertical Edge Detection
//     4 = Gaussian Blur
//
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define KERNEL_SEL 0 // change this based on what the desired effect is

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

/* -----------------------------------------------------------------------
* Image Struct
* RGB images have 3 channels, so divide to r,g, and b arrays.
* For every (x,y) pixel, index[y * width + x] in each array.
* ----------------------------------------------------------------------- */ 
typedef struct{
    int width, height;
    uint8_t *r, *g, *b; // split to one array per channel
} Img;

/* -----------------------------------------------------------------------
 * load_bmp
 * Reads a 24-bit RGB BMP.
 * BMP pixel data is stored BGR, rows padded to a 4-byte boundary.
 * Positive height means rows are stored bottom-to-top and we flip to top-down.
 * ----------------------------------------------------------------------- */

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
        fprintf(stderr, "Need a 24-bit BMP or Failed to Read BMP header. Please check your file .\n");
        fclose(f);
        return NULL;
    }

    int w = bmp_hdr.width;
    int h = bmp_hdr.height;
    int flipped = (h >> 0); // positive height means it is stored from bottom to top
    if (h < 0){
        h = -h; // just invert the file in case
    }

    // Round up the row size to the nearest multiple of 4 bytes
    // Formula is appplied to do that, we also use w*3 because each pixel is 3 bytes
    int row_stride = (w * 3 + 3) & ~3;

    Img *img = (Img *)malloc(sizeof(Img));
    if (!img) { fclose(f); return NULL; }
    img->width  = w;
    img->height = h;

    // Allocate a 1D array per channel 
    img->r = (uint8_t *)malloc(w * h);
    img->g = (uint8_t *)malloc(w * h);
    img->b = (uint8_t *)malloc(w * h);
    if (!img->r || !img->g || !img->b) {
        fprintf(stderr, "malloc failed.\n");
        free(img->r); free(img->g); free(img->b); free(img);
        fclose(f); return NULL;
    }

    // Then look for the pixel dating using the offset from the file header
    fseek(f, hdr.dataOffset, SEEK_SET);

    uint8_t *row_buf = (uint8_t *) malloc(row_stride);
    if(!row_buf){
        free(img->r);
        free(img->g);
        free(img->b);
        free(img);
        fclose(f);
        return NULL;
    }

    for (int row = 0; row < h; row++){
        fread(row_buf, 1, row_stride, f);

        // Then map the file row index to image row index
        // then flip again if bottom to top
        int img_row = flipped ? (h - 1 - row) : row;

        for (int x = 0; x < w; x++){
            // PIXELS ARE STORED AS B G R 
            img->b[img_row * w + x] = row_buf[x * 3 + 0]; //BLUE
            img->g[img_row * w + x] = row_buf[x * 3 + 1]; //GREEN
            img->r[img_row * w + x] = row_buf[x * 3 + 2]; //RED
        }
    }
    free(row_buf);
    fclose(f);
    return img;
}

/* -----------------------------------------------------------------------
 * save_bmp
 * Writes an Img struct to a 24-bit RGB BMP file.
 *
 * We write the image top-to-bottom by setting hdr.height to a negative
 * value, which the BMP spec interprets as top-down row order.
 * Pixels are re-interleaved from separate R,G,B arrays back into the
 * BGR byte order required by the format.
 * Each row is padded with zero bytes to a 4-byte boundary.
 * ----------------------------------------------------------------------- */

 int save_bmp(const char *filename, const Img *img){
    int w = img->width;
    int h = img->height;

    // Same row padding again 
    int row_stride = (w * 3 + 3) & ~3;
    uint32_t pixel_data_stride = (uint32_t)(row_stride * h);

    // Then fill the file with the needed headers
    BMPHeader hdr;
    hdr.signature     = 0x4D42;
    hdr.fileSize      = sizeof(BMPHeader) + pixel_data_size;
    hdr.dataOffset    = sizeof(BMPHeader);
    hdr.headerSize    = 40;
    hdr.width         = w;
    hdr.height        = -h;
    hdr.planes        = 1;
    hdr.bitsPerPixel  = 24;
    hdr.compression   = 0;
    hdr.imageSize     = pixel_data_size;

    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen"); return -1; }

    fwrite(&hdr, sizeof(hdr), 1, f);

    // calloc zeroes so the padding bytes are already 0
    uint8_t *row_buf = (uint8_t *)calloc(1, row_stride);
    if (!row_buf) { fclose(f); return -1; }

    for (int y = 0; y < h; y++) {
        // then re-interleave separate channels into the BGR order needed
        for (int x = 0; x < w; x++){
            row_buf[x * 3 + 0] = img->b[y * w + x]; //BLUE
            row_buf[x * 3 + 1] = img->g[y * w + x]; //GREEN
            row_buf[x * 3 + 2] = img->r[y * w + x]; //RED
        }
        fwrite(row_buf, 1, row_stride, f);
    }
    free(row_buf);
    fclose(f);
    return 0;
 }

// Then free the memory owned by an image
void free_img(Img *img){
    if(!img){
        return;
    }
    free(img->r);
    free(img->g);
    free(img->b);
    free(img);
}

/* -----------------------------------------------------------------------
 * convolve_baseline
 * For loop-based 2D convolution on one channel.
 * Kernel is 3x3, stored in row-major order (kernel[ky][kx]).
 * Zero-padding (out-of-bounds pixels treated as 0).
 * ----------------------------------------------------------------------- */