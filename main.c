#ifdef _WIN32

#include <windows.h>
#include <SDL2/SDL.h>       // Correct if files are in include/SDL2/ // or #include "SDL.h" depending on setup
#include <SDL2/SDL_image.h> // Required for PNG/JPG
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h> // For _O_TEXT
#include <io.h>    // For _open_osfhandle

void RedirectIOToConsole()
{
    AllocConsole(); // Create a console window

    // Redirect stdin
    freopen("CONIN$", "r", stdin);

    // Redirect stdout
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    int fdStdout = _open_osfhandle((intptr_t)hStdout, _O_TEXT);
    FILE *fpStdout = _fdopen(fdStdout, "w");
    freopen("CONOUT$", "w", stdout);
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering

    // Redirect stderr
    HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);
    int fdStderr = _open_osfhandle((intptr_t)hStderr, _O_TEXT);
    FILE *fpStderr = _fdopen(fdStderr, "w");
    freopen("CONOUT$", "w", stderr);
    setvbuf(stderr, NULL, _IONBF, 0);
}

#define WINDOW_WIDTH 850
#define WINDOW_HEIGHT 600
#define BOARD_SIZE 8
#define SQUARE_SIZE (400 / BOARD_SIZE)
#define MAX_MOVES 512

char moveHistory[MAX_MOVES][16];
char moves[MAX_MOVES][10]; // Array to store moves for pgn file
int moveCount = 0;         // Number of moves for pgn file
int running = 1;
int moveCountP = 0; // Number of move in the current game

SDL_Color White = {255, 255, 255, 255};   // white
SDL_Color Grey = {0, 0, 0, 100};          // shadow
SDL_Color Pink = {240, 209, 250, 255};    // bright pink, outilnes text from menu
SDL_Color DarkPink = {185, 50, 230, 255}; // dark pink, text from menu

enum PieceType
{
    EMPTY,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

typedef struct
{
    unsigned int tag : 3;
    unsigned int color : 1; // 0 is white, 1 is black
    unsigned int value : 4; // maxim valoarea 9 la regina, regele are valoare 0

} Piece;
Piece Board[BOARD_SIZE][BOARD_SIZE];

Piece capturedWhite[MAX_MOVES], capturedBlack[MAX_MOVES];
int capturedWhiteCount = 0, capturedBlackCount = 0;

typedef struct coordinates
{
    int x;
    int y;
} Coord_t;

typedef struct
{
    Coord_t from;
    Coord_t to;
    Piece captured; // To support undoing moves
} Move;

Coord_t handleMouseClick(SDL_MouseButtonEvent *click)
{
    Coord_t coord;
    coord.y = (click->x - 80) / SQUARE_SIZE;
    coord.x = (click->y - 80) / SQUARE_SIZE;
    return coord;
}

char *pieceFiles[2][6] = {
    {"assets/pawn.png",
     "assets/knight.png",
     "assets/bishop.png",
     "assets/rook.png",
     "assets/queen.png",
     "assets/king.png"},
    {"assets/pawn1.png",
     "assets/knight1.png",
     "assets/bishop1.png",
     "assets/rook1.png",
     "assets/queen1.png",
     "assets/king1.png"}};

SDL_Texture *pieceTextures[2][6];
int whosTurn = 0;
int validateMove(int initCol, int initRow, int destCol, int destRow);
void casual_mode(SDL_Renderer *renderer);
int isPathClear(int initCol, int initRow, int destCol, int destRow)
{
    int difCol = destCol - initCol;
    int difRow = destRow - initRow;
    int stepCol;
    if (difCol == 0)
    {
        stepCol = 0;
    }
    else
    {
        if (difCol > 0)
        {
            stepCol = 1;
        }
        else
        {
            stepCol = -1;
        }
    }

    int stepRow;
    if (difRow == 0)
    {
        stepRow = 0;
    }
    else
    {
        if (difRow > 0)
        {
            stepRow = 1;
        }
        else
        {
            stepRow = -1;
        }
    }

    int currCol = initCol + stepCol;
    int currRow = initRow + stepRow;

    while (currCol != destCol || currRow != destRow)
    { // pana ajunge la destinatie verifica fiecare pozitie
        if (Board[currRow][currCol].tag != EMPTY)
        {
            return 0; // este blocat
        }
        currCol += stepCol;
        currRow += stepRow;
    }
    return 1; // este cale libera
}

int isSquareAttacked(Coord_t square, int byColor)
{
    // Check if the given square is attacked by pieces of the specified color
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Piece attackingPiece = Board[row][col];

            // Skip empty squares and pieces of the wrong color
            if (attackingPiece.tag == EMPTY || attackingPiece.color != byColor)
                continue;

            // Check if this piece can attack the target square
            if (validateMove(col, row, square.y, square.x))
            {
                return 1; // Square is under attack
            }
        }
    }
    return 0; // Square is not under attack
}

Coord_t findKing(int color)
{
    // Find the king of the specified color
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Piece p = Board[row][col];
            if (p.tag == KING && p.color == color)
            {
                return (Coord_t){row, col};
            }
        }
    }
    // This should never happen in a valid game
    return (Coord_t){-1, -1};
}

int isInCheck(int color)
{
    // Find the king of the specified color
    Coord_t kingPos = findKing(color);

    if (kingPos.x == -1 || kingPos.y == -1)
    {
        return 0; // King not found (shouldn't happen)
    }

    // Check if the king's square is attacked by the opponent
    return isSquareAttacked(kingPos, !color);
}

int wouldMoveResultInCheck(Coord_t from, Coord_t to, int color)
{
    // Make a temporary move to test if it results in check
    Piece tempPiece = Board[to.x][to.y];          // Store the piece that might be captured
    Board[to.x][to.y] = Board[from.x][from.y];    // Move the piece
    Board[from.x][from.y] = (Piece){EMPTY, 0, 0}; // Clear the original square

    int inCheck = isInCheck(color); // Check if the king is in check after the move

    // Restore the board state
    Board[from.x][from.y] = Board[to.x][to.y];
    Board[to.x][to.y] = tempPiece;

    return inCheck;
}

int hasLegalMoves(int color)
{
    // Check if the player has any legal moves
    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++)
    {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++)
        {
            Piece p = Board[fromRow][fromCol];

            // Skip empty squares and opponent's pieces
            if (p.tag == EMPTY || p.color != color)
                continue;

            // Try all possible destination squares
            for (int toRow = 0; toRow < BOARD_SIZE; toRow++)
            {
                for (int toCol = 0; toCol < BOARD_SIZE; toCol++)
                {
                    // Check if the move is valid according to piece rules
                    if (validateMove(fromCol, fromRow, toCol, toRow))
                    {
                        // Check if this move would leave the king in check
                        Coord_t from = {fromRow, fromCol};
                        Coord_t to = {toRow, toCol};

                        if (!wouldMoveResultInCheck(from, to, color))
                        {
                            return 1; // Found at least one legal move
                        }
                    }
                }
            }
        }
    }
    return 0; // No legal moves found
}

int isCheckmate(int color)
{
    return isInCheck(color) && !hasLegalMoves(color);
}

int isStalemate(int color)
{
    return !isInCheck(color) && !hasLegalMoves(color);
}

