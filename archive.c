#include "archive.h"
#include "diretorio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h> // Para basename
#include <limits.h>

// Função para extrair o nome base do arquivo
const char *get_basename(const char *path) {
    const char *base = strrchr(path, '/');
    if (base) {
        return base + 1;  // Pula o caractere '/'
    } else {
        return path;  // Não há '/' no caminho
    }
}

 // Adicione esta função no início do arquivo, antes de inserir_membro()

// Função para ler um arquivo para a memória
unsigned char *le_arquivo(const char *nome_arq, unsigned int *tam) {
    FILE *arquivo = fopen(nome_arq, "rb");
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

    // Aloca buffer
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

    // Imprime os primeiros bytes para debug
    fprintf(stderr, "Primeiros bytes do arquivo %s: ", nome_arq);
    for (int i = 0; i < 10 && i < (int)*tam; i++) {
        fprintf(stderr, "%02x ", buffer[i]);
    }
    fprintf(stderr, "\n");

    fclose(arquivo);
    return buffer;
}


// Adicione também esta função para escrever arquivos
int escreve_arquivo(const char *nome_arq, unsigned char *buffer, unsigned int tam) {
    FILE *arquivo = fopen(nome_arq, "wb");
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
    if (!dados) {
        return 1;
    }
    
    fprintf(stderr, "Primeiros bytes do arquivo %s: ", membro);
    for (int i = 0; i < 10 && i < tam_original; i++) {
        fprintf(stderr, "%02x ", dados[i]);
    }
    fprintf(stderr, "\n");
    
    // Extrai apenas o nome base do arquivo (sem caminho)
    const char *nome_base = get_basename(membro);
    
    // Estrutura para armazenar o diretório
    struct Diretorio *dir = NULL;
    
    // Verifica se o arquivo archive já existe
    FILE *arq = fopen(archive, "rb");
    if (arq) {
        // Lê a quantidade de membros
        int quantidade;
        if (fread(&quantidade, sizeof(int), 1, arq) != 1) {
            fclose(arq);
            free(dados);
            return 1;
        }
        
        // Aloca o diretório
        dir = malloc(sizeof(struct Diretorio));
        if (!dir) {
            fclose(arq);
            free(dados);
            return 1;
        }
        
        dir->quantidade = quantidade;
        
        // Aloca espaço para os membros
        if (quantidade > 0) {
            dir->membros = malloc(quantidade * sizeof(struct Membro));
            if (!dir->membros) {
                free(dir);
                fclose(arq);
                free(dados);
                return 1;
            }
            
            // Lê os membros
            if (fread(dir->membros, sizeof(struct Membro), quantidade, arq) != quantidade) {
                free(dir->membros);
                free(dir);
                fclose(arq);
                free(dados);
                return 1;
            }
        } else {
            dir->membros = NULL;
        }
        
        fclose(arq);
    } else {
        // Cria um novo diretório vazio
        dir = malloc(sizeof(struct Diretorio));
        if (!dir) {
            free(dados);
            return 1;
        }
        dir->quantidade = 0;
        dir->membros = NULL;
    }
    
    // Cria um novo array de membros com espaço para o novo membro
    int nova_quantidade = dir->quantidade + 1;
    struct Membro *novos_membros = malloc(nova_quantidade * sizeof(struct Membro));
    if (!novos_membros) {
        if (dir->membros) free(dir->membros);
        free(dir);
        free(dados);
        return 1;
    }
    
    // Copia os membros existentes
    for (int i = 0; i < dir->quantidade; i++) {
        novos_membros[i] = dir->membros[i];
    }
    
    // Cria o novo membro
    struct Membro novo_membro;
    memset(&novo_membro, 0, sizeof(struct Membro));
    strncpy(novo_membro.nome, nome_base, sizeof(novo_membro.nome) - 1);
    novo_membro.uid = getuid();
    novo_membro.tam_orig = tam_original;
    novo_membro.tam_disco = tam_original; // Sem compressão por enquanto
    novo_membro.data_modif = time(NULL);
    novo_membro.ordem = dir->quantidade;
    
    // Adiciona o novo membro
    novos_membros[dir->quantidade] = novo_membro;
    
    // Atualiza o diretório
    if (dir->membros) free(dir->membros);
    dir->membros = novos_membros;
    dir->quantidade = nova_quantidade;
    
    // Calcula o tamanho do diretório
    long tamanho_diretorio = sizeof(int) + nova_quantidade * sizeof(struct Membro);
    
    // Calcula os offsets para cada membro
    long offset = tamanho_diretorio;
    for (int i = 0; i < nova_quantidade; i++) {
        dir->membros[i].offset = offset;
        offset += dir->membros[i].tam_disco;
    }
    
    // Cria um arquivo temporário para armazenar os dados
    char temp_file[PATH_MAX];
    snprintf(temp_file, PATH_MAX, "%s.tmp", archive);
    
    // Abre o arquivo temporário para escrita
    FILE *temp = fopen(temp_file, "wb");
    if (!temp) {
        free(dir->membros);
        free(dir);
        free(dados);
        return 1;
    }
    
    // Escreve a quantidade de membros
    if (fwrite(&dir->quantidade, sizeof(int), 1, temp) != 1) {
        fclose(temp);
        remove(temp_file);
        free(dir->membros);
        free(dir);
        free(dados);
        return 1;
    }
    
    // Escreve os membros
    if (fwrite(dir->membros, sizeof(struct Membro), dir->quantidade, temp) != dir->quantidade) {
        fclose(temp);
        remove(temp_file);
        free(dir->membros);
        free(dir);
        free(dados);
        return 1;
    }
    
    // Se houver membros existentes, precisamos copiar seus dados
    if (dir->quantidade > 1) {
        // Abre o arquivo original para leitura
        FILE *arq_old = fopen(archive, "rb");
        if (!arq_old) {
            fclose(temp);
            remove(temp_file);
            free(dir->membros);
            free(dir);
            free(dados);
            return 1;
        }
        
        // Para cada membro existente (exceto o novo)
        for (int i = 0; i < dir->quantidade - 1; i++) {
            // Aloca espaço para os dados do membro
            unsigned char *dados_membro = malloc(dir->membros[i].tam_disco);
            if (!dados_membro) {
                fclose(arq_old);
                fclose(temp);
                remove(temp_file);
                free(dir->membros);
                free(dir);
                free(dados);
                return 1;
            }
            
            // Posiciona no offset do membro no arquivo original
            // Precisamos calcular o offset antigo
            long old_offset = sizeof(int) + (dir->quantidade - 1) * sizeof(struct Membro);
            for (int j = 0; j < i; j++) {
                old_offset += dir->membros[j].tam_disco;
            }
            
            if (fseek(arq_old, old_offset, SEEK_SET) != 0) {
                free(dados_membro);
                fclose(arq_old);
                fclose(temp);
                remove(temp_file);
                free(dir->membros);
                free(dir);
                free(dados);
                return 1;
            }
            
            // Lê os dados do membro
            if (fread(dados_membro, 1, dir->membros[i].tam_disco, arq_old) != dir->membros[i].tam_disco) {
                free(dados_membro);
                fclose(arq_old);
                fclose(temp);
                remove(temp_file);
                free(dir->membros);
                free(dir);
                free(dados);
                return 1;
            }
            
            // Escreve os dados do membro no arquivo temporário
            if (fwrite(dados_membro, 1, dir->membros[i].tam_disco, temp) != dir->membros[i].tam_disco) {
                free(dados_membro);
                fclose(arq_old);
                fclose(temp);
                remove(temp_file);
                free(dir->membros);
                free(dir);
                free(dados);
                return 1;
            }
            
            free(dados_membro);
        }
        
        fclose(arq_old);
    }
    
    // Escreve os dados do novo membro
    if (fwrite(dados, 1, tam_original, temp) != tam_original) {
        fclose(temp);
        remove(temp_file);
        free(dir->membros);
        free(dir);
        free(dados);
        return 1;
    }
    
    // Fecha o arquivo temporário
    fclose(temp);
    
    // Substitui o arquivo original pelo temporário
    if (rename(temp_file, archive) != 0) {
        remove(temp_file);
        free(dir->membros);
        free(dir);
        free(dados);
        return 1;
    }
    
    // Limpeza
    free(dir->membros);
    free(dir);
    free(dados);
    
    return 0;
}


