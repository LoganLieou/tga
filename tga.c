#include "tga.h"

struct TGAColor make_color_default() {
    struct TGAColor color;
    color.val = 0;
    color.bytes_pp = 1;

    return color;
}

struct TGAColor make_color_rgb(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    struct TGAColor color;

    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;

    color.bytes_pp = 4;

    return color;
}

struct TGAColor make_color_vals(int val, int bytes_pp) {
    struct TGAColor color;

    color.val = val;
    color.bytes_pp = bytes_pp;

    return color;
}

struct TGAColor make_color_copy(struct TGAColor *other_color) {
    struct TGAColor color;

    color.val = other_color->val;
    color.bytes_pp = other_color->bytes_pp;

    return color;
}

struct TGAColor make_color_p(unsigned char *p, int bytes_pp) {
    struct TGAColor color;

    // We know for sure that this will be <= 4 which is the allocated size of
    // the raw array
    for (int i = 0; i < bytes_pp; ++i)
        color.raw[i] = p[i];

    return color;
}

void assign_color(struct TGAColor *image, struct TGAColor *other_image) {
    if (image != other_image) {
        image->bytes_pp = other_image->bytes_pp;
        image->val = other_image->val;
    }
}

struct TGAImage make_image_default() {
    struct TGAImage image;

    image.data = NULL;

    image.width = 0;
    image.height = 0;

    image.bytes_pp = 0;

    return image;
}

struct TGAImage make_image_size(int width, int height, int bytes_pp) {
    struct TGAImage image;

    unsigned long n = width * height * bytes_pp;
    memset(image.data, 0, n); // hope this works

    return image;
}

struct TGAImage make_image_copy(struct TGAImage *other_image) {
    struct TGAImage image;

    image.width = other_image->width;
    image.height = other_image->height;
    image.bytes_pp = other_image->bytes_pp;

    unsigned long n = image.width * image.height * image.bytes_pp;
    memcpy(&image.data, other_image->data, n); // hope this works

    return image;
}

void assign_image(struct TGAImage *image, struct TGAImage *other_image) {
    if (image != other_image) {

        // really want to test this do we need a deep copy or can you just
        // move the pointer like this?
        image = other_image;
    }
}

// TODO verify
void load_rle_data(struct TGAImage *image, FILE *in) {
	unsigned long pixel_count = image->width * image->height;
	unsigned long current_pixel = 0;
	unsigned long current_byte  = 0;
	struct TGAColor color_buffer;
	do {
		unsigned char chunk_header = 0;
		chunk_header = fgetc(in);
		if (chunk_header<128) {
			chunk_header++;
			for (int i=0; i < chunk_header; ++i) {
				fread((char *)color_buffer.raw, image->bytes_pp, sizeof(color_buffer.raw), in);
				for (int t = 0; t < image->bytes_pp; ++t)
					image->data[current_byte++] = color_buffer.raw[t];
				current_pixel++;
				if (current_pixel > pixel_count) {
                    fprintf(stderr, "Error: too many pixels!\n");
					return false;
				}
			}
		} else {
			chunk_header -= 127;
			fread((char *)color_buffer.raw, image->bytes_pp, sizeof(color_buffer.raw), in);
			for (int i = 0; i < chunk_header; i++) {
				for (int t = 0; t < image->bytes_pp; t++)
					image->data[current_byte++] = color_buffer.raw[t];
				current_pixel++;
				if (current_pixel > pixel_count) {
                    fprintf(stderr, "Error: too many pixels!\n");
					return false;
				}
			}
		}
	} while (current_pixel < pixel_count);
	return true; 
}