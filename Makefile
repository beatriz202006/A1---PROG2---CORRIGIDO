# Makefile para o projeto

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Arquivos fonte e objetos
SRCS = main.c archive.c diretorio.c
OBJS = $(SRCS:.c=.o)

# Nome do executável
EXEC = login/vinac

# Regra padrão
all: $(EXEC)

# Regra para criar o executável
$(EXEC): $(OBJS)
	@mkdir -p login
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Regra para compilar os arquivos objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Regra para limpar os arquivos gerados
clean:
	rm -f $(OBJS) $(EXEC)
