#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    uint32_t etiqueta;
    bool valido;
    bool sujo;
    int frequencia;
    int ultimo_uso;
} LinhaCache;

typedef struct {
    LinhaCache* linhas;
    int associatividade;
} ConjuntoCache;

typedef struct {
    ConjuntoCache* conjuntos;
    int num_conjuntos;
    int tamanho_linha;
    int tempo_acesso;
    int politica_escrita;
    char politica_substituicao[4];
    int tempo;
    int acertos;
    int faltas;
    int leituras;
    int escritas;
    int leituras_memoria_principal;
    int escritas_memoria_principal;
} Cache;

Cache* inicializar_cache(int num_linhas, int tamanho_linha, int associatividade, int politica_escrita, char* politica_substituicao, int tempo_acesso) {
    int escolha;
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    printf("Selecione o tipo de representacao do Numero de linhas da cache:\n");
    printf("0 - Numero de blocos\n");
    printf("1 - Tamanho total da Cache\n");
    scanf ("%d", &escolha);
    if (escolha){
        cache->num_conjuntos = num_linhas / (tamanho_linha*associatividade);
    }
    else {
        cache->num_conjuntos = num_linhas / associatividade;
    }
    cache->tamanho_linha = tamanho_linha;
    cache->tempo_acesso = tempo_acesso;
    cache->politica_escrita = politica_escrita;
    strcpy(cache->politica_substituicao, politica_substituicao);
    cache->tempo = 0;
    cache->acertos = 0;
    cache->faltas = 0;
    cache->leituras = 0;
    cache->escritas = 0;
    cache->leituras_memoria_principal = 0;
    cache->escritas_memoria_principal = 0;

    cache->conjuntos = (ConjuntoCache*)malloc(cache->num_conjuntos * sizeof(ConjuntoCache));
    for (int i = 0; i < cache->num_conjuntos; i++) {
        cache->conjuntos[i].linhas = (LinhaCache*)malloc(associatividade * sizeof(LinhaCache));
        cache->conjuntos[i].associatividade = associatividade;
        for (int j = 0; j < associatividade; j++) {
            cache->conjuntos[i].linhas[j].valido = false;
            cache->conjuntos[i].linhas[j].sujo = false;
            cache->conjuntos[i].linhas[j].frequencia = 0;
            cache->conjuntos[i].linhas[j].ultimo_uso = 0;
        }
    }
    return cache;
}

void liberar_cache(Cache* cache) {
    for (int i = 0; i < cache->num_conjuntos; i++) {
        free(cache->conjuntos[i].linhas);
    }
    free(cache->conjuntos);
    free(cache);
}

LinhaCache* obter_linha_substituicao(ConjuntoCache* conjunto, char* politica_substituicao, int tempo_atual) {
    LinhaCache* linha_substituicao = &conjunto->linhas[0];
    if (strcmp(politica_substituicao, "LFU") == 0) {
        for (int i = 1; i < conjunto->associatividade; i++) {
            if (conjunto->linhas[i].frequencia < linha_substituicao->frequencia) {
                linha_substituicao = &conjunto->linhas[i];
            }
        }
    } else if (strcmp(politica_substituicao, "LRU") == 0) {
        for (int i = 1; i < conjunto->associatividade; i++) {
            if (conjunto->linhas[i].ultimo_uso < linha_substituicao->ultimo_uso) {
                linha_substituicao = &conjunto->linhas[i];
            }
        }
    } else if (strcmp(politica_substituicao, "RND") == 0) {
        linha_substituicao = &conjunto->linhas[rand() % conjunto->associatividade];
    }
    return linha_substituicao;
}

int acessar_cache(Cache* cache, uint32_t endereco, char operacao) {
    uint32_t indice = (endereco / cache->tamanho_linha) % cache->num_conjuntos;
    uint32_t etiqueta = endereco / (cache->tamanho_linha * cache->num_conjuntos);
    ConjuntoCache* conjunto = &cache->conjuntos[indice];
    cache->tempo++;

    for (int i = 0; i < conjunto->associatividade; i++) {
        if (conjunto->linhas[i].valido && conjunto->linhas[i].etiqueta == etiqueta) {
            cache->acertos++;
            if (operacao == 'W') {
                conjunto->linhas[i].sujo = true;
            }
            conjunto->linhas[i].frequencia++;
            conjunto->linhas[i].ultimo_uso = cache->tempo;
            return cache->tempo_acesso;
        }
    }

    cache->faltas++;
    if (operacao == 'R') {
        cache->leituras++;
        cache->leituras_memoria_principal++;
    } else if (operacao == 'W') {
        cache->escritas++;
        if (cache->politica_escrita == 0) {
            cache->escritas_memoria_principal++;
        }
    }

    LinhaCache* linha_substituicao = obter_linha_substituicao(conjunto, cache->politica_substituicao, cache->tempo);
    if (linha_substituicao->valido && linha_substituicao->sujo && cache->politica_escrita == 1) {
        cache->escritas_memoria_principal++;
    }

    linha_substituicao->etiqueta = etiqueta;
    linha_substituicao->valido = true;
    linha_substituicao->sujo = operacao == 'W';
    linha_substituicao->frequencia = 1;
    linha_substituicao->ultimo_uso = cache->tempo;

    return cache->tempo_acesso;
}

