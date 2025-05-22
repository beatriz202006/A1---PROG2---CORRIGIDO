#include "diretorio.h"
#include <stdlib.h>
#include <string.h>

#define CAPACIDADE_INICIAL 10

struct Diretorio *cria_diretorio() {
    struct Diretorio *dir = malloc(sizeof(struct Diretorio));
    if (!dir) return NULL;

    dir->membros = malloc(CAPACIDADE_INICIAL * sizeof(struct Membro));
    if (!dir->membros) {
        free(dir);
        return NULL;
    }

    dir->quantidade = 0;
    dir->capacidade = CAPACIDADE_INICIAL;
    return dir;
}

void destroi_diretorio(struct Diretorio *dir) {
    if (!dir) return;
    
    free(dir->membros);
    free(dir);
}

int adiciona_membro(struct Diretorio *dir, struct Membro membro) {
    if (!dir) return -1;

    // Verifica se precisa redimensionar
    if (dir->quantidade >= dir->capacidade) {
        int nova_capacidade = dir->capacidade * 2;
        struct Membro *novo_vetor = realloc(dir->membros, 
                                           nova_capacidade * sizeof(struct Membro));
        if (!novo_vetor) return -1;
        
        dir->membros = novo_vetor;
        dir->capacidade = nova_capacidade;
    }

    // Adiciona o novo membro
    dir->membros[dir->quantidade] = membro;
    dir->quantidade++;
    
    return (dir->quantidade - 1);
}

int remove_membro(struct Diretorio *dir, int indice) {
    if (!dir || indice < 0 || indice >= dir->quantidade) return -1;

    // Move os membros posteriores para preencher o espaço
    for (int i = indice; i < dir->quantidade - 1; i++) {
        dir->membros[i] = dir->membros[i + 1];
    }
    
    dir->quantidade--;
    return 0;
}

struct Diretorio *le_diretorio(FILE *arq) {
    // Posiciona no início do arquivo
    if (fseek(arq, 0, SEEK_SET) != 0) {
        return NULL;
    }
    
    // Lê a quantidade de membros
    int quantidade;
    if (fread(&quantidade, sizeof(int), 1, arq) != 1) {
        return NULL;
    }
    
    // Cria o diretório
    struct Diretorio *dir = malloc(sizeof(struct Diretorio));
    if (!dir) {
        return NULL;
    }
    
    dir->quantidade = quantidade;
    dir->membros = NULL;
    
    if (quantidade > 0) {
        // Aloca espaço para os membros
        dir->membros = malloc(quantidade * sizeof(struct Membro));
        if (!dir->membros) {
            free(dir);
            return NULL;
        }
        
        // Lê cada membro
        for (int i = 0; i < quantidade; i++) {
            if (fread(&dir->membros[i], sizeof(struct Membro), 1, arq) != 1) {
                free(dir->membros);
                free(dir);
                return NULL;
            }
        }
    }
    
    return dir;
}


int salva_diretorio(FILE *arq, struct Diretorio *dir) {
    // Posiciona no início do arquivo
    if (fseek(arq, 0, SEEK_SET) != 0) {
        return 1;
    }
    
    // Escreve a quantidade de membros
    if (fwrite(&dir->quantidade, sizeof(int), 1, arq) != 1) {
        return 1;
    }
    
    // Escreve cada membro
    for (int i = 0; i < dir->quantidade; i++) {
        if (fwrite(&dir->membros[i], sizeof(struct Membro), 1, arq) != 1) {
            return 1;
        }
    }
    
    return 0;
}
long offset_final(FILE *arq) {
    // Salva a posição atual
    long pos_atual = ftell(arq);
    if (pos_atual < 0) return -1;
    
    // Vai para o final do arquivo
    if (fseek(arq, 0, SEEK_END) != 0) return -1;
    
    // Obtém a posição final
    long pos_final = ftell(arq);
    
    // Restaura a posição original
    if (fseek(arq, pos_atual, SEEK_SET) != 0) return -1;
    
    return pos_final;
}


int busca_membro(const char *nome, struct Membro *diretorio, int num_membros) {
    if (!nome || !diretorio || num_membros <= 0) return -1;

    for (int i = 0; i < num_membros; i++) {
        if (strcmp(diretorio[i].nome, nome) == 0) {
            return i;
        }
    }
    
    return -1;
}
