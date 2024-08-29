#include "tga.h"

int main(void) {
    struct TGAColor white = make_color_rgb(255, 255, 255, 255);
    struct TGAColor red = make_color_rgb(255, 0, 0, 255);

    struct TGAImage image = make_image_size(100, 100, RGB);

    set(&image, 52, 41, red);
    flip_v(&image);

    write_tga_file(&image, "out.tga", true);
}