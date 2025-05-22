#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "archive.h"

void print_usage() {
    printf("Uso: vinac [opção] [archive] [arquivos...]\n");
    printf("Opções:\n");
    printf("  -ip: Insere arquivos sem compressão\n");
    printf("  -ic: Insere arquivos com compressão\n");
    printf("  -r: Remove arquivos\n");
    printf("  -x: Extrai arquivos\n");
    printf("  -c: Lista conteúdo\n");
    printf("  -m: Move arquivo após outro\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    const char *opcao = argv[1];
    const char *archive = argv[2];

    if (strcmp(opcao, "-ip") == 0) {
        // Inserir sem compressão
        if (argc < 4) {
            printf("Erro: Faltam argumentos para -ip\n");
            return 1;
        }
        return inserir_membro(archive, argv[3], 0);
    } 
    else if (strcmp(opcao, "-ic") == 0) {
        // Inserir com compressão
        if (argc < 4) {
            printf("Erro: Faltam argumentos para -ic\n");
            return 1;
        }
        return inserir_membro(archive, argv[3], 1);
    } 
    else if (strcmp(opcao, "-r") == 0) {
        // Remover arquivos
        if (argc < 4) {
            printf("Erro: Faltam argumentos para -r\n");
            return 1;
        }
        const char **membros = (const char **)&argv[3];
        return remover_membros(archive, membros, argc - 3);
    } 
    else if (strcmp(opcao, "-x") == 0) {
        // Extrair arquivos
        if (argc == 3) {
            // Extrair todos
            return extrair_membros(archive, NULL, 0);
        } else {
            // Extrair específicos
            const char **membros = (const char **)&argv[3];
            return extrair_membros(archive, membros, argc - 3);
        }
    } 
    else if (strcmp(opcao, "-c") == 0) {
        // Listar conteúdo
        return listar_conteudo(archive);
    } 
    else if (strcmp(opcao, "-m") == 0) {
        // Mover arquivo
        if (argc < 5) {
            printf("Erro: Faltam argumentos para -m\n");
            return 1;
        }
        return mover_membro(archive, argv[3], argv[4]);
    } 
    else {
        printf("Opção desconhecida: %s\n", opcao);
        print_usage();
        return 1;
    }

    return 0;
}