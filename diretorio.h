#ifndef DIRETORIO_H
#define DIRETORIO_H

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

// Estrutura que representa um membro do archive
struct Membro {
    char nome[1024];         // Nome do arquivo (até 1024 bytes)
    uid_t uid;               // User ID
    unsigned int tam_orig;   // Tamanho original do arquivo
    unsigned int tam_disco;  // Tamanho no disco do archive
    time_t data_modif;       // Data da última modificação
    int ordem;               // Ordem de inserção
    long offset;             // Posição dos dados no archive
};

// Estrutura do diretório
struct Diretorio {
    struct Membro *membros;  // Vetor de membros
    int quantidade;          // Número atual de membros
    int capacidade;          // Tamanho alocado
};

// Cria diretório vazio com capacidade inicial
// RETORNO: ponteiro para o diretório alocado ou NULL em caso de erro
struct Diretorio *cria_diretorio();

/* Lê o diretório do arquivo
 * Retorna um ponteiro para o diretório lido, ou NULL em caso de erro
 */
struct Diretorio *le_diretorio(FILE *archive);

// Libera a memória alocada
void destroi_diretorio(struct Diretorio *dir);

// Adiciona membro no diretório
// RETORNO: índice do novo membro ou -1 em caso de erro
int adiciona_membro(struct Diretorio *dir, struct Membro membro);

// Remove membro por índice
// RETORNO: 0 em caso de sucesso ou -1 em caso de erro
int remove_membro(struct Diretorio *dir, int indice);

// Lê o diretório do arquivo archive
// RETORNO: ponteiro para o diretório preenchido ou NULL em caso de erro
struct Diretorio *le_diretorio(FILE *archive);

// Salva diretório
int salva_diretorio(FILE *archive, struct Diretorio *dir);

// Calcula offset (posição em bytes) onde os dados no novo membro devem ser escritos
// RETORNO: offset (posição) onde novos dados serão escritos ou -1 em caso de erro
long offset_final(FILE *archive);

// Busca um membro pelo nome
// RETORNO: índice do membro ou -1 se não encontrado
int busca_membro(const char *nome, struct Membro *diretorio, int num_membros);

#endif
 