int didWhiteCastle = 0, didBlackCastle = 0;

void promotePawn(int initCol, int initRow)
{
    Board[initRow][initCol].tag = QUEEN;
    Board[initRow][initCol].value = 9;
}

// Add these global variables for en passant tracking
int enPassantCol = -1; // Column where en passant is possible (-1 if none)
int enPassantRow = -1;
int validateMove(int initCol, int initRow, int destCol, int destRow)
{
    // printf("%c %d %d : %d %d %c\n\n", Board[initRow][initCol].piece, initRow, initCol, destRow, destCol, Board[destRow][destCol].piece);
    Piece Initial = Board[initRow][initCol];
    if (Initial.tag == EMPTY)
    {
        return 0;
    }

    Piece Destination = Board[destRow][destCol];
    if (Destination.tag != EMPTY && Destination.color == Initial.color)
    {
        return 0;
    }

    int difCol = destCol - initCol;
    int difRow = destRow - initRow;

    // Common path checking function for sliding pieces
    switch (Initial.tag)
    {
    case PAWN:
        if (Initial.color == 1)
        { // Black Pawn
            // Single move forward
            if (difRow == 1 && difCol == 0 && Destination.tag == EMPTY)
            {
                return 1;
            }
            // Double move forward from starting position
            if (initRow == 1 && difRow == 2 && difCol == 0 && Destination.tag == EMPTY)
            {
                // Check the square in between
                if (Board[initRow + 1][initCol].tag == EMPTY)
                {
                    return 1;
                }
            }

            // Diagonal capture
            if (difRow == 1 && abs(difCol) == 1 && Destination.tag != EMPTY && Destination.color != Initial.color)
            {
                return 1;
            }
        }
        else
        { // White Pawn
            if (difRow == -1 && difCol == 0 && Destination.tag == EMPTY)
            {
                return 1;
            }
            // Double move forward from starting position
            if (initRow == 6 && difRow == -2 && difCol == 0 && Destination.tag == EMPTY)
            {
                // Check the square in between
                if (Board[initRow - 1][initCol].tag == EMPTY)
                {
                    return 1;
                }
            }
            // Diagonal capture
            if (difRow == -1 && abs(difCol) == 1 && Destination.tag != EMPTY && Destination.color != Initial.color)
            {
                return 1;
            }
            // if (initRow == 3 && abs(difRow) == 1 && abs(difCol) == 1 && Destination.tag != EMPTY)
            // {
            //     return 1;
            // }
        }
        break;
    case ROOK:
        if (difRow == 0 || difCol == 0)
        {
            return isPathClear(initCol, initRow, destCol, destRow);
        }
        break;

    case BISHOP:
        if (abs(difRow) == abs(difCol))
        {
            return isPathClear(initCol, initRow, destCol, destRow);
        }
        break;

    case QUEEN:
        if (difRow == 0 || difCol == 0 || abs(difRow) == abs(difCol))
        {
            return isPathClear(initCol, initRow, destCol, destRow);
        }
        break;

    case KING:
        if (abs(difRow) <= 1 && abs(difCol) <= 1)
        {
            return 1;
        }
        if (difRow == 0 && abs(difCol) == 2)
        {
            // Check if king is on starting position
            int startingRow = (Initial.color == 1) ? 0 : 7; // Black king row 0, White king row 7
            if (initRow != startingRow || initCol != 4)
            {
                break; // King not on starting position
            }

            // Determine rook position based on castling direction
            int rookCol = (difCol == 2) ? 7 : 0; // Kingside (right) or Queenside (left)

            // Check if rook exists and hasn't moved
            if (Board[startingRow][rookCol].tag != ROOK || Board[startingRow][rookCol].color != Initial.color)
            {
                break; // No rook or wrong color rook
            }

            // Check if path between king and rook is clear
            int step = (difCol > 0) ? 1 : -1;
            for (int col = initCol + step; col != rookCol; col += step)
            {
                if (Board[startingRow][col].tag != EMPTY)
                {
                    break; // Path is blocked
                }
            }
            int rookNewCol = (difCol == 2) ? 5 : 3; // Kingside rook goes to col 5, Queenside to col 3

            // Move rook from corner to new position
            // Board[startingRow][rookNewCol] = Board[startingRow][rookCol];
            // Board[startingRow][rookCol].tag = EMPTY;

            if (Initial.color == 0)
            {
                didWhiteCastle = 1;
            }
            else if (Initial.color == 1)
            {
                didBlackCastle = 1;
            }
            // If we reach here, castling is valid
            // Note: This doesn't check for check/checkmate conditions
            // You may want to add those checks separately

            return 2;
        }
        break;

    case KNIGHT:
        if ((abs(difRow) == 2 && abs(difCol) == 1) || (abs(difRow) == 1 && abs(difCol) == 2))
        {
            return 1;
        }
        break;

    default:
        return 0;
    }

    // whosTurn = !whosTurn; // Only change turn if move is valid
    return 0;
}

char pgnMoves[MAX_MOVES * 10]; // Stores all moves in PGN format
int moveNumber = 1;            // Tracks the current move number
void open_progress_window();
char *transform_to_pgn(Coord_t moveFrom, Coord_t moveTo, Piece piece, int isCapture)
{
    static char pgnMove[16]; // Buffer to store the PGN move

    // Handle castling
    if (piece.tag == KING && abs(moveTo.y - moveFrom.y) == 2)
    {
        if (moveTo.y > moveFrom.y)
        { // Kingside castling
            snprintf(pgnMove, sizeof(pgnMove), "O-O");
        }
        else
        { // Queenside castling
            snprintf(pgnMove, sizeof(pgnMove), "O-O-O");
        }
        return pgnMove;
    }

    // Handle pawn moves
    if (piece.tag == PAWN)
    {
        if (isCapture)
        {
            snprintf(pgnMove, sizeof(pgnMove), "%c%x%c%d", 'a' + moveFrom.y, 8 - moveFrom.x, 'a' + moveTo.y, 8 - moveTo.x);
        }
        else
        {
            snprintf(pgnMove, sizeof(pgnMove), "%c%d", 'a' + moveTo.y, 8 - moveTo.x);
        }
        return pgnMove;
    }

    // Handle other pieces
    char pieceChar;
    switch (piece.tag)
    {
    case KNIGHT:
        pieceChar = 'N';
        break;
    case BISHOP:
        pieceChar = 'B';
        break;
    case ROOK:
        pieceChar = 'R';
        break;
    case QUEEN:
        pieceChar = 'Q';
        break;
    case KING:
        pieceChar = 'K';
        break;
    default:
        pieceChar = '?';
        break; // Unknown piece
    }

    if (isCapture)
    {
        snprintf(pgnMove, sizeof(pgnMove), "%c%c%dx%c%d", pieceChar, 'a' + moveFrom.y, 8 - moveFrom.x, 'a' + moveTo.y, 8 - moveTo.x);
    }
    else
    {
        snprintf(pgnMove, sizeof(pgnMove), "%c%c%d%c%d", pieceChar, 'a' + moveFrom.y, 8 - moveFrom.x, 'a' + moveTo.y, 8 - moveTo.x);
    }

    return pgnMove;
}

