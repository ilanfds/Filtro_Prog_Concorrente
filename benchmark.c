#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "filtro_seq.h"
#include "concorrente.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <imagem_entrada.ppm> <nthreads>\n", argv[0]);
        return 1;
    }

    char *imagem_entrada = argv[1];
    int nthreads = atoi(argv[2]);

    // ---------- Teste Sequencial ----------
    Image *img_seq = load_ppm(imagem_entrada);
    if (!img_seq) return 1;

    float blur_kernel[3][3] = {
        {1.0/9, 1.0/9, 1.0/9},
        {1.0/9, 1.0/9, 1.0/9},
        {1.0/9, 1.0/9, 1.0/9}
    };

    clock_t start_seq = clock();
    apply_convolution(img_seq, blur_kernel);
    clock_t end_seq = clock();
    double tempo_seq = (double)(end_seq - start_seq) / CLOCKS_PER_SEC;

    save_ppm("saida_seq.ppm", img_seq);
    printf("Sequencial: %.6f segundos\n", tempo_seq);

    free(img_seq->data);
    free(img_seq);

    // ---------- Teste Concorrente ----------
    ImagemPPM *img_conc = lerImagem(imagem_entrada);
    if (!img_conc) return 1;

    clock_t start_conc = clock();
    aplicarFiltroConcorrente(img_conc, 1, nthreads); // 1 = Blur
    clock_t end_conc = clock();
    double tempo_conc = (double)(end_conc - start_conc) / CLOCKS_PER_SEC;

    salvarImagem("saida_conc.ppm", img_conc);
    printf("Concorrente (%d threads): %.6f segundos\n", nthreads, tempo_conc);

    liberarImagem(img_conc);

    return 0;
}
