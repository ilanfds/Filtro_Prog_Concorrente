#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_FILENAME 256
#define MAX_LINE 256

typedef struct {
    int width;
    int height;
    int max_value;
    unsigned char *data; // RGB intercalado
} Image;

//Carrega Imagem
Image *load_ppm(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Erro ao abrir imagem");
        return NULL;
    }

    Image *img = malloc(sizeof(Image));
    char type[3];
    fscanf(f, "%2s", type);
    if (strcmp(type, "P6") != 0) {
        fprintf(stderr, "Formato não suportado: %s\n", type);
        fclose(f);
        free(img);
        return NULL;
    }

    // Lê largura, altura e valor máximo, ignorando comentários
    int c;
    while ((c = fgetc(f)) == '#') while (fgetc(f) != '\n');
    ungetc(c, f);
    fscanf(f, "%d %d %d", &img->width, &img->height, &img->max_value);
    fgetc(f); // consome o '\n'

    img->data = malloc(3 * img->width * img->height);
    fread(img->data, 3, img->width * img->height, f);

    fclose(f);
    return img;
}

//Salvar imagem PPM 
void save_ppm(const char *filename, const Image *img) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Erro ao salvar imagem");
        return;
    }

    fprintf(f, "P6\n%d %d\n%d\n", img->width, img->height, img->max_value);
    fwrite(img->data, 3, img->width * img->height, f);
    fclose(f);
}

//Aplicar filtro (exemplo: filtro de desfoque 3x3)
void apply_convolution(Image *img, float kernel[3][3]) {
    int w = img->width;
    int h = img->height;
    unsigned char *output = malloc(3 * w * h);

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            float sumR = 0, sumG = 0, sumB = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int idx = 3 * ((y + ky) * w + (x + kx));
                    float k = kernel[ky + 1][kx + 1];
                    sumR += img->data[idx] * k;
                    sumG += img->data[idx + 1] * k;
                    sumB += img->data[idx + 2] * k;
                }
            }
            int out_idx = 3 * (y * w + x);
            output[out_idx] = (unsigned char) fmin(fmax(sumR, 0), 255);
            output[out_idx + 1] = (unsigned char) fmin(fmax(sumG, 0), 255);
            output[out_idx + 2] = (unsigned char) fmin(fmax(sumB, 0), 255);
        }
    }

    memcpy(img->data, output, 3 * w * h);
    free(output);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <imagem_entrada.ppm> <imagem_saida.ppm>\n", argv[0]);
        return 1;
    }

    Image *img = load_ppm(argv[1]);
    if (!img) return 1;

    // Exemplo de kernel de desfoque 3x3
    float blur_kernel[3][3] = {
        {1.0/9, 1.0/9, 1.0/9},
        {1.0/9, 1.0/9, 1.0/9},
        {1.0/9, 1.0/9, 1.0/9}
    };

    apply_convolution(img, blur_kernel);
    save_ppm(argv[2], img);

    free(img->data);
    free(img);
    printf("Filtro aplicado com sucesso!\n");
    return 0;
}
