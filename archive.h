#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdio.h>

// Insere/acrescenta membros (-ip/ -ic)
// RETORNO: 0 em caso de sucesso, 1 em caso de erro
int inserir_membro(const char *archive, const char *membro, int comprimir);

// Função para ler um arquivo para a memória
unsigned char *le_arquivo(const char *nome_arq, unsigned int *tam);

// Remove membros (opção -r)
// RETORNO: 0 em caso de sucesso, 1 em caso de erro
int remover_membros(const char *archive, const char **membros, int num_membros);

// Função auxiliar para verificar se um membro deve ser extraído
// RETORNO: 1 se deve extrair, 0 caso contrário
int deve_extrair(const char *nome, const char **membros, int num_membros);

// Extrai membros (opção -x)
// RETORNO: 0 em caso de sucesso, 1 em caso de erro
int extrair_membros(const char *archive, const char **membros, int num_membros);

// Lista o conteúdo (opção -c)
// RETORNO: 0 em caso de sucesso, 1 em caso de erro
int listar_conteudo(const char *archive);

// Move membro (opção -m)
// RETORNO: 0 em caso de sucesso, 1 em caso de erro
int mover_membro(const char *archive, const char *membro, const char *alvo);

// Lê um arquivo e retorna os dados em um buffer
// RETORNO: Ponteiro para o buffer ou NULL em caso de erro
unsigned char *le_arquivo(const char *nome_arq, unsigned int *tam);

// Escreve um buffer em um arquivo
// RETORNO: 0 em caso de sucesso, 1 em caso de erro
int escreve_arquivo(const char *nome_arq, unsigned char *buffer, unsigned int tam);

#endif