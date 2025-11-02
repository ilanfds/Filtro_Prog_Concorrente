#ifndef FILTRO_SEQ_H
#define FILTRO_SEQ_H

typedef struct {
    int width;
    int height;
    int max_value;
    unsigned char *data; // RGB intercalado
} Image;

// Funções exportadas do filtro sequencial
Image *load_ppm(const char *filename);
void save_ppm(const char *filename, const Image *img);
void apply_convolution(Image *img, float kernel[3][3]);

#endif