int makeAIMove(Coord_t moveFrom, Coord_t moveTo) //, FILE *fileText
{
    // First validate the move according to piece rules
    if (!validateMove(moveFrom.y, moveFrom.x, moveTo.y, moveTo.x))
    {
        printf("Move is invalid according to piece rules\n");
        return 0;
    }

    // Check if this move would leave the current player's king in check
    if (wouldMoveResultInCheck(moveFrom, moveTo, whosTurn))
    {
        printf("Move is invalid - would leave king in check\n");
        return 0;
    }
    // Piece piece = Board[moveFrom.x][moveFrom.y];
    // int isCapture = (Board[moveTo.x][moveTo.y].tag != EMPTY);
    // snprintf(moveHistory[moveCountP], sizeof(moveHistory[moveCountP]),
    //          "%d. %c%d -> %c%d", moveCountP + 1,
    //          'a' + moveFrom.y, 8 - moveFrom.x, // Convert coordinates to chess notation
    //          'a' + moveTo.y, 8 - moveTo.x);
    // moveCountP++;
    // // Transform move to PGN style
    // char *pgnMove = transform_to_pgn(moveFrom, moveTo, piece, isCapture);

    // Append PGN move to pgnMoves
    // if (whosTurn == 0)
    // { // White's move
    //     snprintf(pgnMoves + strlen(pgnMoves), sizeof(pgnMoves) - strlen(pgnMoves), "%d. %s ", moveNumber, pgnMove);
    // }
    // else
    // { // Black's move
    //     snprintf(pgnMoves + strlen(pgnMoves), sizeof(pgnMoves) - strlen(pgnMoves), "%s ", pgnMove);
    //     moveNumber++; // Increment move number after Black's move
    // }
    // Make the move
    Piece capturedPiece = Board[moveTo.x][moveTo.y]; // Store for potential undo
    Board[moveTo.x][moveTo.y] = Board[moveFrom.x][moveFrom.y];
    Board[moveFrom.x][moveFrom.y] = (Piece){EMPTY, 0, 0};

    // Switch turns
    whosTurn = !whosTurn;

    // Check game state after the move
    if (isCheckmate(whosTurn))
    {
        printf("Checkmate! Player %d wins!\n", !whosTurn);
        // You might want to set a game over flag here
    }
    else if (isStalemate(whosTurn))
    {
        printf("Stalemate! The game is a draw!\n");
        // You might want to set a game over flag here
    }
    else if (isInCheck(whosTurn))
    {
        printf("Check!\n");
    }
    return 1;
}

int makeMove(Coord_t moveFrom, Coord_t moveTo) //, FILE *fileText
{
    // First validate the move according to piece rules
    int valid = validateMove(moveFrom.y, moveFrom.x, moveTo.y, moveTo.x);
    if (!valid)
    {
        printf("Move is invalid according to piece rules\n");
        return 0;
    }
    printf("the valid is: %d", valid);
    // Check if this move would leave the current player's king in check
    if (wouldMoveResultInCheck(moveFrom, moveTo, whosTurn))
    {
        printf("Move is invalid - would leave king in check\n");
        return 0;
    }
    Piece piece = Board[moveFrom.x][moveFrom.y];
    int isCapture = (Board[moveTo.x][moveTo.y].tag != EMPTY);
    snprintf(moveHistory[moveCountP], sizeof(moveHistory[moveCountP]),
             "%d. %c%d -> %c%d", moveCountP + 1,
             'a' + moveFrom.y, 8 - moveFrom.x, // Convert coordinates to chess notation
             'a' + moveTo.y, 8 - moveTo.x);
    moveCountP++;
    // Transform move to PGN style
    char *pgnMove = transform_to_pgn(moveFrom, moveTo, piece, isCapture);

    // Append PGN move to pgnMoves
    if (whosTurn == 0)
    { // White's move
        snprintf(pgnMoves + strlen(pgnMoves), sizeof(pgnMoves) - strlen(pgnMoves), "%d. %s ", moveNumber, pgnMove);
    }
    else
    { // Black's move
        snprintf(pgnMoves + strlen(pgnMoves), sizeof(pgnMoves) - strlen(pgnMoves), "%s ", pgnMove);
        moveNumber++; // Increment move number after Black's move
    }

    if (isCapture)
    {
        if (Board[moveTo.x][moveTo.y].color == 0)
        {
            capturedWhite[capturedWhiteCount++] = Board[moveTo.x][moveTo.y];
        }
        else
        {
            capturedBlack[capturedBlackCount++] = Board[moveTo.x][moveTo.y];
        }
    }

    // Make the move
    Piece capturedPiece = Board[moveTo.x][moveTo.y]; // Store for potential undo
    Board[moveTo.x][moveTo.y] = Board[moveFrom.x][moveFrom.y];
    Board[moveFrom.x][moveFrom.y] = (Piece){EMPTY, 0, 0};
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (Board[0][i].tag == PAWN && Board[0][i].color == 0)
            promotePawn(i, 0);
        else if (Board[7][i].tag == PAWN && Board[7][i].color == 1)
            promotePawn(i, 7);
    }
    if (valid == 2)
    {
        printf("%d", abs(moveFrom.y - moveTo.y));
        if (moveFrom.y - moveTo.y < 0)
        {

            printf("ok");
            Board[moveFrom.x][5] = Board[moveFrom.x][7];
            Board[moveFrom.x][7].tag = EMPTY;
        }
        else
        {
            printf("oki");
            Board[moveFrom.x][3] = Board[moveFrom.x][0];
            Board[moveFrom.x][0].tag = EMPTY;
        }
    }

    if (valid == 3)
    {
        promotePawn(moveTo.x, moveTo.y);
    }
    // Switch turns
    whosTurn = !whosTurn;

    // Check game state after the move
    if (isCheckmate(whosTurn))
    {
        printf("Checkmate! Player %d wins!\n", !whosTurn);
        // You might want to set a game over flag here
    }
    else if (isStalemate(whosTurn))
    {
        printf("Stalemate! The game is a draw!\n");
        // You might want to set a game over flag here
    }
    else if (isInCheck(whosTurn))
    {
        printf("Check!\n");
    }
    return 1;
}

void setupBoard()
{
    memset(Board, 0, sizeof(Board));
    // Pawns
    for (int i = 0; i < 8; i++)
    {
        Board[1][i] = (Piece){PAWN, 1, 1}; // Black
        Board[6][i] = (Piece){PAWN, 0, 1}; // White
    }
    // Rooks
    Board[0][0] = Board[0][7] = (Piece){ROOK, 1, 5};
    Board[7][0] = Board[7][7] = (Piece){ROOK, 0, 5};
    // Knights
    Board[0][1] = Board[0][6] = (Piece){KNIGHT, 1, 3};
    Board[7][1] = Board[7][6] = (Piece){KNIGHT, 0, 3};
    // Bishops
    Board[0][2] = Board[0][5] = (Piece){BISHOP, 1, 3};
    Board[7][2] = Board[7][5] = (Piece){BISHOP, 0, 3};
    // Queens
    Board[0][3] = (Piece){QUEEN, 1, 9};
    Board[7][3] = (Piece){QUEEN, 0, 9};
    // Kings
    Board[0][4] = (Piece){KING, 1, 0};
    Board[7][4] = (Piece){KING, 0, 0};
}

