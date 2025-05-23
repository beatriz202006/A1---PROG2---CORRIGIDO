#include "diretorio.h"
#include <stdlib.h>
#include <string.h>

#define CAPACIDADE_INICIAL 10

struct Diretorio *cria_diretorio() {

    //Aloca memória para o diretório:
    struct Diretorio *dir = malloc(sizeof(struct Diretorio));
    if (!dir) return NULL;

    dir->membros = malloc(CAPACIDADE_INICIAL * sizeof(struct Membro));

    //Verifica caso de erro:
    if (!dir->membros) {
        free(dir);
        return NULL;
    }

    ///Inicializa os campos:
    dir->quantidade = 0;
    dir->capacidade = CAPACIDADE_INICIAL;
    return dir;
}

struct Membro inicializa_membro(const char *nome, uid_t uid, unsigned int tam_orig,
                         unsigned int tam_disco, time_t data_modif, int ordem,
                         long offset, int comprimido) {
    struct Membro membro;

    //Zera a estrutura para evitar lixo na memória:
    memset(&membro, 0, sizeof(struct Membro));

    //Inicializa os campos da Struct Membro:
    strncpy(membro.nome, nome, sizeof(membro.nome) - 1);
    membro.nome[sizeof(membro.nome) - 1] = '\0';  // Garante terminação
    membro.uid = uid;
    membro.tam_orig = tam_orig;
    membro.tam_disco = tam_disco;
    membro.data_modif = data_modif;
    membro.ordem = ordem;
    membro.offset = offset;
    membro.comprimido = comprimido;
    
    return membro;
}

void destroi_diretorio(struct Diretorio *dir) {
    if (!dir) return;
    
    //Libera a memória alocada:
    free(dir->membros);
    free(dir);
}

int adiciona_membro(struct Diretorio *dir, struct Membro membro) {
    if (!dir) return -1;

    // Verifica se precisa redimensionar
    if (dir->quantidade >= dir->capacidade) {

        //Dobra a capacidade do vetor de membros:
        int nova_capacidade = dir->capacidade * 2;
        struct Membro *novo_vetor = realloc(dir->membros, nova_capacidade * sizeof(struct Membro));

        //Verifica caso de erro:
        if (!novo_vetor) 
            return -1;
        
        //Atualiza o vetor com a nova capacidade:
        dir->membros = novo_vetor;
        dir->capacidade = nova_capacidade;
    }

    // Adiciona o novo membro
    dir->membros[dir->quantidade] = membro;
    dir->quantidade++;
    
    return (dir->quantidade - 1);
}

int remove_membro(struct Diretorio *dir, int indice) {

    //Verifica casos de erro;
    if (!dir || indice < 0 || indice >= dir->quantidade) 
        return -1;

    // Move os membros posteriores para preencher o espaço
    for (int i = indice; i < dir->quantidade - 1; i++) {
        dir->membros[i] = dir->membros[i + 1];
    }
    
    dir->quantidade--;
    return 0;
}

struct Diretorio *le_diretorio(FILE *arq) {

    // Posiciona no início do arquivo
    rewind(arq);
    
    // Lê a quantidade de membros
    int quantidade;
    if (fread(&quantidade, sizeof(int), 1, arq) != 1) {
        fprintf(stderr, "Erro ao ler quantidade de membros\n");
        return NULL;
    }
    
    
    // Verifica se a quantidade é válida (limite arbitrário) de 100 membros)
    if (quantidade < 0 || quantidade > 1000) {  
        fprintf(stderr, "Quantidade inválida de membros: %d\n", quantidade);
        return NULL;
    }
    
    // Aloca o diretório
    struct Diretorio *dir = malloc(sizeof(struct Diretorio));
    if (!dir) {
        fprintf(stderr, "Erro ao alocar diretório\n");
        return NULL;
    }
    
    dir->quantidade = quantidade;
    
    // Se não há membros, retorna o diretório vazio
    if (quantidade == 0) {
        dir->membros = NULL;
        return dir;
    }
    
    // Aloca espaço para os membros
    dir->membros = malloc(quantidade * sizeof(struct Membro));
    if (!dir->membros) {
        fprintf(stderr, "Erro ao alocar membros\n");
        free(dir);
        return NULL;
    }
    
    // Lê os membros
    if (fread(dir->membros, sizeof(struct Membro), quantidade, arq) != quantidade) {
        fprintf(stderr, "Erro ao ler membros\n");
        free(dir->membros);
        free(dir);
        return NULL;
    }
    
    return dir;
}

int salva_diretorio(FILE *arq, struct Diretorio *dir) {
    
    // Posiciona no início do arquivo
    if (fseek(arq, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Erro ao posicionar no início do arquivo\n");
        return 1;
    }
    
    // Escreve a quantidade de membros
    if (fwrite(&dir->quantidade, sizeof(int), 1, arq) != 1) {
        fprintf(stderr, "Erro ao escrever quantidade de membros\n");
        return 1;
    }
    
    // Escreve cada membro
    for (int i = 0; i < dir->quantidade; i++) {        
        if (fwrite(&dir->membros[i], sizeof(struct Membro), 1, arq) != 1) {
            fprintf(stderr, "Erro ao escrever membro %d\n", i);
            return 1;
        }
    }
    
    return 0;
}

long offset_final(FILE *arq) {
    // Salva a posição atual
    long pos_atual = ftell(arq);

    //Verifica se a posição é válida:
    if (pos_atual < 0) 
        return -1;
    
    // Vai para o final do arquivo
    if (fseek(arq, 0, SEEK_END) != 0) 
        return -1;
    
    // Obtém a posição final
    long pos_final = ftell(arq);
    
    // Restaura a posição original
    if (fseek(arq, pos_atual, SEEK_SET) != 0) return -1;
    
    return pos_final;
}


int busca_membro(const char *nome, struct Membro *diretorio, int num_membros) {

    //Verifica casos de erro:
    if (!nome || !diretorio || num_membros <= 0) 
        return -1;

    // Procura o membro pelo nome, percorrendo o vetor de membros:
    for (int i = 0; i < num_membros; i++) {
        if (strcmp(diretorio[i].nome, nome) == 0) {
            return i;
        }
    }
    
    return -1;
}