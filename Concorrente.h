// Funções exportadas do concorrente
ImagemPPM* lerImagem(const char* nomeArquivo);
void salvarImagem(const char* nomeArquivo, ImagemPPM *img);
void aplicarFiltroConcorrente(ImagemPPM *img, int escolha, int nthreads);
void liberarImagem(ImagemPPM *img);