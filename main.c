#include "archive.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Verifica se há argumentos suficientes
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <opção> <archive> [membros...]\n", argv[0]);
        return 1;
    }

    // Obtém a opção
    const char *opcao = argv[1];
    const char *archive = argv[2];

    // Verifica a opção
    if (strcmp(opcao, "-i") == 0 || strcmp(opcao, "-ip") == 0) {
        // Inserir membros
        int comprimir = (strcmp(opcao, "-ip") == 0);
        
        // Verifica se há membros para inserir
        if (argc < 4) {
            fprintf(stderr, "Erro: Nenhum membro especificado para inserção\n");
            return 1;
        }
        
        // Insere cada membro
        for (int i = 3; i < argc; i++) {
            if (inserir_membro(archive, argv[i], comprimir) != 0) {
                fprintf(stderr, "Erro ao inserir membro: %s\n", argv[i]);
                return 1;
            }
        }
    } else if (strcmp(opcao, "-x") == 0) {
        // Extrair membros
        if (argc == 3) {
            // Extrair todos os membros
            return extrair_membros(archive, NULL, 0);
        } else {
            // Extrair membros específicos
            const char **membros = (const char **)&argv[3];
            int num_membros = argc - 3;
            return extrair_membros(archive, membros, num_membros);
        }
    } else if (strcmp(opcao, "-d") == 0) {
        // Remover membros
        if (argc < 4) {
            fprintf(stderr, "Erro: Nenhum membro especificado para remoção\n");
            return 1;
        }
        
        const char **membros = (const char **)&argv[3];
        int num_membros = argc - 3;
        return remover_membros(archive, membros, num_membros);
    } else if (strcmp(opcao, "-m") == 0) {
        // Mover membro
        if (argc != 5) {
            fprintf(stderr, "Erro: Uso incorreto da opção -m\n");
            fprintf(stderr, "Uso: %s -m <archive> <membro> <alvo>\n", argv[0]);
            return 1;
        }
        
        const char *membro = argv[3];
        const char *alvo = argv[4];
        
        return mover_membro(archive, membro, alvo);
    } else if (strcmp(opcao, "-c") == 0) {
        // Listar conteúdo
        return listar_conteudo(archive);
    } else {
        fprintf(stderr, "Erro: Opção desconhecida: %s\n", opcao);
        return 1;
    }

    return 0;
}