void drawPieces(SDL_Renderer *renderer)
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Piece p = Board[row][col];
            if (p.tag != EMPTY)
            {
                SDL_Rect dest = {
                    col * SQUARE_SIZE + 85, // x position
                    row * SQUARE_SIZE + 85, // y position
                    SQUARE_SIZE - 10,       // width
                    SQUARE_SIZE - 10        // height
                };
                SDL_RenderCopy(renderer, pieceTextures[p.color][p.tag - 1], NULL, &dest);
            }
        }
    }
}

void render_multiline_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color, int x, int y, int line_spacing)
{
    char *text_copy = strdup(text); // Duplicate string so we can modify it
    if (!text_copy)
        return;

    char *line = strtok(text_copy, "\n");
    int offset_y = 0;

    while (line)
    {
        SDL_Surface *surface = TTF_RenderUTF8_Blended(font, line, color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dst = {x, y + offset_y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        offset_y += surface->h + line_spacing;

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        line = strtok(NULL, "\n");
    }

    free(text_copy);
}
void parse_pgn(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Failed  opento PGN file: %s\n", filename);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        // Skip metadata lines (lines starting with '[')
        if (line[0] == '[')
        {
            continue;
        }

        // Tokenize the line to extract moves
        char *token = strtok(line, " \n");
        while (token)
        {
            // Skip move numbers (e.g., "1.", "2.")
            if (strchr(token, '.'))
            {
                token = strtok(NULL, " \n");
                continue;
            }

            // Store the move
            strncpy(moves[moveCount], token, sizeof(moves[moveCount]) - 1);
            moves[moveCount][sizeof(moves[moveCount]) - 1] = '\0';
            moveCount++;
            printf("%s\n", moves[moveCount - 1]);
            token = strtok(NULL, " \n");
        }
    }

    fclose(file);
}

int pgn_to_coords(const char *move, Coord_t *from, Coord_t *to)
{
    // printf("%s\n", move);
    // Handle castling
    if (strcmp(move, "O-O") == 0)
    {
        if (whosTurn == 0) // White
        {
            *from = (Coord_t){7, 4};
            *to = (Coord_t){7, 6};
        }
        else // Black
        {
            *from = (Coord_t){0, 4};
            *to = (Coord_t){0, 6};
        }
        return 1;
    }
    else if (strcmp(move, "O-O-O") == 0)
    {
        if (whosTurn == 0) // White
        {
            *from = (Coord_t){7, 4};
            *to = (Coord_t){7, 2};
        }
        else // Black
        {
            *from = (Coord_t){0, 4};
            *to = (Coord_t){0, 2};
        }
        return 1;
    }

    int len = strlen(move);

    // Skip move numbers and annotations like "+", "#", "!", "?"
    const char *clean_move = move;
    while (*clean_move && (*clean_move < 'A' || *clean_move > 'z'))
        clean_move++;

    // Remove trailing annotations
    char temp_move[20];
    strcpy(temp_move, clean_move);
    len = strlen(temp_move);
    while (len > 0 && (temp_move[len - 1] == '+' || temp_move[len - 1] == '#' ||
                       temp_move[len - 1] == '!' || temp_move[len - 1] == '?'))
    {
        temp_move[--len] = '\0';
    }

    // Handle standard moves (e.g., e4, Nf3)
    int col = temp_move[len - 2] - 'a';       // Column (file)
    int row = 8 - (temp_move[len - 1] - '0'); // Row (rank)
    // printf("%d %d\n", row, col);              // Fixed: print row, col in correct order

    int pieceType; // Piece type
    if (temp_move[0] >= 'a' && temp_move[0] <= 'h')
    {
        pieceType = PAWN;
    }
    else
    {
        switch (temp_move[0])
        {
        case 'K':
            pieceType = KING;
            break;
        case 'Q':
            pieceType = QUEEN;
            break;
        case 'R':
            pieceType = ROOK;
            break;
        case 'B':
            pieceType = BISHOP;
            break;
        case 'N':
            pieceType = KNIGHT;
            break;
        default:
            return 0; // Invalid piece
        }
    }

    int prefer_row = -1, prefer_col = -1;

    // Parse disambiguation (e.g., Nbd2, N1f3, Rh1h8)
    for (int i = 1; i < len - 2; i++)
    {
        if (temp_move[i] == 'x') // Skip capture symbol
            continue;

        if (temp_move[i] >= 'a' && temp_move[i] <= 'h')
        {
            prefer_col = temp_move[i] - 'a'; // Fixed: this should set prefer_col, not prefer_row
        }
        else if (temp_move[i] >= '1' && temp_move[i] <= '8')
        {
            prefer_row = 8 - (temp_move[i] - '0'); // Fixed: this should set prefer_row, not prefer_col
        }
    }

    // Find the piece to move
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            if (pieceType != Board[r][c].tag || Board[r][c].color != whosTurn)
            {
                continue; // Skip if the piece type doesn't match
            }
            if (prefer_row != -1 && prefer_row != r)
            {
                continue;
            }
            if (prefer_col != -1 && prefer_col != c)
            {
                continue;
            }
            // Check if the piece can move to the target square
            if (validateMove(c, r, col, row))
            {
                *from = (Coord_t){r, c};
                *to = (Coord_t){row, col};
                return 1;
            }
        }
    }

    return 0; // Move not found
}
void exit_game()
{
    printf("Exit button clicked\n");
    // SDL_Quit();
    // exit(0);
    setupBoard();
    whosTurn = 0;
    moveCount = 0;
    moveCountP = 0;
    memset(moveHistory, 0, sizeof(moveHistory));
    // Set running to 0 to exit the main loop
}

void save_game()
{
    printf("Save button clicked\n");

    FILE *file = fopen("save.pgn", "w");
    if (!file)
    {
        printf("Failed to open file for saving\n");
        return;
    }

    // Write metadata
    fprintf(file, "[Event \"Casual Game\"]\n");
    fprintf(file, "[Site \"Local\"]\n");
    fprintf(file, "[Date \"2025.06.05\"]\n");
    fprintf(file, "[Round \"-\"]\n");
    fprintf(file, "[White \"Player1\"]\n");
    fprintf(file, "[Black \"Player2\"]\n");
    fprintf(file, "[Result \"*\"]\n\n");

    // Write moves
    fprintf(file, "%s\n", pgnMoves);

    fclose(file);
    printf("Game saved to game.pgn\n");
}

void reload_game(SDL_Renderer *renderer)
{
    printf("Reload button clicked\n");

    FILE *file = fopen("save.pgn", "r");
    if (!file)
    {
        printf("failed to open");
    }

    setupBoard();
    whosTurn = moveCount % 2;
    memset(moveHistory, 0, sizeof(moveHistory));
    memset(pgnMoves, 0, sizeof(pgnMoves));
    moveCountP = 0;
    moveCount = 0;
    moveNumber = 1;

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == '[')
            continue;

        char *token = strtok(line, " \n");
        while (token)
        {
            if (strchr(token, '.'))
            {
                token = strtok(NULL, " \n");
                continue;
            }

            strncpy(moves[moveCount], token, sizeof(moves[moveCount]) - 1);
            moves[moveCount][sizeof(moves[moveCount]) - 1] = '\0';
            moveCount++;
            token = strtok(NULL, " \n");
        }
    }
    fclose(file);

    for (int i = 0; i < moveCount; i++)
    {
        Coord_t from, to;
        if (pgn_to_coords(moves[i], &from, &to))
        {
            makeMove(from, to);
            printf("");
        }
        else
        {
            printf("Invalis move");
        }
    }
    whosTurn = moveCount % 2;

    casual_mode(renderer);
}

