#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 1000      // Limite de linhas no texto
#define MAX_LINE_LENGTH 256 // Máximo de caracteres por linha
#define UNDO_REDO_DEPTH 100 // Máximo de operações armazenadas para undo/redo

// Estrutura básica para armazenar o texto como array de linhas
typedef struct {
    char *lines[MAX_LINES];
    int line_count;
} TextBuffer;

// Estrutura para armazenar estado do texto para undo/redo
typedef struct {
    TextBuffer states[UNDO_REDO_DEPTH];
    int top;       // posição atual para undo/redo
    int max_state; // número máximo de estados salvo
} UndoRedoStack;

// Inicializa o buffer de texto, aloca memória para cada linha
void init_text_buffer(TextBuffer *txt) {
    txt->line_count = 0;
    for (int i = 0; i < MAX_LINES; i++) {
        txt->lines[i] = (char*) malloc(MAX_LINE_LENGTH);
        txt->lines[i][0] = '\0';
    }
}

// Copia conteúdo de um buffer para outro (para salvar estados)
void copy_text_buffer(TextBuffer *dest, TextBuffer *src) {
    dest->line_count = src->line_count;
    for (int i = 0; i < src->line_count; i++) {
        strcpy(dest->lines[i], src->lines[i]);
    }
    for (int i = src->line_count; i < MAX_LINES; i++) {
        dest->lines[i][0] = '\0';
    }
}

// Inicializa pilha de undo/redo
void init_undo_redo(UndoRedoStack *stack) {
    stack->top = -1;
    stack->max_state = -1;
}

// Salva estado atual do texto para undo/redo
void save_state(UndoRedoStack *stack, TextBuffer *txt) {
    if (stack->top < UNDO_REDO_DEPTH - 1) {
        stack->top++;
        stack->max_state = stack->top;
        copy_text_buffer(&stack->states[stack->top], txt);
    } else {
        // Se atingiu limite, descarta o mais antigo e desloca para esquerda
        for (int i = 0; i < UNDO_REDO_DEPTH - 1; i++) {
            copy_text_buffer(&stack->states[i], &stack->states[i+1]);
        }
        copy_text_buffer(&stack->states[UNDO_REDO_DEPTH-1], txt);
    }
}

// Faz undo: retorna ao estado anterior se possível
int undo(UndoRedoStack *stack, TextBuffer *txt) {
    if (stack->top > 0) {
        stack->top--;
        copy_text_buffer(txt, &stack->states[stack->top]);
        return 1;
    }
    printf("Nada para desfazer.\n");
    return 0;
}

// Faz redo: avança ao próximo estado se possível
int redo(UndoRedoStack *stack, TextBuffer *txt) {
    if (stack->top < stack->max_state) {
        stack->top++;
        copy_text_buffer(txt, &stack->states[stack->top]);
        return 1;
    }
    printf("Nada para refazer.\n");
    return 0;
}

// Carrega arquivo para buffer de texto, linha a linha
int load_file(const char *filename, TextBuffer *txt) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Não foi possível abrir o arquivo %s para leitura.\n", filename);
        return 0;
    }
    init_text_buffer(txt);
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file) && txt->line_count < MAX_LINES) {
        line[strcspn(line, "\r\n")] = 0; // remove \n ou \r\n
        strcpy(txt->lines[txt->line_count], line);
        txt->line_count++;
    }
    fclose(file);
    return 1;
}

// Salva o buffer de texto no arquivo
int save_file(const char *filename, TextBuffer *txt) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Não foi possível abrir o arquivo %s para escrita.\n", filename);
        return 0;
    }
    for (int i = 0; i < txt->line_count; i++) {
        fprintf(file, "%s\n", txt->lines[i]);
    }
    fclose(file);
    return 1;
}

// Exibe o conteúdo atual do buffer com numeração de linhas
void display_text(TextBuffer *txt) {
    printf("\n=== Texto Atualmente ===\n");
    for (int i = 0; i < txt->line_count; i++) {
        printf("%3d: %s\n", i+1, txt->lines[i]);
    }
    printf("=======================\n");
}

