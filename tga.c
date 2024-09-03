#include "tga.h"

struct TGAColor make_color_default() {
    struct TGAColor color;

    color.bytes_pp = 4;

	// TODO bgra instead so rgba values are reversed this needs to be fixed
	for (short i = 0; i < 4; ++i)
		color.rgba[i] = 0;

    return color;
}

struct TGAColor make_color_rgb(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    struct TGAColor color;

    color.bytes_pp = 4;

	color.rgba[0] = r;
	color.rgba[1] = g;
	color.rgba[2] = b;
	color.rgba[3] = a;

    return color;
}

struct TGAColor make_color_copy(struct TGAColor *other_color) {
    struct TGAColor color;

    color.bytes_pp = other_color->bytes_pp;
	for (short i = 0; i < color.bytes_pp; ++i)
		color.rgba[i] = other_color->rgba[i];

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

	image.width = width;
	image.height = height;
	image.bytes_pp = bytes_pp;

    unsigned long n = width * height * bytes_pp;

	image.data = (unsigned char *)malloc(n);
    memset(image.data, 0, n);

	// make sure the background is visible this comes with using RGBA now
	for (int i = 3; i < n; i += 4)
		image.data[i] = 255; // setting the alpha

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
				fread((char *)color_buffer.rgba, image->bytes_pp, sizeof(color_buffer.rgba), in);
				for (int t = 0; t < image->bytes_pp; ++t)
					image->data[current_byte++] = color_buffer.rgba[t];
				current_pixel++;
				if (current_pixel > pixel_count) {
                    fprintf(stderr, "Error: too many pixels!\n");
					return false;
				}
			}
		} else {
			chunk_header -= 127;
			fread((char *)color_buffer.rgba, image->bytes_pp, sizeof(color_buffer.rgba), in);
			for (int i = 0; i < chunk_header; i++) {
				for (int t = 0; t < image->bytes_pp; t++)
					image->data[current_byte++] = color_buffer.rgba[t];
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
            fprintf(stderr, "Error: cannot dump tga file\n");
			return false;
		}

        fwrite((char *)image->data + chunk_start,
            (raw ? run_length * image->bytes_pp : image->bytes_pp),
            sizeof((char *)image->data + chunk_start),
            out
        ); // hopefully this works

		if (ferror(out)) {
            fprintf(stderr, "Error: cannot dump tga file\n");
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
		flip_v(image);
	}
	if (header.image_descriptor & 0x10) {
		flip_h(image);
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

	FILE *out = fopen(filename, "wb");
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
	header.data_type_code = (image->bytes_pp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
	header.image_descriptor = 0x20; // top-left origin
	fwrite((char *)&header, 1, sizeof(header), out);
	if (ferror(out)) {
		fclose(out);
		fprintf(stderr, "Error: cannot dump tga file\n");
		return false;
	}
	if (!rle) {
		fwrite((char *)image->data, 1, image->width * image->height * image->bytes_pp, out);
		if (ferror(out)) {
			fclose(out);
			fprintf(stderr, "Error: cannot unload raw data\n");
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

bool scale(struct TGAImage *image, int w, int h) {
	if (w <= 0 || h <= 0 || !image->data) 
		return false;

	int n = w * h * image->bytes_pp;
	unsigned char *t_data = (unsigned char *)malloc(n);

	int n_scan_line = 0;
	int o_scan_line = 0;
	int erry = 0;
	unsigned long n_line_bytes = w * image->bytes_pp;
	unsigned long o_line_bytes = image->width * image->bytes_pp;

	for (int j = 0; j < image->height; ++j) {
		int errx = image->width - w;
		int nx   = -image->bytes_pp;
		int ox   = -image->bytes_pp;
		for (int i = 0; i < image->width; i++) {
			ox += image->bytes_pp;
			errx += w;
			while (errx>=(int)image->width) {
				errx -= image->width;
				nx += image->bytes_pp;
				memcpy(t_data + n_scan_line+nx, image->data+o_scan_line+ox, image->bytes_pp);
			}
		}
		erry += h;
		o_scan_line += o_line_bytes;
		while (erry >= (int)image->height) {
			if (erry >= (int)image->height << 1)
				memcpy(t_data + n_scan_line + n_line_bytes, t_data + n_scan_line, n_line_bytes);
			erry -= image->height;
			n_scan_line += n_line_bytes;
		}
	}
	free(image->data);
	image->data = t_data;
	image->width = w;
	image->height = h;
	return true;
}

bool flip_h(struct TGAImage *image) {
	if (image->data == NULL) 
		return false;

	int half = image->width >> 1;
	for (int i=0; i < half; i++) {
		for (int j=0; j < image->height; j++) {
			struct TGAColor c1 = get(image, i, j);
			struct TGAColor c2 = get(image, image->width-1-i, j);
			set(image, i, j, c2);
			set(image, image->width-1-i, j, c1);
		}
	}
	return true;
}

bool flip_v(struct TGAImage *image) {
	if (image->data == NULL) 
		return false;
	unsigned long bytes_per_line = image->width * image->bytes_pp;
	unsigned char *line = (unsigned char *)malloc(bytes_per_line);
	int half = image->height >> 1;
	for (int j = 0; j < half; ++j) {
		unsigned long l1 = j * bytes_per_line;
		unsigned long l2 = (image->height-1-j) * bytes_per_line;
		memmove((void *)line, (void *)(image->data + l1), bytes_per_line);
		memmove((void *)(image->data + l1), (void *)(image->data + l2), bytes_per_line);
		memmove((void *)(image->data + l2), (void *)line, bytes_per_line);
	}
	free(line);
	return true;
}

struct TGAColor get(struct TGAImage *image, int x, int y) {
	struct TGAColor color = make_color_default();
	if (!image->data || x < 0 || y < 0 || x >= image->width || y >= image->height)
		return color;

	// offset
	// that chunk of data will be the pixel information
	color.bytes_pp = image->bytes_pp;
	unsigned char *dat = image->data + (x + y * image->width) * image->bytes_pp;
	for (short i = 0; i < image->bytes_pp; ++i)
		color.rgba[i] = dat[i];

	return color;
}

void set(struct TGAImage *image, int x, int y, struct TGAColor color) {
	if (!image->data || x < 0 || y < 0 || x >= image->width || y >= image->height) {
		fprintf(stderr, "could not set values\n");
		return;
	}

	memcpy(image->data + (x + y * image->width) * image->bytes_pp, color.rgba, image->bytes_pp);
}

unsigned char *buffer(struct TGAImage *image) {
	return image->data;
}

void clear(struct TGAImage *image) {
	memset((void *)image->data, 0, image->width * image->height * image->bytes_pp);
}