void tutorial_mode(SDL_Renderer *renderer)
{
    printf("Tutorial mode started\n");

    parse_pgn("save.pgn"); // Load the PGN file
    printf("Loaded %d moves from PGN file\n", moveCount);
    for (int i = 0; i < moveCount; i++)
    {
        Coord_t from, to;
        if (pgn_to_coords(moves[i], &from, &to))
        {
            makeMove(from, to);
            // printf("Move %d: %s\n", i + 1, moves[i]);

            // Render the board
            SDL_SetRenderDrawColor(renderer, 48, 0, 72, 255);
            SDL_RenderClear(renderer);

            // Render chessboard
            for (int row = 0; row < BOARD_SIZE; row++)
            {
                for (int col = 0; col < BOARD_SIZE; col++)
                {
                    SDL_Rect square = {col * SQUARE_SIZE + 80, row * SQUARE_SIZE + 80, SQUARE_SIZE, SQUARE_SIZE};
                    if ((row + col) % 2 == 0)
                        SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255);
                    else
                        SDL_SetRenderDrawColor(renderer, 185, 50, 230, 255);
                    SDL_RenderFillRect(renderer, &square);
                }
            }

            // Render pieces
            drawPieces(renderer);

            SDL_RenderPresent(renderer);

            SDL_Delay(2000); // Pause for 1 second between moves
        }
        else
        {
            printf("Invalid move: %s\n", moves[i]);
        }
    }

    printf("Tutorial mode finished\n");
}

const int PAWN_POSITION_BONUS[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5, 5, 10, 25, 25, 10, 5, 5},
    {0, 0, 0, 20, 20, 0, 0, 0},
    {5, -5, -10, 0, 0, -10, -5, 5},
    {5, 10, 10, -20, -20, 10, 10, 5},
    {0, 0, 0, 0, 0, 0, 0, 0}};

const int KNIGHT_POSITION_BONUS[8][8] = {
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0, 0, 0, 0, -20, -40},
    {-30, 0, 10, 15, 15, 10, 0, -30},
    {-30, 5, 15, 20, 20, 15, 5, -30},
    {-30, 0, 15, 20, 20, 15, 0, -30},
    {-30, 5, 10, 15, 15, 10, 5, -30},
    {-40, -20, 0, 5, 5, 0, -20, -40},
    {-50, -40, -30, -30, -30, -30, -40, -50}};

const int CENTER_CONTROL_BONUS[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 5, 5, 5, 5, 5, 5, 0},
    {0, 5, 10, 10, 10, 10, 5, 0},
    {0, 5, 10, 15, 15, 10, 5, 0},
    {0, 5, 10, 15, 15, 10, 5, 0},
    {0, 5, 10, 10, 10, 10, 5, 0},
    {0, 5, 5, 5, 5, 5, 5, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}};

float staticEvaluation()
{
    int score = 0;

    // Material evaluation
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Piece piece = Board[row][col];
            if (piece.tag != EMPTY)
            {
                int pieceValue = piece.value * 10;

                // Add position bonuses
                int positionBonus = 0;
                if (piece.tag == PAWN)
                {
                    positionBonus = PAWN_POSITION_BONUS[row][col];
                }
                else if (piece.tag == KNIGHT)
                {
                    positionBonus = KNIGHT_POSITION_BONUS[row][col];
                }
                else
                {
                    positionBonus = CENTER_CONTROL_BONUS[row][col];
                }

                // Flip position bonus for black pieces
                if (piece.color == 1)
                {
                    positionBonus = PAWN_POSITION_BONUS[7 - row][col];
                }

                int totalValue = pieceValue + positionBonus;

                if (piece.color == 0)
                { // White
                    score += totalValue;
                }
                else
                { // Black
                    score -= totalValue;
                }
            }
        }
    }

    // King safety evaluation
    // score += evaluateKingSafety();

    // Mobility evaluation (number of legal moves)
    // score += evaluateMobility();

    return score;
}

void copyBoard(Piece dest[BOARD_SIZE][BOARD_SIZE], Piece src[BOARD_SIZE][BOARD_SIZE])
{
    memcpy(dest, src, sizeof(Piece) * BOARD_SIZE * BOARD_SIZE);
}
int minimax(int depth, int alpha, int beta, int maximizingPlayer)
{
    if (depth == 0)
    {
        return staticEvaluation();
    }

    int best = maximizingPlayer ? -10000 : 10000;
    Piece tempBoard[BOARD_SIZE][BOARD_SIZE];
    copyBoard(tempBoard, Board);
    int turnBefore = whosTurn;

    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++)
    {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++)
        {
            Piece p = Board[fromRow][fromCol];
            if (p.tag == EMPTY || p.color != maximizingPlayer)
                continue;

            for (int toRow = 0; toRow < BOARD_SIZE; toRow++)
            {
                for (int toCol = 0; toCol < BOARD_SIZE; toCol++)
                {
                    if (!validateMove(fromCol, fromRow, toCol, toRow))
                        continue;

                    Coord_t from = {fromRow, fromCol};
                    Coord_t to = {toRow, toCol};

                    makeAIMove(from, to);
                    int score = minimax(depth - 1, alpha, beta, !maximizingPlayer);
                    copyBoard(Board, tempBoard); // Undo move
                    whosTurn = turnBefore;       // Restore turn

                    if (maximizingPlayer) // Score for the white player
                    {
                        if (score > best)
                            best = score;
                        if (score > alpha)
                            alpha = score;
                    }
                    else // score for the black player
                    {
                        if (score < best)
                            best = score;
                        if (score < beta)
                            beta = score;
                    }

                    if (beta <= alpha) // pruning condition
                        return best;
                }
            }
        }
    }

    return best;
}

Coord_t lastAIMoveFrom = {-1, -1};
Coord_t lastAIMoveTo = {-1, -1};