void ler_parametros(const char* caminho_arquivo, int* parametros, char* politica_substituicao) {
    FILE* arquivo = fopen(caminho_arquivo, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Não é possível abrir o arquivo de parâmetros.\n");
        exit(EXIT_FAILURE);
    }
    fscanf(arquivo, "%d %d %d %d %d %s %d %d", &parametros[0], &parametros[1], &parametros[2], &parametros[3], &parametros[4], politica_substituicao, &parametros[5], &parametros[6]);
    fclose(arquivo);
}

int ler_enderecos(const char* caminho_arquivo, uint32_t** enderecos, char** operacoes) {
    FILE* arquivo = fopen(caminho_arquivo, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Não é possível abrir o arquivo de endereços.\n");
        exit(EXIT_FAILURE);
    }
    int contagem = 0;
    uint32_t endereco;
    char operacao;
    while (fscanf(arquivo, "%x %c", &endereco, &operacao) != EOF) {
        contagem++;
    }
    rewind(arquivo);
    *enderecos = (uint32_t*)malloc(contagem * sizeof(uint32_t));
    *operacoes = (char*)malloc(contagem * sizeof(char));
    for (int i = 0; i < contagem; i++) {
        fscanf(arquivo, "%x %c", &(*enderecos)[i], &(*operacoes)[i]);
    }
    fclose(arquivo);
    return contagem;
}


void escrever_resultados(const char* caminho_arquivo, int* parametros, int contagem_enderecos, Cache* cache, char* politica_substituicao) {
    FILE* arquivo = fopen(caminho_arquivo, "w");
    if (arquivo == NULL) {
        fprintf(stderr, "Não é possível abrir o arquivo de resultados.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(arquivo, "Política de Escrita: %d\n", parametros[0]);
    fprintf(arquivo, "Tamanho da Linha: %d\n", parametros[1]);
    fprintf(arquivo, "Número de Linhas: %d\n", parametros[2]);
    fprintf(arquivo, "Associatividade: %d\n", parametros[3]);
    fprintf(arquivo, "Tempo de Acesso: %d\n", parametros[4]);
    fprintf(arquivo, "Política de Substituição: %s\n", politica_substituicao);
    fprintf(arquivo, "Tempo de Leitura da Memória Principal: %d\n", parametros[5]);
    fprintf(arquivo, "Tempo de Escrita da Memória Principal: %d\n", parametros[6]);
    fprintf(arquivo, "Total de Endereços: %d\n", contagem_enderecos);
    fprintf(arquivo, "Leituras: %d\n", cache->leituras);
    fprintf(arquivo, "Escritas: %d\n", cache->escritas);
    fprintf(arquivo, "Leituras da Memória Principal: %d\n", cache->leituras_memoria_principal);
    fprintf(arquivo, "Escritas da Memória Principal: %d\n", cache->escritas_memoria_principal);
    fprintf(arquivo, "Taxa de Acerto: %.4f\n", (double) cache->acertos / (cache->acertos + cache->faltas));
    fprintf(arquivo, "Tempo Médio de Acesso: %.4f ns\n", (double)(cache->tempo_acesso + (1 - (double) cache->acertos / (cache->acertos + cache->faltas)) * parametros [5]));
    fclose(arquivo);
}

int main() {
    srand(time(NULL));

    int parametros[8];
    char politica_substituicao[4];
    ler_parametros("params.txt", parametros, politica_substituicao);

    uint32_t* enderecos;
    char* operacoes;
    int contagem_enderecos = ler_enderecos("oficial.cache", &enderecos, &operacoes);
    Cache* cache = inicializar_cache(parametros[2], parametros[1], parametros[3], parametros[0], politica_substituicao, parametros[4]);

    for (int i = 0; i < contagem_enderecos; i++) {
        acessar_cache(cache, enderecos[i], operacoes[i]);
    }


    escrever_resultados("resultado.txt", parametros, contagem_enderecos, cache, politica_substituicao);

    free(enderecos);
    free(operacoes);
    liberar_cache(cache);

    return 0;
}
