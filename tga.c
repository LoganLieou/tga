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

// TODO verify
bool load_rle_data(struct TGAImage *image, FILE *in) {
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

bool unload_rle_data(struct TGAImage *image, FILE *out) {
	const unsigned char max_chunk_length = 128;
	unsigned long n = image->width * image->height;
	unsigned long cur = 0;

	while (cur < n) {
		unsigned long chunk_start = cur * image->bytes_pp;
		unsigned long cur_byte = cur * image->bytes_pp;
		unsigned char run_length = 1;
		bool raw = true;

		while (cur + run_length < n && run_length < max_chunk_length) {
			bool succ_eq = true;
			for (int t=0; succ_eq && t < image->bytes_pp; t++) {
				succ_eq = (image->data[cur_byte + t] == image->data[cur_byte+ t + image->bytes_pp]);
			}
			cur_byte += image->bytes_pp;
			if (1 == run_length) {
				raw = !succ_eq;
			}
			if (raw && succ_eq) {
				run_length--;
				break;
			}
			if (!raw && !succ_eq) {
				break;
			}
			run_length++;
		}
		cur += run_length;
        fputc(raw ? run_length - 1 : run_length + 127, out);
		if (ferror(out)) {
            printf(stderr, "Error: cannot dump tga file\n");
			return false;
		}

        fwrite((char *)image->data + chunk_start,
            (raw ? run_length * image->bytes_pp : image->bytes_pp),
            sizeof((char *)image->data + chunk_start),
            out
        ); // hopefully this works

		if (ferror(out)) {
            printf(stderr, "Error: cannot dump tga file\n");
			return false;
		}
	}
	return true;
}

bool read_tga_file(struct TGAImage *image, char *filename) {
	if (image->data)
        free(image->data);
	image->data = NULL;

    FILE *in;
    in = fopen(filename, "r");
	if (in == NULL) {
        fprintf(stderr, "Error: cannot open file\n");
        fclose(in);
		return false;
	}

	struct TGAHeader header;

    fread((char *) &header, 1, sizeof(header), in);

	image->width   = header.width;
	image->height  = header.height;
	image->bytes_pp = header.bits_per_pixel>>3;

	if (image->width <= 0 || image->height <= 0 || (image->bytes_pp != GRAYSCALE
        && image->bytes_pp!=RGB && image->bytes_pp!=RGBA)) {
            fclose(in);
            fprintf(stderr, "Error: bad headers\n");
            return 1;
	}

	unsigned long n = image->bytes_pp * image->width * image->height;

	image->data = (unsigned char *)malloc(n); // hopefully this works

	if (3 == header.data_type_code || 2 == header.data_type_code) {
        fread((char *)image->data, n, sizeof(image->data), in);
	} else if (10 == header.data_type_code || 11 == header.data_type_code) {
		if (!load_rle_data(image, in)) {
            fclose(in);
            fprintf(stderr, "Error: something went wrong wile reading the data\n");
			return false;
		}
	} else {
        fclose(in);
        fprintf(stderr, "Error: unknown file format %d\n", (int)header.data_type_code);
		return false;
	}
	if (!(header.image_descriptor & 0x20)) {
		flip_vertically();
	}
	if (header.image_descriptor & 0x10) {
		flip_horizontally();
	}
    printf("%dx%d/%d\n",
        image->width, image->height, image->bytes_pp * 8);
    fclose(in);
	return true;
}

bool write_tga_file(struct TGAImage *image, char *filename, bool rle) {
	unsigned char developer_area_ref[4] = {0, 0, 0, 0};
	unsigned char extension_area_ref[4] = {0, 0, 0, 0};
	unsigned char footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};

	FILE *out;
    out = fopen(filename, "b");
	if (out == NULL) {
        fprintf(stderr, "Error: cannot open file\n");
        fclose(out);
		return false;
	}

    struct TGAHeader header;
	memset((void *)&header, 0, sizeof(header));
	header.bits_per_pixel = image->bytes_pp << 3;
	header.width  = image->width;
	header.height = image->height;
	header.data_type_code = (image->bytes_pp == GRAYSCALE ? (rle ? 11 : 3) :(rle ? 10 : 2));
	header.image_descriptor = 0x20; // top-left origin

    fwrite((char *)&header, 1, sizeof(header), out); // hopefully this works

	if (ferror(out)) {
        fclose(out);
        fprintf(stderr, "Error: cannot dump tga file\n");
		return false;
	}
	if (!rle) {
        fwrite((char *)image->data, 1, 
            image->width * image->height * image->bytes_pp, out);
		if (ferror(out)) {
            fprintf(stderr, "Error: cannot unload raw data\n");
            fclose(out);
			return false;
		}
	} else {
		if (!unload_rle_data(image, out)) {
            fclose(out);
            fprintf(stderr, "Error: cannot unload rle data\n");
			return false;
		}
	}
    fwrite((char *)developer_area_ref, 1, sizeof(developer_area_ref), out);
	if (ferror(out)) {
        fprintf(stderr, "Error: cannot dump tga file\n");
        fclose(out);
		return false;
	}
    fwrite((char *)extension_area_ref, 1, sizeof(extension_area_ref), out);
	if (ferror(out)) {
        fprintf(stderr, "Error: cannot dump tga file\n");
        fclose(out);
		return false;
	}
    fwrite((char *)footer, 1, sizeof(footer), out);
	if (ferror(out)) {
        fprintf(stderr, "Error: cannot dump tga file\n");
        fclose(out);
		return false;
	}
    fclose(out);
	return true;
}