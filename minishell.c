#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// Limites do shell
#define MAX_LINE 1024
#define MAX_ARGS 64
#define DELIM " \t\r\n"

// Função para ler uma linha da entrada padrão
char* read_line() {
    char *line = malloc(MAX_LINE);
    if (!line) {
        fprintf(stderr, "Erro de alocação\n");
        exit(EXIT_FAILURE);
    }
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        free(line);
        return NULL;
    }
    return line;
}

// Parser simples que tokeniza a linha e retorna o número de tokens
int parse_line(char *line, char **args) {
    int pos = 0;
    char *token = strtok(line, DELIM);
    while (token != NULL && pos < MAX_ARGS - 1) {
        args[pos++] = token;
        token = strtok(NULL, DELIM);
    }
    args[pos] = NULL;
    return pos;
}

// Verifica se é comando interno
int is_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) return 1;
    if (strcmp(args[0], "exit") == 0) return 1;
    return 0;
}

// Função para executar comandos internos
int exec_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: falha, argumento esperado\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }
    return 0;
}

// Executa comando simples (sem pipes ou redirecionamento)
void exec_command(char **args) {
    if (args[0] == NULL) return;

    if (is_builtin(args)) {
        exec_builtin(args);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Filho: executa o comando
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        // Pai: espera filho terminar
        waitpid(pid, NULL, 0);
    }
}

// Função para encontrar operador de pipe na linha (retorna índice ou -1)
int find_pipe(char **args) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "|") == 0)
            return i;
    }
    return -1;
}

// Trata execução de pipeline simples (dois comandos separados por '|')
void exec_pipe(char **args) {
    int pipe_index = find_pipe(args);
    if (pipe_index == -1) {
        exec_command(args);  // Sem pipe, executa normalmente
        return;
    }

    args[pipe_index] = NULL;  // Separa comando 1 e 2

    char **left_cmd = args;
    char **right_cmd = &args[pipe_index + 1];

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {  // Filho 1 executa comando da esquerda
        close(fd[0]);  // Fecha lado de leitura do pipe
        dup2(fd[1], STDOUT_FILENO); // Redireciona saída padrão para pipe
        close(fd[1]);
        exec_command(left_cmd);
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {  // Filho 2 executa comando da direita
        close(fd[1]);  // Fecha lado de escrita do pipe
        dup2(fd[0], STDIN_FILENO);  // Redireciona entrada padrão para pipe
        close(fd[0]);
        exec_command(right_cmd);
        exit(EXIT_FAILURE);
    }

    // Pai fecha ambos os lados do pipe e espera filhos
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// Verifica e executa redirecionamentos (<, >)
void handle_redirection(char **args) {
    int i = 0;
    int in_redirect = -1, out_redirect = -1;

    while (args[i]) {
        if (strcmp(args[i], "<") == 0) {
            in_redirect = i;
        } else if (strcmp(args[i], ">") == 0) {
            out_redirect = i;
        }
        i++;
    }

    if (in_redirect != -1) {
        if (args[in_redirect + 1] == NULL) {
            fprintf(stderr, "Erro: arquivo esperado após '<'\n");
            return;
        }
        int fd_in = open(args[in_redirect + 1], O_RDONLY);
        if (fd_in < 0) {
            perror("open input");
            return;
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
        args[in_redirect] = NULL;
    }

    if (out_redirect != -1) {
        if (args[out_redirect + 1] == NULL) {
            fprintf(stderr, "Erro: arquivo esperado após '>'\n");
            return;
        }
        int fd_out = open(args[out_redirect + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror("open output");
            return;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
        args[out_redirect] = NULL;
    }
}

// Executa comando considerando redirecionamento e pipe
void exec_with_redirection_and_pipe(char **args) {
    int pipe_idx = find_pipe(args);
    if (pipe_idx == -1) {
        // Redirecionamento direto
        pid_t pid = fork();
        if (pid == 0) {
            handle_redirection(args);
            if (is_builtin(args))
                exec_builtin(args);
            else
                execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        else if (pid < 0) {
            perror("fork");
        }
        else {
            waitpid(pid, NULL, 0);
        }
    } else {
        // Pipeline - só suporta um pipe simples
        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe");
            return;
        }

        pid_t pid1 = fork();
        if (pid1 == 0) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            args[pipe_idx] = NULL;
            handle_redirection(args);
            if (is_builtin(args))
                exec_builtin(args);
            else
                execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        pid_t pid2 = fork();
        if (pid2 == 0) {
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            char **right_cmd = &args[pipe_idx + 1];
            handle_redirection(right_cmd);
            if (is_builtin(right_cmd))
                exec_builtin(right_cmd);
            else
                execvp(right_cmd[0], right_cmd);
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        close(fd[0]);
        close(fd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
}

int main() {
    while (1) {
        printf("mini-shell$ ");
        fflush(stdout);

        char *line = read_line();
        if (line == NULL) {
            printf("\nSaindo do shell.\n");
            break;
        }

        // Remove quebra de linha
        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        char *args[MAX_ARGS];
        parse_line(line, args);

        if (args[0] == NULL) {
            free(line);
            continue;
        }

        // Comando interno 'exit' tratado separadamente para sair do shell
        if (strcmp(args[0], "exit") == 0) {
            free(line);
            break;
        }

        exec_with_redirection_and_pipe(args);

        free(line);
    }

    return 0;
}