int ai_make_move()
{
    int bestScore = 10000;
    Coord_t bestFrom = {-1, -1};
    Coord_t bestTo = {-1, -1};

    Piece tempBoard[BOARD_SIZE][BOARD_SIZE];
    copyBoard(tempBoard, Board);
    int turnBefore = whosTurn;

    for (int fromRow = 0; fromRow < BOARD_SIZE; fromRow++)
    {
        for (int fromCol = 0; fromCol < BOARD_SIZE; fromCol++)
        {
            Piece p = Board[fromRow][fromCol];
            if (p.tag == EMPTY || p.color != 1)
                continue; // AI is Black

            for (int toRow = 0; toRow < BOARD_SIZE; toRow++)
            {
                for (int toCol = 0; toCol < BOARD_SIZE; toCol++)
                {
                    if (!validateMove(fromCol, fromRow, toCol, toRow))
                        continue;

                    Coord_t from = {fromRow, fromCol};
                    Coord_t to = {toRow, toCol};

                    makeAIMove(from, to);
                    int score = minimax(3, -10000, 10000, 0);
                    copyBoard(Board, tempBoard);
                    whosTurn = turnBefore;

                    if (score < bestScore &&
                        !(from.x == lastAIMoveTo.x && from.y == lastAIMoveTo.y &&
                          to.x == lastAIMoveFrom.x && to.y == lastAIMoveFrom.y))
                    {
                        // Avoid going back and forth between two squares
                        bestScore = score;
                        bestFrom = from;
                        bestTo = to;
                    }
                }
            }
        }
    }

    if (bestFrom.x != -1 && bestTo.x != -1)
    {
        printf("AI moves from (%d, %d) to (%d, %d) with eval %d\n", bestFrom.x, bestFrom.y, bestTo.x, bestTo.y, bestScore);
        makeMove(bestFrom, bestTo);
        lastAIMoveFrom = bestFrom;
        lastAIMoveTo = bestTo;
        printf("score: %d", staticEvaluation());
        return 1;
    }

    printf("AI found no legal move.\n");
    return 0;
}

Coord_t selectedPiece = {-1, -1};

void renderMoveHighlights(SDL_Renderer *renderer, Coord_t from)
{
    if (from.x == -1 || from.y == -1)
        return; // No piece selected

    // Enable alpha blending for transparency
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (validateMove(from.y, from.x, j, i))
            {
                SDL_Rect square = {j * SQUARE_SIZE + 80, i * SQUARE_SIZE + 80, SQUARE_SIZE, SQUARE_SIZE};

                // Set a semi-transparent green color for valid moves
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100); // Green with 100/255 opacity
                SDL_RenderFillRect(renderer, &square);

                // Optional: Add a border for better visibility
                SDL_SetRenderDrawColor(renderer, 0, 200, 0, 200); // Darker green border
                SDL_RenderDrawRect(renderer, &square);
            }
        }
    }

    // Highlight the selected piece with a different color
    SDL_Rect selectedSquare = {from.y * SQUARE_SIZE + 80, from.x * SQUARE_SIZE + 80, SQUARE_SIZE, SQUARE_SIZE};
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 120); // Yellow highlight for selected piece
    SDL_RenderFillRect(renderer, &selectedSquare);

    // Reset blend mode
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void renderCollectedPieces(SDL_Renderer *renderer)
{
    int xOffsetWhite = 200;
    int xOffsetBlack = 200;
    int yOffsetWhite = 520;
    int yOffsetBlack = 550;
    int pieceSpacing = 1;
    for (int i = 0; i < capturedWhiteCount; i++)
    {
        SDL_Rect dest = {
            xOffsetWhite + i * (SQUARE_SIZE / 2 + pieceSpacing),
            yOffsetWhite,
            SQUARE_SIZE / 2,
            SQUARE_SIZE / 2};
        SDL_RenderCopy(renderer, pieceTextures[0][capturedWhite[i].tag - 1], NULL, &dest);
    }

    for (int i = 0; i < capturedBlackCount; i++)
    {
        SDL_Rect dest = {
            xOffsetBlack + i * (SQUARE_SIZE / 2 + pieceSpacing),
            yOffsetBlack,
            SQUARE_SIZE / 2,
            SQUARE_SIZE / 2};
        SDL_RenderCopy(renderer, pieceTextures[1][capturedBlack[i].tag - 1], NULL, &dest);
    }
}

void generateLegalMoves(Coord_t from)
{
    // printf("%d %d\n", from.x, from.y);
    selectedPiece = from;
}

void trainer_mode(SDL_Renderer *renderer)
{
    printf("Trainer mode (Human vs AI) started\n");

    int move = 0;            // Keeps track of whose turn it is (0 = Human/White, 1 = AI/Black)
    Coord_t From = {-1, -1}; // Initialize with invalid coordinates
    Coord_t To = {-1, -1};   // Initialize with invalid coordinates
    int runningGameMode = 1;
    while (runningGameMode)
    {
        if (move % 2 == 0)
        { // Human's turn
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    runningGameMode = 0;
                    break;
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    int mx = event.button.x; // Mouse X coordinate
                    int my = event.button.y;
                    Coord_t clicked = handleMouseClick(&event.button);

                    // Ensure the click is within the chessboard bounds
                    if (clicked.x >= 0 && clicked.x < BOARD_SIZE && clicked.y >= 0 && clicked.y < BOARD_SIZE)
                    {
                        if (From.x == -1 && From.y == -1)
                        { // Selecting a piece
                            Piece selectedPiece = Board[clicked.x][clicked.y];
                            if (selectedPiece.tag != EMPTY && selectedPiece.color == 0)
                            {                   // Human can only move White pieces
                                From = clicked; // Store the selected piece's coordinates
                                generateLegalMoves(clicked);
                                printf("Selected piece at (%d, %d)\n", From.x, From.y);
                            }
                            else
                            {
                                printf("Invalid selection. Select your own piece.\n");
                            }
                        }
                        else
                        { // Selecting a destination square
                            To = clicked;
                            printf("Attempting move: (%d, %d) -> (%d, %d)\n", From.x, From.y, To.x, To.y);

                            // Attempt to make the move
                            if (makeMove(From, To))
                            {
                                printf("Move made: (%d, %d) -> (%d, %d)\n", From.x, From.y, To.x, To.y);
                                move++; // Switch turn to AI
                            }
                            else
                            {
                                printf("Invalid move. Try again.\n");
                            }

                            // Reset selection
                            From.x = -1;
                            From.y = -1;
                            To.x = -1;
                            To.y = -1;
                        }
                    }
                    if (mx >= 680 && mx <= 680 + 130 &&
                        my >= 200 && my <= 200 + 50)
                    {
                        runningGameMode = 0;
                    }
                    if (mx >= 534 && mx <= 534 + 130 &&
                        my >= 340 && my <= 340 + 50)
                    {
                        save_game();
                    }

                    if (mx >= 534 && mx <= 534 + 276 &&
                        my >= 420 && my <= 420 + 50)
                    {
                        open_progress_window();
                    }
                }
            }
        }
        else
        { // AI's turn
            printf("AI's turn\n");
            SDL_Delay(500); // Add a small delay to simulate thinking time
            if (ai_make_move())
            {
                move++; // Switch turn to Human
            }
            else
            {
                printf("AI could not make a move\n");
            }
        }

        // Render only the chessboard and pieces
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                SDL_Rect square = {col * SQUARE_SIZE + 80, row * SQUARE_SIZE + 80, SQUARE_SIZE, SQUARE_SIZE};
                if ((row + col) % 2 == 0)
                    SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 185, 50, 230, 255);
                SDL_RenderFillRect(renderer, &square);
            }
        }

        // Render pieces
        drawPieces(renderer);

        renderMoveHighlights(renderer, selectedPiece);

        SDL_RenderPresent(renderer);
    }
    exit_game();
}

SDL_Rect turnIndicatorRect = {110, 540, 50, 20};

