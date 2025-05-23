#include "archive.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <opção> <arquivo> [membros...]\n", argv[0]);
        return 1;
    }

    const char *opcao = argv[1];
    const char *arquivo = argv[2];

    if (strcmp(opcao, "-ip") == 0 || strcmp(opcao, "-ic") == 0) {
        // Inserir membros com compressão
        if (argc < 4) {
            fprintf(stderr, "Erro: Faltando nome do membro para inserir\n");
            return 1;
        }
        
        // Insere cada membro especificado com compressão
        for (int i = 3; i < argc; i++) {
            int resultado = inserir_membro(arquivo, argv[i], 1); // Com compressão
            if (resultado != 0) {
                fprintf(stderr, "Erro ao inserir membro: %s\n", argv[i]);
                return resultado;
            }
        }
        return 0;
    } else if (strcmp(opcao, "-x") == 0) {
        // Extrair membros
        if (argc == 3) {
            // Extrair todos os membros
            return extrair_membros(arquivo, NULL, 0);
        } else {
            // Extrair membros específicos
            return extrair_membros(arquivo, (const char **)&argv[3], argc - 3);
        }
    } else if (strcmp(opcao, "-d") == 0) {
        // Listar diretório
        return listar_conteudo(arquivo);
    } else if (strcmp(opcao, "-r") == 0) {
        // Remover membro
        if (argc < 4) {
            fprintf(stderr, "Erro: Faltando nome do membro para remover\n");
            return 1;
        }
        return remover_membros(arquivo, (const char **)&argv[3], argc - 3);
    } else if (strcmp(opcao, "-m") == 0) {
        // Mover membro
        if (argc < 5) {
            fprintf(stderr, "Uso: %s -m <arquivo> <membro> <alvo>\n", argv[0]);
            return 1;
        }
        return mover_membro(arquivo, argv[3], argv[4]);
    } else {
        fprintf(stderr, "Erro: Opção desconhecida: %s\n", opcao);
        return 1;
    }
}
