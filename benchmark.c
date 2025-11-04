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
#define N_REPETICOES    100   // Quantidade de vezes que cada código será executado

// --- Nomes dos Binários ---
#define BIN_SEQ         "./filtro_seq_bin"
#define BIN_CONC        "./concorrente_bin"

/**
 * Função que mede o tempo de execução de um comando shell.
 * NOTA: O tempo medido é apenas o do comando em si, isolando chamadas de printf e gettimeofday externas.
 * @param comando O comando a ser executado.
 * @return O tempo de execução em segundos (double).
 */
double medir_tempo_execucao(const char *comando) {
    struct timeval start, end;
    double tempo_usado;
    int status;

    gettimeofday(&start, NULL);
    // Apenas a execução do comando está sendo cronometrada aqui.
    status = system(comando);
    gettimeofday(&end, NULL);

    if (status != 0) {
        // Log de erro, mas permitindo que o loop continue se possível
        fprintf(stderr, "ERRO: O comando '%s' falhou com status de saída %d.\n", comando, status);
        return -1.0; 
    }

    tempo_usado = (end.tv_sec - start.tv_sec) + 
                  (end.tv_usec - start.tv_usec) / 1000000.0;
    
    return tempo_usado;
}

/**
 * Função utilitária para calcular a média de um array de doubles.
 * @param tempos O array de tempos.
 * @param n O número de elementos no array.
 * @return A média dos tempos.
 */
double calcular_media(const double *tempos, int n) {
    double soma = 0.0;
    for (int i = 0; i < n; i++) {
        // Ignorar resultados negativos (erros) no cálculo da média
        if (tempos[i] >= 0) {
            soma += tempos[i];
        }
    }
    return soma / n;
}


int main(void) {
    double tempos_seq[N_REPETICOES];
    double tempos_conc[N_REPETICOES];
    double media_seq, media_conc;
    int vitorias_seq = 0;
    int vitorias_conc = 0;
    int empates = 0;
    char comando_seq[512];
    char comando_conc[512];

    printf("========================================================\n");
    printf("        COMPARAÇÃO DE DESEMPENHO ESTATÍSTICA EM C       \n");
    printf("========================================================\n");
    printf("Imagem de Teste: %s\n", IMAGEM_ENTRADA);
    printf("Threads Concorrentes: %d\n", N_THREADS);
    printf("Filtro Escolhido: %c\n", FILTRO_ESCOLHA);
    printf("** Número de Repetições: %d **\n", N_REPETICOES);


    // 1. Compilação
    printf("\n--- 🛠️  Compilando programas ---\n");
    if (system("gcc filtro_seq.c -o filtro_seq_bin -lm") != 0) {
        fprintf(stderr, "Falha na compilação do filtro_seq.c\n");
        return 1;
    }
    // Certifique-se de que o compilador suporta -pthread (para POSIX threads)
    if (system("gcc Concorrente.c -o concorrente_bin -pthread -lm") != 0) {
        fprintf(stderr, "Falha na compilação do Concorrente.c\n");
        return 1;
    }

    // Preparação dos comandos para o loop
    sprintf(comando_seq, "%s %s %s", BIN_SEQ, IMAGEM_ENTRADA, IMAGEM_SAIDA_SEQ);
    sprintf(comando_conc, "echo %c | %s %s %s %d", 
            FILTRO_ESCOLHA, BIN_CONC, IMAGEM_ENTRADA, IMAGEM_SAIDA_CONC, N_THREADS);
    
    printf("\n--- ⏱️  Executando %d Repetições ---\n", N_REPETICOES);

    // 2. Loop de Execução e Medição
    for (int i = 0; i < N_REPETICOES; i++) {
        // Execução Sequencial
        tempos_seq[i] = medir_tempo_execucao(comando_seq);
        
        // Execução Concorrente
        tempos_conc[i] = medir_tempo_execucao(comando_conc);
        
        // Contagem de Vitórias (apenas se ambos executaram sem erro)
        if (tempos_seq[i] >= 0 && tempos_conc[i] >= 0) {
            if (tempos_seq[i] < tempos_conc[i]) {
                vitorias_seq++;
            } else if (tempos_conc[i] < tempos_seq[i]) {
                vitorias_conc++;
            } else {
                empates++;
            }
        }

        // Exibição de progresso
        if ((i + 1) % 10 == 0 || i == N_REPETICOES - 1) {
            printf("  Repetição %d/%d concluída.\r", i + 1, N_REPETICOES);
            fflush(stdout); // Garante que o progresso seja exibido
        }
    }
    printf("\n"); // Nova linha após o progresso

    // 3. Análise de Resultados Estatísticos
    media_seq = calcular_media(tempos_seq, N_REPETICOES);
    media_conc = calcular_media(tempos_conc, N_REPETICOES);

    printf("\n========================================================\n");
    printf("            📊 ESTATÍSTICAS FINAIS (%d REPETIÇÕES)        \n", N_REPETICOES);
    printf("========================================================\n");

    // Médias
    printf("MÉDIA DE TEMPO DE EXECUÇÃO:\n");
    printf("  SEQUENCIAL (1 thread):   **%.6f segundos**\n", media_seq);
    printf("  CONCORRENTE (%d threads): **%.6f segundos**\n", N_THREADS, media_conc);
    printf("--------------------------------------------------------\n");

    // Contagem de Vitórias
    printf("COMPARAÇÃO (Vitórias por Rodada):\n");
    printf("  Sequencial foi melhor:   %d vezes\n", vitorias_seq);
    printf("  Concorrente foi melhor:  %d vezes\n", vitorias_conc);
    printf("  Empates/Erros:           %d vezes\n", empates + (N_REPETICOES - (vitorias_seq + vitorias_conc + empates)));
    printf("--------------------------------------------------------\n");

    // Conclusão Baseada na Média
    if (media_seq < media_conc) {
        printf("CONLUSÃO PELA MÉDIA: O código SEQUENCIAL foi, em média, o mais rápido.\n");
    } else if (media_conc < media_seq) {
        double speedup = media_seq / media_conc;
        printf("CONLUSÃO PELA MÉDIA: O código CONCORRENTE foi, em média, o mais rápido.\n");
        printf("Ganho de Velocidade (Speedup médio): **%.2fx** mais rápido.\n", speedup);
    } else {
        printf("CONLUSÃO PELA MÉDIA: As médias de tempo foram iguais (ou muito próximas).\n");
    }

    // 4. Limpeza
    printf("\n--- Limpeza de Binários ---\n");
    system("rm -f filtro_seq_bin concorrente_bin");
    
    return 0;
}