void casual_mode(SDL_Renderer *renderer)
{
    printf("Casual mode (Human vs Human) started\n");

    int move = 0;            // Keeps track of whose turn it is (0 = White, 1 = Black)
    Coord_t From = {-1, -1}; // Initialize with invalid coordinates
    Coord_t To = {-1, -1};   // Initialize with invalid coordinates

    int runningGameMode = 1;
    while (runningGameMode)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                runningGameMode = 0;
                break;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                int mx = event.button.x; // Mouse X coordinate
                int my = event.button.y; // Mouse Y coordinate
                Coord_t clicked = handleMouseClick(&event.button);

                // Ensure the click is within the chessboard bounds
                if (clicked.x >= 0 && clicked.x < BOARD_SIZE && clicked.y >= 0 && clicked.y < BOARD_SIZE)
                {
                    if (From.x == -1 && From.y == -1)
                    { // Selecting a piece
                        Piece selectedPieceObj = Board[clicked.x][clicked.y];
                        if (selectedPieceObj.tag != EMPTY && selectedPieceObj.color == whosTurn)
                        {
                            From = clicked;              // Store the selected piece's coordinates
                            generateLegalMoves(clicked); // This now just stores the selection
                            // printf("Selected piece at (%d, %d)\n", From.x, From.y);
                        }
                        else
                        {
                            printf("Invalid selection. Select your own piece.\n");
                            selectedPiece.x = -1; // Clear selection
                            selectedPiece.y = -1;
                        }
                    }
                    else
                    { // Selecting a destination square
                        To = clicked;
                        // printf("Attempting move: (%d, %d) -> (%d, %d)\n", From.x, From.y, To.x, To.y);

                        // Attempt to make the move
                        if (makeMove(From, To))
                        {
                            printf("%d \n", staticEvaluation());
                            // printf("Move made: (%d, %d) -> (%d, %d)\n", From.x, From.y, To.x, To.y);
                            move++; // Switch turn
                        }
                        else
                        {
                            printf("Invalid move. Try again.\n");
                        }

                        // Reset selection
                        From.x = -1;
                        From.y = -1;
                        To.x = -1;
                        To.y = -1;
                        selectedPiece.x = -1; // Clear move highlights
                        selectedPiece.y = -1;
                    }
                }
                if (mx >= 680 && mx <= 680 + 130 &&
                    my >= 200 && my <= 200 + 50)
                {
                    runningGameMode = 0;
                }
                if (mx >= 534 && mx <= 534 + 130 &&
                    my >= 340 && my <= 340 + 50)
                {
                    save_game();
                }

                if (mx >= 534 && mx <= 534 + 276 &&
                    my >= 420 && my <= 420 + 50)
                {
                    open_progress_window();
                }
            }
        }

        // Render the chessboard with original colors
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                SDL_Rect square = {col * SQUARE_SIZE + 80, row * SQUARE_SIZE + 80, SQUARE_SIZE, SQUARE_SIZE};
                if ((row + col) % 2 == 0)
                    SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 185, 50, 230, 255);
                SDL_RenderFillRect(renderer, &square);
            }
        }

        // Render pieces
        drawPieces(renderer);
        if (whosTurn == 0)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }
        SDL_RenderFillRect(renderer, &turnIndicatorRect);
        // Render move highlights on top of everything
        renderMoveHighlights(renderer, selectedPiece);
        renderCollectedPieces(renderer);
        SDL_RenderPresent(renderer);
    }
    exit_game();
}

void open_progress_window()
{
    SDL_Window *progressWindow = SDL_CreateWindow("Progress", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 200, 300, SDL_WINDOW_SHOWN);
    if (!progressWindow)
    {
        SDL_Log("Failed to create progress window: %s", SDL_GetError());
        return;
    }

    SDL_Renderer *progressRenderer = SDL_CreateRenderer(progressWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!progressRenderer)
    {
        SDL_DestroyWindow(progressWindow);
        SDL_Log("Failed to create progress renderer: %s", SDL_GetError());
        return;
    }

    TTF_Font *progressFont = TTF_OpenFont("./assets/CutePixel.ttf", 24);
    if (!progressFont)
    {
        SDL_Log("Failed to load font for progress window: %s", TTF_GetError());
        SDL_DestroyRenderer(progressRenderer);
        SDL_DestroyWindow(progressWindow);
        return;
    }

    int running = 1;
    SDL_Event e;
    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = 0; // Handle window close button
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = 0; // Handle ESC key
                }
            }
            else if (e.type == SDL_WINDOWEVENT)
            {
                if (e.window.event == SDL_WINDOWEVENT_CLOSE &&
                    e.window.windowID == SDL_GetWindowID(progressWindow))
                {
                    running = 0; // Specifically handle this window's close event
                }
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(progressRenderer, Pink.r, Pink.g, Pink.b, Pink.a); // Light blue background
        SDL_RenderClear(progressRenderer);

        // Render move history
        SDL_Color textColor = {0, 0, 0, 255}; // Black text
        int yOffset = 10;                     // Start rendering text 10 pixels from the top
        for (int i = 0; i < moveCountP; i++)
        {
            SDL_Surface *textSurface = TTF_RenderText_Blended(progressFont, moveHistory[i], textColor);
            if (textSurface)
            {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(progressRenderer, textSurface);
                SDL_Rect textRect = {10, yOffset, textSurface->w, textSurface->h};
                SDL_RenderCopy(progressRenderer, textTexture, NULL, &textRect);
                SDL_FreeSurface(textSurface);
                SDL_DestroyTexture(textTexture);
                yOffset += textRect.h + 5; // Move down for the next line
            }
        }

        SDL_RenderPresent(progressRenderer);
    }

    // Cleanup
    TTF_CloseFont(progressFont);
    SDL_DestroyRenderer(progressRenderer);
    SDL_DestroyWindow(progressWindow);
}