// Insere nova linha no índice informado (1-based), empurrando as linhas para baixo
void insert_line(TextBuffer *txt, int index, const char *line) {
    if (txt->line_count >= MAX_LINES) {
        printf("Limite de linhas atingido.\n");
        return;
    }
    if (index < 1 || index > txt->line_count + 1) {
        printf("Índice inválido para inserção.\n");
        return;
    }
    for (int i = txt->line_count; i >= index; i--) {
        strcpy(txt->lines[i], txt->lines[i - 1]);
    }
    strncpy(txt->lines[index - 1], line, MAX_LINE_LENGTH);
    txt->line_count++;
}

// Edita linha existente (1-based)
void edit_line(TextBuffer *txt, int index, const char *line) {
    if (index < 1 || index > txt->line_count) {
        printf("Índice inválido para edição.\n");
        return;
    }
    strncpy(txt->lines[index - 1], line, MAX_LINE_LENGTH);
}

// Remove linha do índice informado (1-based), puxando linhas para cima
void remove_line(TextBuffer *txt, int index) {
    if (index < 1 || index > txt->line_count) {
        printf("Índice inválido para remoção.\n");
        return;
    }
    for (int i = index -1; i < txt->line_count -1; i++) {
        strcpy(txt->lines[i], txt->lines[i+1]);
    }
    txt->lines[txt->line_count -1][0] = '\0';
    txt->line_count--;
}

// Menu principal de operações
void menu(TextBuffer *txt, UndoRedoStack *urs) {
    int running = 1;
    while (running) {
        printf("\n--- Editor de Texto Simples ---\n");
        printf("1. Exibir texto\n");
        printf("2. Inserir linha\n");
        printf("3. Editar linha\n");
        printf("4. Remover linha\n");
        printf("5. Desfazer (Undo)\n");
        printf("6. Refazer (Redo)\n");
        printf("7. Salvar arquivo\n");
        printf("8. Carregar arquivo\n");
        printf("9. Sair\n");
        printf("Escolha: ");

        int choice, idx;
        char line[MAX_LINE_LENGTH];
        char filename[256];
        scanf("%d", &choice);
        while(getchar() != '\n');

        switch (choice) {
            case 1:
                display_text(txt);
                break;
            case 2:
                printf("Digite o número da linha para inserir: ");
                scanf("%d", &idx);
                while(getchar() != '\n');
                printf("Digite o texto a ser inserido: ");
                fgets(line, sizeof(line), stdin);
                line[strcspn(line, "\r\n")] = 0;
                save_state(urs, txt);
                insert_line(txt, idx, line);
                break;
            case 3:
                printf("Digite o número da linha para editar: ");
                scanf("%d", &idx);
                while(getchar() != '\n');
                printf("Digite o novo texto: ");
                fgets(line, sizeof(line), stdin);
                line[strcspn(line, "\r\n")] = 0;
                save_state(urs, txt);
                edit_line(txt, idx, line);
                break;
            case 4:
                printf("Digite o número da linha para remover: ");
                scanf("%d", &idx);
                while(getchar() != '\n');
                save_state(urs, txt);
                remove_line(txt, idx);
                break;
            case 5:
                if(undo(urs, txt)) {
                    printf("Undo realizado.\n");
                }
                break;
            case 6:
                if(redo(urs, txt)) {
                    printf("Redo realizado.\n");
                }
                break;
            case 7:
                printf("Digite o nome do arquivo para salvar: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\r\n")] = 0;
                if (save_file(filename, txt)) {
                    printf("Arquivo salvo com sucesso.\n");
                }
                break;
            case 8:
                printf("Digite o nome do arquivo para carregar: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\r\n")] = 0;
                if (load_file(filename, txt)) {
                    printf("Arquivo carregado com sucesso.\n");
                    save_state(urs, txt); // Estado inicial para undo
                }
                break;
            case 9:
                running = 0;
                break;
            default:
                printf("Opção inválida.\n");
        }
    }
}

// FUNÇÃO PRINCIPAL
int main() {
    TextBuffer txt;
    UndoRedoStack urs;
    init_text_buffer(&txt);
    init_undo_redo(&urs);
    save_state(&urs, &txt); // Estado inicial vazio

    menu(&txt, &urs);

    // Liberação de memória
    for (int i=0; i<MAX_LINES; i++) {
        free(txt.lines[i]);
    }

    return 0;
}
