#ifndef TGA_H
#define TGA_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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

struct TGAColor {
    union {
        struct {
            unsigned char b, g, r, a;
        };
        unsigned char raw[4];
        unsigned int val;
    };
    int bytes_pp;
};

struct TGAImage {
    unsigned char *data;
    int height, width, bytes_pp; // bytes per pixel
};

enum Format {
    GRAYSCALE = 1,
    RGB = 3,
    RGBA = 4
};

/* Constructors for the TGAColor struct */
struct TGAColor make_color_default();
struct TGAColor make_color_rgb(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
struct TGAColor make_color_vals(int val, int bytes_pp);
struct TGAColor make_color_copy(struct TGAColor *color);
struct TGAColor make_color_p(unsigned char *p, int bytes_pp);

/* Misc operations for TGAColor */
void assign_color(struct TGAImage *image, struct TGAImage *other_image);

/* Constructors for the TGAImage struct */
struct TGAImage make_image_default();
struct TGAImage make_image_size(int width, int height, int bytes_pp);
struct TGAImage make_image_copy(struct TGAImage *image);


/* RLE Loads */
void load_rle_data(struct TGAImage *image, FILE *in);
void unload_rle_data(struct TGAImage *image, FILE *out);

/* File IO */
bool read_tga_file(struct TGAImage *image, char *filename);
bool write_tga_file(struct TGAImage *image, char *filename);

/* Transform the Image */
bool flip_h(struct TGAImage *image);
bool flip_v(struct TGAImage *image);
bool scale(struct TGAImage *image);

/* Misc operations for TGAImage */
unsigned char *buffer(struct TGAImage *image);
void clear(struct TGAImage *image);
void assign_image(struct TGAImage *image, struct TGAImage *other_image);

#endif // TGA_H