int main(int argc, char *argv[])
{
    setupBoard();
    RedirectIOToConsole();
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        printf("SDL_image initialization failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() != 0)
    {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        return 1;
    }
    TTF_Font *largeFont = TTF_OpenFont("./assets/CutePixel.ttf", 70);  // Large text
    TTF_Font *mediumFont = TTF_OpenFont("./assets/CutePixel.ttf", 39); // Medium text
    TTF_Font *smallFont = TTF_OpenFont("./assets/CutePixel.ttf", 28);  // Small text

    if (!largeFont || !mediumFont || !smallFont)
    {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        return 1;
    }

    // 2. Create a window
    SDL_Window *window = SDL_CreateWindow(
        "Eșec",                 // Window title
        SDL_WINDOWPOS_CENTERED, // Initial x position
        SDL_WINDOWPOS_CENTERED, // Initial y position
        WINDOW_WIDTH,           // Width
        WINDOW_HEIGHT,          // Height
        SDL_WINDOW_SHOWN        // Flags
    );

    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 3. Create a renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    for (int color = 0; color < 2; color++)
    {
        for (int type = 0; type < 6; type++)
        {
            SDL_Surface *surf = IMG_Load(pieceFiles[color][type]);
            if (!surf)
            {
                printf("Failed to load %s\n", pieceFiles[color][type]);
                return 1;
            }
            pieceTextures[color][type] = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_FreeSurface(surf);
        }
    }

    // SDL_Color White = {255, 255, 255, 255};   // white
    // SDL_Color Grey = {0, 0, 0, 100};          // shadow
    // SDL_Color Pink = {240, 209, 250, 255};    // bright pink, outilnes text from menu
    // SDL_Color DarkPink = {185, 50, 230, 255}; // dark pink, text from menu

    const char *multilineText = "8\n7\n6\n5\n4\n3\n2\n1";

    SDL_Rect buttonRects[7] = {
        {534, 120, 130, 50}, {680, 120, 130, 50}, {534, 200, 130, 50}, {680, 200, 130, 50}, {534, 340, 130, 50}, {680, 340, 130, 50}, {534, 420, 276, 50}};
    const char *buttonTexts[7] = {"Tutorial", "Trainer", "Casual", "Exit", "Save", "Load", "Progress"};

    SDL_Texture *buttonTextures[7];
    SDL_Texture *buttonShadowTextures[7];
    SDL_Rect buttonTextRects[7];
    SDL_Rect buttonShadowRects[7];
    SDL_Color buttonTextColor = {185, 50, 230, 255};
    SDL_Color buttonShadowColor = {240, 209, 250, 255};

    for (int i = 0; i < 7; i++)
    {
        SDL_Surface *text = TTF_RenderText_Blended(mediumFont, buttonTexts[i], White);
        buttonTextures[i] = SDL_CreateTextureFromSurface(renderer, text);
        buttonTextRects[i] = (SDL_Rect){
            buttonRects[i].x + (buttonRects[i].w - text->w) / 2,
            buttonRects[i].y + (buttonRects[i].h - text->h) / 2,
            text->w, text->h};
        SDL_FreeSurface(text);
    }

    Coord_t From;
    Coord_t To;
    // 4. Main loop

    int move = 0;
    int started = 0;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                int mx = event.button.x; // Mouse X coordinate
                int my = event.button.y; // Mouse Y coordinate

                for (int i = 0; i < 7; i++)
                {
                    if (mx >= buttonRects[i].x && mx <= buttonRects[i].x + buttonRects[i].w &&
                        my >= buttonRects[i].y && my <= buttonRects[i].y + buttonRects[i].h)
                    {
                        // Button clicked
                        switch (i)
                        {
                        case 0: // Tutorial button
                            tutorial_mode(renderer);
                            break;
                        case 1: // Trainer button
                            trainer_mode(renderer);
                            break;
                        case 2: // Casual button
                            casual_mode(renderer);
                            break;
                        case 3: // Exit button
                            // running = 0; // Exit the game
                            exit_game();
                            break;
                        case 4: // Save button
                            save_game();
                            break;
                        case 5: // Load button
                            reload_game(renderer);
                            break;
                        case 6: // Progress button
                            open_progress_window();
                            break;
                        default:
                            printf("Unknown button clicked\n");
                            break;
                        }
                    }
                }
            }
        }
        // Render everything
        SDL_SetRenderDrawColor(renderer, 48, 0, 72, 255);
        SDL_RenderClear(renderer);

        SDL_Rect extraRect = {50, 100, 410, 410};
        SDL_SetRenderDrawColor(renderer, 112, 0, 112, 255); // shadow color for board;
        SDL_RenderFillRect(renderer, &extraRect);

        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                SDL_Rect square = {col * SQUARE_SIZE + 80, row * SQUARE_SIZE + 80, SQUARE_SIZE, SQUARE_SIZE};
                if ((row + col) % 2 == 0)
                    SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 185, 50, 230, 255);
                SDL_RenderFillRect(renderer, &square);
            }
        }

        for (int i = 0; i < 7; i++)
        {
            SDL_SetRenderDrawColor(renderer, 112, 0, 112, 255); //  shadow color for button
            SDL_Rect shadowRect = {
                buttonRects[i].x - 4, // Offset shadow by 3 pixels
                buttonRects[i].y + 4,
                buttonRects[i].w,
                buttonRects[i].h};
            SDL_RenderFillRect(renderer, &shadowRect);
            SDL_SetRenderDrawColor(renderer, 228, 135, 224, 100); // outline color for button
            SDL_RenderFillRect(renderer, &buttonRects[i]);
            // SDL_RenderCopy(renderer, buttonShadowTextures[i], NULL, &buttonShadowRects[i]);
            SDL_RenderCopy(renderer, buttonTextures[i], NULL, &buttonTextRects[i]);
        }

        // Title and menu labels
        SDL_Surface *textSurface5 = TTF_RenderText_Blended(largeFont, "PixelChess", Grey);
        SDL_Texture *textTexture5 = SDL_CreateTextureFromSurface(renderer, textSurface5);
        SDL_Rect textRect5 = {52, 17, textSurface5->w, textSurface5->h};
        SDL_FreeSurface(textSurface5); // shadow for title
        SDL_RenderCopy(renderer, textTexture5, NULL, &textRect5);
        SDL_DestroyTexture(textTexture5);
        SDL_Surface *titleSurface = TTF_RenderText_Blended(largeFont, "PixelChess", White);
        SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        SDL_Rect titleRect = {60, 10, titleSurface->w, titleSurface->h};
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_FreeSurface(titleSurface);
        SDL_DestroyTexture(titleTexture);

        SDL_Surface *textSurface3 = TTF_RenderText_Blended(largeFont, "Menu", Grey);
        SDL_Texture *textTexture3 = SDL_CreateTextureFromSurface(renderer, textSurface3);
        SDL_Rect textRect3 = {598, 37, textSurface3->w, textSurface3->h};
        SDL_FreeSurface(textSurface3); // shadow for menu
        SDL_RenderCopy(renderer, textTexture3, NULL, &textRect3);
        SDL_DestroyTexture(textTexture3);
        SDL_Surface *menuSurface = TTF_RenderText_Blended(largeFont, "Menu", White);
        SDL_Texture *menuTexture = SDL_CreateTextureFromSurface(renderer, menuSurface);
        SDL_Rect menuRect = {605, 30, menuSurface->w, menuSurface->h};
        SDL_RenderCopy(renderer, menuTexture, NULL, &menuRect);
        SDL_FreeSurface(menuSurface);
        SDL_DestroyTexture(menuTexture);

        SDL_Surface *textSurface2 = TTF_RenderText_Blended(smallFont, " a    b    c    d     e    f    g    h", White);
        SDL_Texture *textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
        SDL_Rect textRect2 = {92, 480, textSurface2->w, textSurface2->h};
        SDL_FreeSurface(textSurface2);
        SDL_RenderCopy(renderer, textTexture2, NULL, &textRect2);
        SDL_DestroyTexture(textTexture2);

        render_multiline_text(renderer, smallFont, multilineText, White, 63, 107, 18);

        drawPieces(renderer);

        SDL_RenderPresent(renderer);
    }

    for (int i = 0; i < 7; i++)
    {
        SDL_DestroyTexture(buttonTextures[i]);
        // SDL_DestroyTexture(buttonShadowTextures[i]);
    }
    for (int color = 0; color < 2; color++)
        for (int type = 0; type < 6; type++)
            SDL_DestroyTexture(pieceTextures[color][type]);

    TTF_CloseFont(mediumFont);
    TTF_CloseFont(largeFont);
    TTF_CloseFont(smallFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
#endif
