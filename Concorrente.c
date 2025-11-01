#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

ImagemPPM *imagem;
int escolha;

typedef struct {
   //numero de linhas da imagem
   long int nlinhas;
   //numero de threads 
   short int nthreads;
   //identificador da thread
   short int id;
} t_args;  

typedef struct {
    unsigned char r, g, b;  // cada canal vai de 0 a 255
} Pixel;

typedef struct {
    int largura, altura;
    Pixel **pixels;
} ImagemPPM;

void* convolução(void* args) {
    t_args *arg = (t_args*) args; //argumentos da thread
    int ini, fim, fatia; //auxiliares para divisao do vetor em blocos
    fatia = arg->nlinhas / arg->nthreads;
    ini = arg->id * fatia;
    if (arg->id == arg->nthreads - 1) fim = arg->nlinhas; //ultima thread pega o resto
    else fim = ini + fatia;
    
    // Aplicar o filtro escolhido na fatia da imagem
    for (int y = ini; y < fim; y++) {
        for (int x = 0; x < imagem->largura; x++) {
            imagem->pixels[y][x] = aplicarFiltro(imagem->pixels, x, y, imagem->largura, imagem->altura, escolha);
        }
    }

    pthread_exit(NULL);
}

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

    // Ignorar comentários no arquivo ppm
    int c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n');
        c = getc(fp);
    }
    ungetc(c, fp);

    ImagemPPM *img = malloc(sizeof(ImagemPPM));
    fscanf(fp, "%d %d", &img->largura, &img->altura);

    int max_val; // valor maximo de cor rgb
    fscanf(fp, "%d", &max_val);
    fgetc(fp); // consumir o \n depois do max_val

    img->pixels = malloc(img->altura * sizeof(Pixel *));
    for (int i = 0; i < img->altura; i++) {
        img->pixels[i] = malloc(img->largura * sizeof(Pixel));
        fread(img->pixels[i], sizeof(Pixel), img->largura, fp);
    }

    fclose(fp);
    return img;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <imagem_entrada> <arquivo_saida> <nthreads>\n", argv[0]);
        return 1;
    }

    char *imagem_entrada = argv[1];
    char *imagem_saida = argv[2];
    int nthreads = atoi(argv[3]);
    pthread_t *threads;
    t_args *args;

    // Carregar a imagem PPM
    imagem = lerImagem(imagem_entrada);
    if (imagem == NULL) {
        return 1;
    }

    // Alocar memória para threads e argumentos
    threads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
    args = (t_args*) malloc(nthreads * sizeof(t_args));

    printf("Digite o numero do filtro a ser aplicado:\n");
    printf("1. Blur\n");
    printf("2. Sharpen\n");
    printf("3. Edge Detection\n");
    scanf("%d", &escolha);  


    // Criar threads
    for (int i = 0; i < nthreads; i++) {
        args[i].nlinhas = imagem->altura; // Exemplo: dividir por linhas
        args[i].nthreads = nthreads;
        args[i].id = i;
        pthread_create(&threads[i], NULL, convolução, (void*) &args[i]);
    }

    // Esperar todas as threads terminarem
    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Salvar a imagem processada
    // (Implementar a função de salvamento da imagem aqui)

    free(threads);
    free(args);
    // Liberar memória da imagem
    // (Implementar a liberação da memória aqui)

    return 0;
}