int deve_extrair(const char *nome, const char **membros, int num_membros) {
    if (!membros || num_membros <= 0) {
        // Se não há lista específica, extrai todos
        return 1;
    }
    
    for (int i = 0; i < num_membros; i++) {
        if (membros[i] && strcmp(nome, membros[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

int extrair_membros(const char *archive, const char **membros, int num_membros) {
    fprintf(stderr, "Iniciando extração de membros. num_membros=%d\n", num_membros);
    
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

    fprintf(stderr, "Diretório lido com %d membros\n", dir->quantidade);

    // Extrai cada membro
    for (int i = 0; i < dir->quantidade; i++) {
        struct Membro *m = &dir->membros[i];
        
        fprintf(stderr, "Verificando membro %d: %s\n", i, m->nome);
        
        // Verifica se deve extrair este membro
        int extrair = 0;
        
        if (num_membros == 0) {
            fprintf(stderr, "num_membros é 0, extraindo todos os membros\n");
            extrair = 1;  // Extrai todos os membros
        } else {
            for (int j = 0; j < num_membros; j++) {
                if (membros[j] && strcmp(m->nome, membros[j]) == 0) {
                    fprintf(stderr, "Membro %s encontrado na lista de membros a extrair\n", m->nome);
                    extrair = 1;
                    break;
                }
            }
        }
        
        if (extrair) {
            fprintf(stderr, "Extraindo membro: %s (tamanho: %u bytes, offset: %ld)\n", 
                    m->nome, m->tam_disco, m->offset);
            
            // Posiciona no início dos dados do membro
            if (fseek(arq, m->offset, SEEK_SET) != 0) {
                fprintf(stderr, "Erro ao posicionar no offset %ld\n", m->offset);
                destroi_diretorio(dir);
                fclose(arq);
                return 1;
            }
            
            // Aloca buffer para os dados
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
            
            // Escreve os dados no arquivo de saída
            FILE *saida = fopen(m->nome, "wb");
            if (!saida) {
                fprintf(stderr, "Erro ao criar arquivo de saída: %s\n", m->nome);
                free(buffer);
                destroi_diretorio(dir);
                fclose(arq);
                return 1;
            }
            
            size_t bytes_escritos = fwrite(buffer, 1, m->tam_disco, saida);
            if (bytes_escritos != m->tam_disco) {
                fprintf(stderr, "Erro ao escrever dados no arquivo %s: escrito %zu de %u bytes\n", 
                        m->nome, bytes_escritos, m->tam_disco);
                fclose(saida);
                free(buffer);
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
                
                fprintf(stderr, "Tamanho do arquivo extraído: %ld bytes (esperado: %u bytes)\n", 
                        tamanho_arquivo, m->tam_disco);
                
                // Imprime os primeiros bytes do arquivo extraído
                rewind(verificacao);
                unsigned char primeiros_bytes[10];
                size_t bytes_lidos_verificacao = fread(primeiros_bytes, 1, sizeof(primeiros_bytes), verificacao);
                
                fprintf(stderr, "Primeiros bytes do arquivo extraído: ");
                for (size_t j = 0; j < bytes_lidos_verificacao; j++) {
                    fprintf(stderr, "%02x ", primeiros_bytes[j]);
                }
                fprintf(stderr, "\n");
                
                fclose(verificacao);
            }
            
            free(buffer);
        } else {
            fprintf(stderr, "Membro %s não será extraído\n", m->nome);
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