#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BOARD_SIZE 8

// Representação das peças
typedef enum {
    EMPTY,
    PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING
} PieceType;

typedef enum {
    NONE,
    WHITE,
    BLACK
} PieceColor;

typedef struct {
    PieceType type;
    PieceColor color;
} Piece;

typedef struct {
    Piece board[BOARD_SIZE][BOARD_SIZE];
    PieceColor turn;
} ChessGame;

// Inicializa o tabuleiro com posições iniciais padrão
void init_board(ChessGame *game) {
    // Zera tudo
    for(int r=0; r<BOARD_SIZE; r++) {
        for(int c=0; c<BOARD_SIZE; c++) {
            game->board[r][c].type = EMPTY;
            game->board[r][c].color = NONE;
        }
    }
    game->turn = WHITE;

    // Peças pretas
    game->board[0][0] = game->board[0][7] = (Piece){ROOK, BLACK};
    game->board[0][1] = game->board[0][6] = (Piece){KNIGHT, BLACK};
    game->board[0][2] = game->board[0][5] = (Piece){BISHOP, BLACK};
    game->board[0][3] = (Piece){QUEEN, BLACK};
    game->board[0][4] = (Piece){KING, BLACK};
    for(int c=0; c<BOARD_SIZE; c++) {
        game->board[1][c] = (Piece){PAWN, BLACK};
    }

    // Peças brancas
    game->board[7][0] = game->board[7][7] = (Piece){ROOK, WHITE};
    game->board[7][1] = game->board[7][6] = (Piece){KNIGHT, WHITE};
    game->board[7][2] = game->board[7][5] = (Piece){BISHOP, WHITE};
    game->board[7][3] = (Piece){QUEEN, WHITE};
    game->board[7][4] = (Piece){KING, WHITE};
    for(int c=0; c<BOARD_SIZE; c++) {
        game->board[6][c] = (Piece){PAWN, WHITE};
    }
}

// Retorna caractere do símbolo da peça para mostrar no tabuleiro
char piece_symbol(Piece p) {
    if (p.color == NONE) return '.';
    char symbols[] = {'.','P','R','N','B','Q','K'};
    char symbol = symbols[p.type];
    if (p.color == BLACK) symbol = tolower(symbol);
    return symbol;
}

// Imprime o tabuleiro no terminal
void print_board(ChessGame *game) {
    printf("  a b c d e f g h\n");
    for(int r=0; r<BOARD_SIZE; r++) {
        printf("%d ", 8 - r);
        for(int c=0; c<BOARD_SIZE; c++) {
            printf("%c ", piece_symbol(game->board[r][c]));
        }
        printf("%d\n", 8 - r);
    }
    printf("  a b c d e f g h\n");
}

// Converte posição no formato x='a'-'h', y='1'-'8' para índices matriz [0-7][0-7]
int pos_to_index(char file, char rank, int *row, int *col) {
    if(file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return 0;
    }
    *col = file - 'a';
    *row = 8 - (rank - '0');
    return 1;
}

// Verifica se posição está dentro do tabuleiro
int in_bounds(int r, int c) {
    return (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE);
}

// Verifica se caminho está livre entre origem e destino para movimentos lineares (torre, bispo, rainha)
int path_clear(ChessGame *game, int r1, int c1, int r2, int c2) {
    int dr = (r2 > r1) ? 1 : (r2 < r1) ? -1 : 0;
    int dc = (c2 > c1) ? 1 : (c2 < c1) ? -1 : 0;

    int r = r1 + dr;
    int c = c1 + dc;
    while (r != r2 || c != c2) {
        if (game->board[r][c].type != EMPTY) return 0;
        r += dr; c += dc;
    }
    return 1;
}

// Valida movimento de peão
int valid_pawn_move(ChessGame *game, int r1, int c1, int r2, int c2) {
    Piece p = game->board[r1][c1];
    int direction = (p.color == WHITE) ? -1 : 1;
    int start_row = (p.color == WHITE) ? 6 : 1;

    // avanço simples
    if(c1 == c2 && r2 == r1 + direction && game->board[r2][c2].type == EMPTY) return 1;
    // avanço duplo na posição inicial
    if(c1 == c2 && r1 == start_row && r2 == r1 + 2*direction &&
       game->board[r1 + direction][c1].type == EMPTY && game->board[r2][c2].type == EMPTY) return 1;
    // captura diagonal
    if(abs(c2 - c1) == 1 && r2 == r1 + direction && game->board[r2][c2].type != EMPTY &&
       game->board[r2][c2].color != p.color) return 1;
    return 0;
}

