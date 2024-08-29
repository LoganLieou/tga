#ifndef TGA_H
#define TGA_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
struct TGAHeader {
    char id_length;
    char color_map_type;
    char data_type_code;
    short color_map_origin;
    short color_map_length;
    char color_map_depth;
    short x_origin;
    short y_origin;
    short width;
    short height;
    char  bits_per_pixel;
    char  image_descriptor;
};
#pragma pack(pop)

struct TGAColor {
    unsigned char rgba[4];
    int bytes_pp;
};

struct TGAImage {
    unsigned char *data;
    int height, width, bytes_pp; // bytes per pixel
};

// DEPRECATED colors are always RGBA now
enum Format {
    GRAYSCALE = 1,
    RGB = 3,
    RGBA = 4
};

/* Constructors for the TGAColor struct */
struct TGAColor make_color_default();
struct TGAColor make_color_rgb(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
struct TGAColor make_color_copy(struct TGAColor *color);

/* Constructors for the TGAImage struct */
struct TGAImage make_image_default();
struct TGAImage make_image_size(int width, int height, int bytes_pp);
struct TGAImage make_image_copy(struct TGAImage *image);

/* RLE Loads */
bool load_rle_data(struct TGAImage *image, FILE *in);
bool unload_rle_data(struct TGAImage *image, FILE *out);

/* File IO */
bool read_tga_file(struct TGAImage *image, char *filename);
bool write_tga_file(struct TGAImage *image, char *filename, bool rle);

/* Transform the Image */
bool flip_h(struct TGAImage *image);
bool flip_v(struct TGAImage *image);
bool scale(struct TGAImage *image, int h, int w);

/* Assign pixel values in TGAImage */
struct TGAColor get(struct TGAImage *image, int x, int y);
void set(struct TGAImage *image, int x, int y, struct TGAColor color);

/* Misc operations for TGAImage */
unsigned char *buffer(struct TGAImage *image);
void clear(struct TGAImage *image);

#endif // TGA_H
