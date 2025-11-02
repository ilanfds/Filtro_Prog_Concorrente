#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
   long int nlinhas;   // número de linhas da imagem
   short int nthreads; // número de threads
   short int id;       // identificador da thread
} t_args;  

typedef struct {
    unsigned char r, g, b;  // cada canal de cor vai de 0 a 255
} Pixel;

typedef struct {
    int largura, altura;
    Pixel **pixels;
} ImagemPPM;

ImagemPPM *imagem;        // imagem original
ImagemPPM *imagemAux;     // imagem auxiliar para escrever resultados
int escolha;

// Função que aplica o filtro 3x3 em um pixel
Pixel aplicarFiltro(Pixel **pixels, int x, int y, int largura, int altura, int escolha) {
    Pixel resultado = {0, 0, 0};
    int filtro[3][3];
    switch (escolha) {
        case 1: // Blur
            filtro[0][0] = 1; filtro[0][1] = 1; filtro[0][2] = 1;
            filtro[1][0] = 1; filtro[1][1] = 1; filtro[1][2] = 1;
            filtro[2][0] = 1; filtro[2][1] = 1; filtro[2][2] = 1;
            break;
        case 2: // Sharpen
            filtro[0][0] = 0; filtro[0][1] = -1; filtro[0][2] = 0;
            filtro[1][0] = -1; filtro[1][1] = 5; filtro[1][2] = -1;
            filtro[2][0] = 0; filtro[2][1] = -1; filtro[2][2] = 0;
            break;
        case 3: // Edge Detection
            filtro[0][0] = -1; filtro[0][1] = -1; filtro[0][2] = -1;
            filtro[1][0] = -1; filtro[1][1] = 8; filtro[1][2] = -1;
            filtro[2][0] = -1; filtro[2][1] = -1; filtro[2][2] = -1;
            break;
        default:
            return pixels[y][x]; // Nenhum filtro aplicado
    }

    int somaR = 0, somaG = 0, somaB = 0;
    for (int fy = -1; fy <= 1; fy++) {
        for (int fx = -1; fx <= 1; fx++) {
            int imgX = x + fx;
            int imgY = y + fy;
            if (imgX >= 0 && imgX < largura && imgY >= 0 && imgY < altura) {
                somaR += pixels[imgY][imgX].r * filtro[fy + 1][fx + 1];
                somaG += pixels[imgY][imgX].g * filtro[fy + 1][fx + 1];
                somaB += pixels[imgY][imgX].b * filtro[fy + 1][fx + 1];
            }
        }
    }

    resultado.r = (somaR < 0) ? 0 : (somaR > 255) ? 255 : somaR;
    resultado.g = (somaG < 0) ? 0 : (somaG > 255) ? 255 : somaG;
    resultado.b = (somaB < 0) ? 0 : (somaB > 255) ? 255 : somaB;

    return resultado;
}

// Função executada por cada thread
void* convolucao(void* args) {
    t_args *arg = (t_args*) args;
    int ini, fim, fatia;
    fatia = arg->nlinhas / arg->nthreads;
    ini = arg->id * fatia;
    if (arg->id == arg->nthreads - 1) fim = arg->nlinhas;
    else fim = ini + fatia;

    for (int y = ini; y < fim; y++) {
        for (int x = 0; x < imagem->largura; x++) {
            imagemAux->pixels[y][x] = aplicarFiltro(imagem->pixels, x, y, imagem->largura, imagem->altura, escolha);
        }
    }

    pthread_exit(NULL);
}

// Ler imagem PPM formato P6
ImagemPPM* lerImagem(const char* nomeArquivo) {
    FILE *fp = fopen(nomeArquivo, "rb");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        return NULL;
    }

    char formato[3];
    if (fscanf(fp, "%2s", formato) != 1 || strcmp(formato, "P6") != 0) {
        fprintf(stderr, "Formato não suportado (somente P6)\n");
        fclose(fp);
        return NULL;
    }

    int c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n');
        c = getc(fp);
    }
    ungetc(c, fp);

    ImagemPPM *img = malloc(sizeof(ImagemPPM));
    fscanf(fp, "%d %d", &img->largura, &img->altura);

    int max_val;
    fscanf(fp, "%d", &max_val);
    fgetc(fp);

    img->pixels = malloc(img->altura * sizeof(Pixel *));
    for (int i = 0; i < img->altura; i++) {
        img->pixels[i] = malloc(img->largura * sizeof(Pixel));
        fread(img->pixels[i], sizeof(Pixel), img->largura, fp);
    }

    fclose(fp);
    return img;
}

// Salvar imagem PPM
void salvarImagem(const char* nomeArquivo, ImagemPPM *img) {
    FILE *fp = fopen(nomeArquivo, "wb");
    if (!fp) {
        perror("Erro ao criar arquivo de saída");
        return;
    }

    fprintf(fp, "P6\n%d %d\n255\n", img->largura, img->altura);
    for (int i = 0; i < img->altura; i++) {
        fwrite(img->pixels[i], sizeof(Pixel), img->largura, fp);
    }

    fclose(fp);
}

// Liberar memória da imagem
void liberarImagem(ImagemPPM *img) {
    if (!img) return;
    for (int i = 0; i < img->altura; i++) {
        free(img->pixels[i]);
    }
    free(img->pixels);
    free(img);
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <imagem_entrada> <arquivo_saida> <nthreads>\n", argv[0]);
        return 1;
    }

    char *imagem_entrada = argv[1];
    char *imagem_saida = argv[2];
    int nthreads = atoi(argv[3]);

    pthread_t *threads = malloc(nthreads * sizeof(pthread_t));
    t_args *args = malloc(nthreads * sizeof(t_args));

    // Carregar a imagem original
    imagem = lerImagem(imagem_entrada);
    if (!imagem) return 1;

    // Criar imagem auxiliar
    imagemAux = malloc(sizeof(ImagemPPM));
    imagemAux->largura = imagem->largura;
    imagemAux->altura = imagem->altura;
    imagemAux->pixels = malloc(imagemAux->altura * sizeof(Pixel *));
    for (int i = 0; i < imagemAux->altura; i++)
        imagemAux->pixels[i] = malloc(imagemAux->largura * sizeof(Pixel));

    printf("Digite o numero do filtro a ser aplicado:\n");
    printf("1. Blur\n");
    printf("2. Sharpen\n");
    printf("3. Edge Detection\n");
    scanf("%d", &escolha);  

    // Criar threads
    for (int i = 0; i < nthreads; i++) {
        args[i].nlinhas = imagem->altura;
        args[i].nthreads = nthreads;
        args[i].id = i;
        pthread_create(&threads[i], NULL, convolucao, (void*)&args[i]);
    }

    // Esperar threads
    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Copiar resultado da imagem auxiliar para a imagem principal
    for (int i = 0; i < imagem->altura; i++)
        for (int j = 0; j < imagem->largura; j++)
            imagem->pixels[i][j] = imagemAux->pixels[i][j];

    // Salvar a imagem processada
    salvarImagem(imagem_saida, imagem);

    // Liberar memória
    liberarImagem(imagem);
    liberarImagem(imagemAux);
    free(threads);
    free(args);

    return 0;
}