// Valida movimento de cavalo
int valid_knight_move(int r1, int c1, int r2, int c2) {
    int dr = abs(r2 - r1);
    int dc = abs(c2 - c1);
    return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
}

// Valida movimentos rook (torre)
int valid_rook_move(ChessGame *game, int r1, int c1, int r2, int c2) {
    if (r1 != r2 && c1 != c2) return 0;
    return path_clear(game, r1, c1, r2, c2);
}

// Valida movimentos bishop (bispo)
int valid_bishop_move(ChessGame *game, int r1, int c1, int r2, int c2) {
    if (abs(r2 - r1) != abs(c2 - c1)) return 0;
    return path_clear(game, r1, c1, r2, c2);
}

// Valida movimentos queen (rainha)
int valid_queen_move(ChessGame *game, int r1, int c1, int r2, int c2) {
    if (r1 == r2 || c1 == c2)
        return valid_rook_move(game, r1, c1, r2, c2);
    else if (abs(r2 - r1) == abs(c2 - c1))
        return valid_bishop_move(game, r1, c1, r2, c2);
    return 0;
}

// Valida movimento king (rei)
int valid_king_move(int r1, int c1, int r2, int c2) {
    int dr = abs(r2 - r1);
    int dc = abs(c2 - c1);
    return (dr <= 1 && dc <= 1);
}

// Valida se movimento é válido para a peça na origem
int valid_move(ChessGame *game, int r1, int c1, int r2, int c2) {
    if (!in_bounds(r1,c1) || !in_bounds(r2,c2)) return 0;
    Piece from = game->board[r1][c1];
    Piece to = game->board[r2][c2];

    if (from.type == EMPTY || from.color != game->turn) return 0;
    if (to.color == from.color) return 0;

    switch(from.type) {
        case PAWN: return valid_pawn_move(game, r1, c1, r2, c2);
        case KNIGHT: return valid_knight_move(r1,c1,r2,c2);
        case BISHOP: return valid_bishop_move(game, r1, c1, r2, c2);
        case ROOK: return valid_rook_move(game, r1, c1, r2, c2);
        case QUEEN: return valid_queen_move(game, r1, c1, r2, c2);
        case KING: return valid_king_move(r1,c1,r2,c2);
        default: return 0;
    }
}

// Executa movimento validado, retorna 1 se rei capturado = fim
int make_move(ChessGame *game, int r1, int c1, int r2, int c2) {
    Piece to = game->board[r2][c2];
    if (to.type == KING) {
        // Rei capturado: fim do jogo
        game->board[r2][c2] = game->board[r1][c1];
        game->board[r1][c1].type = EMPTY;
        game->board[r1][c1].color = NONE;
        return 1;
    }
    // Move peça
    game->board[r2][c2] = game->board[r1][c1];
    game->board[r1][c1].type = EMPTY;
    game->board[r1][c1].color = NONE;
    return 0;
}

// Alterna turno entre Branco e Preto
void switch_turn(ChessGame *game) {
    game->turn = (game->turn == WHITE) ? BLACK : WHITE;
}

// Lê e valida entrada do usuário com formato como "e2 e4"
int read_move(int *r1, int *c1, int *r2, int *c2) {
    char from[3], to[3];
    printf("Digite seu movimento (exemplo e2 e4): ");
    if(scanf(" %2s %2s", from, to) != 2) {
        while(getchar() != '\n');
        return 0;
    }
    while(getchar() != '\n');
    if(!pos_to_index(tolower(from[0]), from[1], r1, c1)) return 0;
    if(!pos_to_index(tolower(to[0]), to[1], r2, c2)) return 0;
    return 1;
}

// Função principal do jogo
int main() {
    ChessGame game;
    init_board(&game);

    printf("Jogo de Xadrez Simples\n");
    print_board(&game);

    while (1) {
        printf("Turno do %s\n", game.turn == WHITE ? "BRANCO" : "PRETO");
        int r1,c1,r2,c2;
        if (!read_move(&r1,&c1,&r2,&c2)) {
            printf("Entrada inválida. Tente novamente.\n");
            continue;
        }
        if (!valid_move(&game, r1,c1,r2,c2)) {
            printf("Movimento inválido. Tente outro.\n");
            continue;
        }
        if(make_move(&game, r1,c1,r2,c2)) {
            print_board(&game);
            printf("%s ganhou! Rei capturado.\n", game.turn == WHITE ? "BRANCO" : "PRETO");
            break;
        }
        print_board(&game);
        switch_turn(&game);
    }
    return 0;
}
