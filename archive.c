#include "archive.h"
#include "diretorio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h> // Para basename


// Função para extrair o nome base do arquivo
const char *get_basename(const char *path) {
    const char *base = strrchr(path, '/');
    if (base) {
        return base + 1;  // Pula o caractere '/'
    } else {
        return path;  // Não há '/' no caminho
    }
}


unsigned char *le_arquivo(const char *nome_arq, unsigned int *tam) {

    // Abre o arquivo
    FILE *arquivo = fopen(nome_arq, "rb");

    //Verifica caso de erro:
    if (!arquivo) {
        fprintf(stderr, "Erro ao abrir arquivo: %s\n", nome_arq);
        return NULL;
    }

    // Determina o tamanho do arquivo
    if (fseek(arquivo, 0, SEEK_END) != 0) {
        fprintf(stderr, "Erro ao posicionar no final do arquivo: %s\n", nome_arq);
        fclose(arquivo);
        return NULL;
    }
    
    long tamanho = ftell(arquivo);

    //Verifica caso de erro:
    if (tamanho < 0) {
        fprintf(stderr, "Erro ao determinar tamanho do arquivo: %s\n", nome_arq);
        fclose(arquivo);
        return NULL;
    }
    
    *tam = (unsigned int)tamanho;
    
    // Volta para o início do arquivo
    if (fseek(arquivo, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Erro ao posicionar no início do arquivo: %s\n", nome_arq);
        fclose(arquivo);
        return NULL;
    }

    // Aloca buffer do tamaho do arquivo
    unsigned char *buffer = malloc(*tam);
    if (!buffer) {
        fprintf(stderr, "Erro ao alocar memória para arquivo: %s\n", nome_arq);
        fclose(arquivo);
        return NULL;
    }

    // Lê o arquivo para o buffer
    size_t bytes_lidos = fread(buffer, 1, *tam, arquivo);
    if (bytes_lidos != *tam) {
        fprintf(stderr, "Erro ao ler conteúdo do arquivo: %s (lido %zu de %u bytes)\n", 
                nome_arq, bytes_lidos, *tam);
        free(buffer);
        fclose(arquivo);
        return NULL;
    }

    fclose(arquivo);
    return buffer;
}


int escreve_arquivo(const char *nome_arq, unsigned char *buffer, unsigned int tam) {

    FILE *arquivo = fopen(nome_arq, "wb");

    //Verifica caso de erro:
    if (!arquivo) {
        fprintf(stderr, "Erro ao criar arquivo: %s\n", nome_arq);
        return 1;
    }

    // Escreve o buffer no arquivo
    if (fwrite(buffer, 1, tam, arquivo) != tam) {
        fprintf(stderr, "Erro ao escrever no arquivo: %s\n", nome_arq);
        fclose(arquivo);
        return 1;
    }

    fclose(arquivo);
    return 0;
}


int inserir_membro(const char *archive, const char *membro, int comprimir) {

    // Lê o arquivo a ser inserido
    unsigned int tam_original;
    unsigned char *dados = le_arquivo(membro, &tam_original);
    if (!dados) 
        return 1;

    // Comprime os dados se necessário
    unsigned char *dados_comprimidos = NULL;
    unsigned int tam_comprimido = 0;
    
    if (comprimir) {
        // Aloca espaço para os dados comprimidos (pior caso: mesmo tamanho do original)
        dados_comprimidos = malloc(tam_original);
        if (!dados_comprimidos) {
            free(dados);
            return 1;
        }
        
        // Comprime os dados chamando a biblioteca LZ
        tam_comprimido = LZ_Compress(dados, dados_comprimidos, tam_original);
        
        // Se a compressão não reduziu o tamanho, usa os dados originais
        if (tam_comprimido >= tam_original) {
            free(dados_comprimidos);
            dados_comprimidos = NULL;
            comprimir = 0;  // Desativa a compressão
        }
    }

    // Extrai apenas o nome base do arquivo (sem caminho)
    const char *nome_base = get_basename(membro);

    // Estrutura para armazenar o diretório
    struct Diretorio dir;
    dir.quantidade = 0;
    dir.membros = NULL;

    // Verifica se o arquivo já existe
    FILE *arq = fopen(archive, "rb");
    if (arq) {
        // Lê a quantidade de membros
        if (fread(&dir.quantidade, sizeof(int), 1, arq) != 1) {
            fclose(arq);
            free(dados);
            if (dados_comprimidos) free(dados_comprimidos);
            return 1;
        }

        // Aloca espaço para os membros
        if (dir.quantidade > 0) {
            dir.membros = malloc(dir.quantidade * sizeof(struct Membro));
            if (!dir.membros) {
                fclose(arq);
                free(dados);
                if (dados_comprimidos) free(dados_comprimidos);
                return 1;
            }

            // Lê os membros
            if (fread(dir.membros, sizeof(struct Membro), dir.quantidade, arq) != dir.quantidade) {
                free(dir.membros);
                fclose(arq);
                free(dados);
                if (dados_comprimidos) free(dados_comprimidos);
                return 1;
            }
        }
        fclose(arq);
    }

    // Verifica se já existe um membro com o mesmo nome, percorrendo o vetor de membros
    int membro_existente = -1;
    for (int i = 0; i < dir.quantidade; i++) {
        if (strcmp(dir.membros[i].nome, nome_base) == 0) {
            membro_existente = i;
            break;
        }
    }

    // Cria um novo vetor de membros
    int nova_quantidade;
    struct Membro *novos_membros;

    if (membro_existente == -1) {
        // Adiciona um novo membro
        nova_quantidade = dir.quantidade + 1;
        novos_membros = malloc(nova_quantidade * sizeof(struct Membro));
        if (!novos_membros) {
            if (dir.membros) free(dir.membros);
            free(dados);
            if (dados_comprimidos) free(dados_comprimidos);
            return 1;
        }

        // Copia os membros existentes
        for (int i = 0; i < dir.quantidade; i++) {
            novos_membros[i] = dir.membros[i];
        }

        // Adiciona o novo membro
        novos_membros[dir.quantidade] = inicializa_membro(nome_base, getuid(),tam_original,
        comprimir ? tam_comprimido : tam_original, time(NULL), dir.quantidade, 0, comprimir ? 1 : 0);

    // Se o membro já existe, atualiza o membro existente
    } else {
        // Substitui o membro existente
        nova_quantidade = dir.quantidade;
        novos_membros = malloc(nova_quantidade * sizeof(struct Membro));

        //Veifica caso  de erro:
        if (!novos_membros) {
            if (dir.membros) free(dir.membros);
            free(dados);
            if (dados_comprimidos) free(dados_comprimidos);
            return 1;
        }

        // Copia os membros existentes
        for (int i = 0; i < dir.quantidade; i++) {
            if (i == membro_existente) {
                // Atualiza o membro existente
                novos_membros[i] = inicializa_membro(nome_base, getuid(), tam_original,
                comprimir ? tam_comprimido : tam_original, time(NULL), i, 0, comprimir ? 1 : 0);
            } else {
                // Copia o membro existente
                novos_membros[i] = dir.membros[i];
            }
        }
    }

    // Calcula o tamanho do diretório
    long tamanho_diretorio = sizeof(int) + nova_quantidade * sizeof(struct Membro);

    // Calcula os offsets para cada membro
    long offset = tamanho_diretorio;
    for (int i = 0; i < nova_quantidade; i++) {
        novos_membros[i].offset = offset;
        offset += novos_membros[i].tam_disco;
    }

    // Cria um arquivo temporário para armazenar os dados
    char temp_file[1024];
    snprintf(temp_file, 1024, "%s.tmp", archive);

    // Abre o arquivo temporário para escrita
    FILE *temp = fopen(temp_file, "wb");
    if (!temp) {
        free(novos_membros);
        if (dir.membros) free(dir.membros);
        free(dados);
        if (dados_comprimidos) free(dados_comprimidos);
        return 1;
    }

    // Escreve a quantidade de membros
    if (fwrite(&nova_quantidade, sizeof(int), 1, temp) != 1) {
        fclose(temp);
        remove(temp_file);
        free(novos_membros);
        if (dir.membros) free(dir.membros);
        free(dados);
        if (dados_comprimidos) free(dados_comprimidos);
        return 1;
    }

    // Escreve os membros
    if (fwrite(novos_membros, sizeof(struct Membro), nova_quantidade, temp) != nova_quantidade) {
        fclose(temp);
        remove(temp_file);
        free(novos_membros);
        if (dir.membros) free(dir.membros);
        free(dados);
        if (dados_comprimidos) free(dados_comprimidos);
        return 1;
    }

    // Abre o arquivo original para leitura
    arq = fopen(archive, "rb");
    if (arq) {
        // Pula o cabeçalho do arquivo original
        int old_quantidade;
        if (fread(&old_quantidade, sizeof(int), 1, arq) != 1) {
            fclose(arq);
            fclose(temp);
            remove(temp_file);
            free(novos_membros);
            if (dir.membros) free(dir.membros);
            free(dados);
            if (dados_comprimidos) free(dados_comprimidos);
            return 1;
        }

        // Pula as estruturas de membros
        if (fseek(arq, old_quantidade * sizeof(struct Membro), SEEK_CUR) != 0) {
            fclose(arq);
            fclose(temp);
            remove(temp_file);
            free(novos_membros);
            if (dir.membros) free(dir.membros);
            free(dados);
            if (dados_comprimidos) free(dados_comprimidos);
            return 1;
        }

        // Para cada membro no novo diretório
        for (int i = 0; i < nova_quantidade; i++) {
            // Se este é o membro que está sendo substituído ou adicionado
            if ((membro_existente != -1 && i == membro_existente) || (membro_existente == -1 && i == nova_quantidade - 1)) {
                
                // Escreve os dados (comprimidos ou originais)
                if (comprimir) {
                    if (fwrite(dados_comprimidos, 1, tam_comprimido, temp) != tam_comprimido) {
                        fclose(arq);
                        fclose(temp);
                        remove(temp_file);
                        free(novos_membros);
                        if (dir.membros) free(dir.membros);
                        free(dados);
                        free(dados_comprimidos);
                        return 1;
                    }
                } else {
                    if (fwrite(dados, 1, tam_original, temp) != tam_original) {
                        fclose(arq);
                        fclose(temp);
                        remove(temp_file);
                        free(novos_membros);
                        if (dir.membros) free(dir.membros);
                        free(dados);
                        if (dados_comprimidos) free(dados_comprimidos);
                        return 1;
                    }
                }
            } else {
                // Encontra o índice correspondente no arquivo original
                int indice_original = -1;
                for (int j = 0; j < old_quantidade; j++) {
                    if (strcmp(dir.membros[j].nome, novos_membros[i].nome) == 0) {
                        indice_original = j;
                        break;
                    }
                }

                if (indice_original == -1) {
                    fclose(arq);
                    fclose(temp);
                    remove(temp_file);
                    free(novos_membros);
                    if (dir.membros) free(dir.membros);
                    free(dados);
                    if (dados_comprimidos) free(dados_comprimidos);
                    return 1;
                }

                // Posiciona o ponteiro no início dos dados do membro no arquivo original
                if (fseek(arq, dir.membros[indice_original].offset, SEEK_SET) != 0) {
                    fclose(arq);
                    fclose(temp);
                    remove(temp_file);
                    free(novos_membros);
                    if (dir.membros) free(dir.membros);
                    free(dados);
                    if (dados_comprimidos) free(dados_comprimidos);
                    return 1;
                }

                // Aloca espaço para os dados do membro
                unsigned char *dados_membro = malloc(dir.membros[indice_original].tam_disco);
                if (!dados_membro) {
                    fclose(arq);
                    fclose(temp);
                    remove(temp_file);
                    free(novos_membros);
                    if (dir.membros) free(dir.membros);
                    free(dados);
                    if (dados_comprimidos) free(dados_comprimidos);
                    return 1;
                }

                // Lê os dados do membro
                if (fread(dados_membro, 1, dir.membros[indice_original].tam_disco, arq) != dir.membros[indice_original].tam_disco) {
                    free(dados_membro);
                    fclose(arq);
                    fclose(temp);
                    remove(temp_file);
                    free(novos_membros);
                    if (dir.membros) free(dir.membros);
                    free(dados);
                    if (dados_comprimidos) free(dados_comprimidos);
                    return 1;
                }

                // Escreve os dados do membro no arquivo temporário
                if (fwrite(dados_membro, 1, dir.membros[indice_original].tam_disco, temp) != dir.membros[indice_original].tam_disco) {
                    free(dados_membro);
                    fclose(arq);
                    fclose(temp);
                    remove(temp_file);
                    free(novos_membros);
                    if (dir.membros) free(dir.membros);
                    free(dados);
                    if (dados_comprimidos) free(dados_comprimidos);
                    return 1;
                }

                free(dados_membro);
            }
        }

        fclose(arq);
    } else {
        // Se o arquivo original não existe, apenas escreve os dados do novo membro
        if (comprimir) {
            if (fwrite(dados_comprimidos, 1, tam_comprimido, temp) != tam_comprimido) {
                fclose(temp);
                remove(temp_file);
                free(novos_membros);
                if (dir.membros) free(dir.membros);
                free(dados);
                free(dados_comprimidos);
                return 1;
            }
        } else {
            if (fwrite(dados, 1, tam_original, temp) != tam_original) {
                fclose(temp);
                remove(temp_file);
                free(novos_membros);
                if (dir.membros) free(dir.membros);
                free(dados);
                if (dados_comprimidos) free(dados_comprimidos);
                return 1;
            }
        }
    }

    // Fecha o arquivo temporário
    fclose(temp);

    // Substitui o arquivo original pelo temporário
    if (rename(temp_file, archive) != 0) {
        remove(temp_file);
        free(novos_membros);
        if (dir.membros) free(dir.membros);
        free(dados);
        if (dados_comprimidos) free(dados_comprimidos);
        return 1;
    }

    // Limpeza
    free(novos_membros);
    if (dir.membros) free(dir.membros);
    free(dados);
    if (dados_comprimidos) free(dados_comprimidos);
    
    return 0;
}


int deve_extrair(const char *nome, const char **membros, int num_membros) {

    // Se não há lista específica, extrai todos os membros
    if (!membros || num_membros <= 0) 
        return 1;
    
    for (int i = 0; i < num_membros; i++) {
        if (membros[i] && strcmp(nome, membros[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

int extrair_membros(const char *archive, const char **membros, int num_membros) {
    
    // Abre o arquivo archive
    FILE *arq = fopen(archive, "rb");
    if (!arq) {
        fprintf(stderr, "Erro ao abrir archive: %s\n", archive);
        return 1;
    }

    // Lê o diretório
    struct Diretorio *dir = le_diretorio(arq);
    if (!dir) {
        fprintf(stderr, "Erro ao ler diretório\n");
        fclose(arq);
        return 1;
    }

    // Extrai cada membro
    for (int i = 0; i < dir->quantidade; i++) {
        struct Membro *m = &dir->membros[i];
        
        // Verifica se deve extrair este membro
        int extrair = 0;
        
        // Se não há lista específica, extrai todos os membros
        if (num_membros == 0) {
            extrair = 1;
        } else {
            for (int j = 0; j < num_membros; j++) {
                if (membros[j] && strcmp(m->nome, membros[j]) == 0) {
                    extrair = 1;
                    break;
                }
            }
        }
        
        if (extrair) {
            
            // Posiciona no início dos dados do membro
            if (fseek(arq, m->offset, SEEK_SET) != 0) {
                fprintf(stderr, "Erro ao posicionar no offset %ld\n", m->offset);
                destroi_diretorio(dir);
                fclose(arq);
                return 1;
            }
            
            // Aloca buffer para os dados comprimidos
            unsigned char *buffer = malloc(m->tam_disco);
            if (!buffer) {
                fprintf(stderr, "Erro ao alocar memória para extração\n");
                destroi_diretorio(dir);
                fclose(arq);
                return 1;
            }
            
            // Lê os dados do membro
            size_t bytes_lidos = fread(buffer, 1, m->tam_disco, arq);
            if (bytes_lidos != m->tam_disco) {
                fprintf(stderr, "Erro ao ler dados do membro %s: lido %zu de %u bytes\n", 
                        m->nome, bytes_lidos, m->tam_disco);
                free(buffer);
                destroi_diretorio(dir);
                fclose(arq);
                return 1;
            }
            
            // Se o membro está comprimido, descomprime
            unsigned char *dados_finais = buffer;
            unsigned int tam_final = m->tam_disco;
            
            if (m->comprimido) {
                
                unsigned char *dados_descomprimidos = malloc(m->tam_orig);
                if (!dados_descomprimidos) {
                    fprintf(stderr, "Erro ao alocar memória para descompressão\n");
                    free(buffer);
                    destroi_diretorio(dir);
                    fclose(arq);
                    return 1;
                }
                
                // Descomprime os dados
                LZ_Uncompress(buffer, dados_descomprimidos, m->tam_disco);
                
                // Libera os dados comprimidos, pois não são mais necessários
                free(buffer);
                
                // Usa os dados descomprimidos
                dados_finais = dados_descomprimidos;
                tam_final = m->tam_orig;
            }
            
            // Escreve os dados no arquivo de saída
            FILE *saida = fopen(m->nome, "wb");
            if (!saida) {
                fprintf(stderr, "Erro ao criar arquivo de saída: %s\n", m->nome);
                free(dados_finais);
                destroi_diretorio(dir);
                fclose(arq);
                return 1;
            }
            
            size_t bytes_escritos = fwrite(dados_finais, 1, tam_final, saida);
            if (bytes_escritos != tam_final) {
                fprintf(stderr, "Erro ao escrever dados no arquivo %s: escrito %zu de %u bytes\n", 
                        m->nome, bytes_escritos, tam_final);
                fclose(saida);
                free(dados_finais);
                destroi_diretorio(dir);
                fclose(arq);
                return 1;
            }
            
            fclose(saida);
            
            // Verifica o arquivo extraído
            FILE *verificacao = fopen(m->nome, "rb");
            if (verificacao) {
                fseek(verificacao, 0, SEEK_END);
                long tamanho_arquivo = ftell(verificacao);
                rewind(verificacao);
                unsigned char primeiros_bytes[10];
                size_t bytes_lidos_verificacao = fread(primeiros_bytes, 1, sizeof(primeiros_bytes), verificacao);
                
                for (size_t j = 0; j < bytes_lidos_verificacao; j++) {
                    fprintf(stderr, "%02x ", primeiros_bytes[j]);
                }
                fprintf(stderr, "\n");
                
                fclose(verificacao);
            }
            
            free(dados_finais);
        }
    }

    // Limpeza
    destroi_diretorio(dir);
    fclose(arq);
    return 0;
}

int remover_membros(const char *archive, const char **membros, int num_membros) {
    if (!archive || !membros || num_membros <= 0) return 1;

    // Abre o arquivo archive
    FILE *arq = fopen(archive, "rb+");
    if (!arq) return 1;

    // Lê o diretório
    struct Diretorio *dir = le_diretorio(arq);
    if (!dir) {
        fclose(arq);
        return 1;
    }

    int removidos = 0;
    
    // Remove cada membro solicitado
    for (int i = 0; i < num_membros; i++) {
        int idx = busca_membro(membros[i], dir->membros, dir->quantidade);
        if (idx != -1) {
            if (remove_membro(dir, idx) == 0) {
                removidos++;
            }
        }
    }

    // Se removeu algum membro, salva o diretório atualizado
    if (removidos > 0) {
        salva_diretorio(arq, dir);
    }

    // Limpeza
    destroi_diretorio(dir);
    fclose(arq);
    return 0;
}

int listar_conteudo(const char *archive) {
    // Abre o arquivo archive
    FILE *arq = fopen(archive, "rb");
    if (!arq) return 1;

    // Lê o diretório
    struct Diretorio *dir = le_diretorio(arq);
    if (!dir) {
        fclose(arq);
        return 1;
    }

    // Imprime cabeçalho
    printf("Conteúdo do archive '%s':\n", archive);
    printf("Ordem | Nome | Tamanho Original | Tamanho Disco | UID | Data (timestamp)\n");
    printf("--------------------------------------------------------------------------------\n");

    // Lista cada membro
    for (int i = 0; i < dir->quantidade; i++) {
        struct Membro *m = &dir->membros[i];
        printf("%5d | %-12s | %15u | %12u | %4d | %ld\n",
               i, m->nome, m->tam_orig, m->tam_disco, m->uid, m->data_modif);
    }

    // Limpeza
    destroi_diretorio(dir);
    fclose(arq);
    return 0;
}

int mover_membro(const char *archive, const char *membro, const char *alvo) {
    // Abre o arquivo archive
    FILE *arq = fopen(archive, "rb+");
    if (!arq) return 1;

    // Lê o diretório
    struct Diretorio *dir = le_diretorio(arq);
    if (!dir) {
        fclose(arq);
        return 1;
    }

    // Busca posições do membro e do alvo
    int pos_membro = busca_membro(membro, dir->membros, dir->quantidade);
    int pos_alvo = busca_membro(alvo, dir->membros, dir->quantidade);

    // Verifica se ambos existem
    if (pos_membro == -1 || pos_alvo == -1) {
        destroi_diretorio(dir);
        fclose(arq);
        return 1;
    }

    // Guarda o membro a ser movido
    struct Membro membro_movido = dir->membros[pos_membro];

    // Remove o membro da posição atual
    for (int i = pos_membro; i < dir->quantidade - 1; i++) {
        dir->membros[i] = dir->membros[i + 1];
    }
    dir->quantidade--;

    // Ajusta a posição do alvo se necessário
    if (pos_membro < pos_alvo) {
        pos_alvo--;
    }

    // Insere o membro após o alvo
    int nova_pos = pos_alvo + 1;
    
    // Abre espaço para o membro
    for (int i = dir->quantidade; i > nova_pos; i--) {
        dir->membros[i] = dir->membros[i - 1];
    }
    
    // Insere o membro na nova posição
    dir->membros[nova_pos] = membro_movido;
    dir->quantidade++;

    // Atualiza a ordem de todos os membros
    for (int i = 0; i < dir->quantidade; i++) {
        dir->membros[i].ordem = i;
    }

    // Salva o diretório atualizado
    salva_diretorio(arq, dir);

    // Limpeza
    destroi_diretorio(dir);
    fclose(arq);
    return 0;
}