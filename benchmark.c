#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h> // Para gettimeofday
#include <unistd.h>   // Para usleep (opcional)

// --- Variáveis de Configuração ---
#define IMAGEM_ENTRADA  "neymar_entrada.ppm"
#define IMAGEM_SAIDA_SEQ "neymar_saida_seq.ppm"
#define IMAGEM_SAIDA_CONC "neymar_saida_conc.ppm"
#define N_THREADS       3     // Número de threads para o teste concorrente
#define FILTRO_ESCOLHA  '1'   // 1: Blur, 2: Sharpen, 3: Edge Detection

// --- Nomes dos Binários ---
#define BIN_SEQ         "./filtro_seq_bin"
#define BIN_CONC        "./concorrente_bin"

/**
 * Função que mede o tempo de execução de um comando shell.
 * @param comando O comando a ser executado.
 * @return O tempo de execução em segundos (double).
 */
double medir_tempo_execucao(const char *comando) {
    struct timeval start, end;
    double tempo_usado;
    int status;

    printf("\n> Comando: %s\n", comando);

    gettimeofday(&start, NULL);
    status = system(comando);
    gettimeofday(&end, NULL);

    if (status != 0) {
        fprintf(stderr, "ERRO: O comando falhou com status de saída %d.\n", status);
        return -1.0; // Retorna um valor negativo em caso de erro
    }

    tempo_usado = (end.tv_sec - start.tv_sec) + 
                  (end.tv_usec - start.tv_usec) / 1000000.0;
    
    printf("  Tempo de parede: %.6f segundos.\n", tempo_usado);
    return tempo_usado;
}

int main(void) {
    double tempo_seq, tempo_conc;
    char comando[512];

    printf("========================================================\n");
    printf("              COMPARAÇÃO DE DESEMPENHO EM C             \n");
    printf("========================================================\n");
    printf("Imagem de Teste: %s\n", IMAGEM_ENTRADA);
    printf("Threads Concorrentes: %d\n", N_THREADS);
    printf("Filtro Escolhido: %c (Blur/Sharpen/Edge)\n", FILTRO_ESCOLHA);

    // 1. Compilação
    printf("\n--- 🛠️  Compilando programas ---\n");
    if (system("gcc filtro_seq.c -o filtro_seq_bin -lm") != 0) {
        fprintf(stderr, "Falha na compilação do filtro_seq.c\n");
        return 1;
    }
    if (system("gcc Concorrente.c -o concorrente_bin -pthread -lm") != 0) {
        fprintf(stderr, "Falha na compilação do Concorrente.c\n");
        return 1;
    }

    // 2. Execução Sequencial
    printf("\n--- ⏱️  Execução Sequencial ---\n");
    // O filtro_seq.c usa um kernel fixo (Blur).
    sprintf(comando, "%s %s %s", BIN_SEQ, IMAGEM_ENTRADA, IMAGEM_SAIDA_SEQ);
    tempo_seq = medir_tempo_execucao(comando);
    if (tempo_seq < 0) return 1;

    // 3. Execução Concorrente
    printf("\n--- ⏱️  Execução Concorrente (%d Threads) ---\n", N_THREADS);
    // O concorrente.c recebe o filtro por stdin, que é simulado com 'echo' e 'pipe'
    sprintf(comando, "echo %c | %s %s %s %d", 
            FILTRO_ESCOLHA, BIN_CONC, IMAGEM_ENTRADA, IMAGEM_SAIDA_CONC, N_THREADS);
    tempo_conc = medir_tempo_execucao(comando);
    if (tempo_conc < 0) return 1;

    // 4. Análise de Resultados
    printf("\n========================================================\n");
    printf("               🏆 RESULTADO FINAL DA COMPARAÇÃO          \n");
    printf("========================================================\n");
    printf("SEQUENCIAL (1 thread):   %.6f segundos\n", tempo_seq);
    printf("CONCORRENTE (%d threads): %.6f segundos\n", N_THREADS, tempo_conc);
    printf("--------------------------------------------------------\n");

    if (tempo_seq < tempo_conc) {
        printf("Conclusão: O código SEQUENCIAL foi o mais rápido.\n");
    } else if (tempo_conc < tempo_seq) {
        double speedup = tempo_seq / tempo_conc;
        printf("Conclusão: O código CONCORRENTE foi o mais rápido.\n");
        printf("Ganho de Velocidade (Speedup): %.2fx mais rápido.\n", speedup);
    } else {
        printf("Conclusão: Os tempos de execução foram iguais (ou muito próximos).\n");
    }

    // 5. Limpeza
    printf("\n--- Limpeza de Binários ---\n");
    system("rm -f filtro_seq_bin concorrente_bin");
    
    return 0;
}