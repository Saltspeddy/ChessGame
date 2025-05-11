#ifdef _WIN32
#include <windows.h>
#include <SDL2/SDL.h>       // Correct if files are in include/SDL2/ // or #include "SDL.h" depending on setup
#include <SDL2/SDL_image.h> // Required for PNG/JPG
// #include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define BOARD_SIZE 8
#define SQUARE_SIZE (500 / BOARD_SIZE)

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

struct cheessBoard
{
    unsigned int color : 1; // 0 is white, 1 is black
    unsigned int value : 4; // maxim valoarea 9 la regina, regele are valoare 0
    unsigned int tag : 3;   // 6 piese se reprezinta pe 3 biti
    char piece;

} Board[BOARD_SIZE][BOARD_SIZE];

typedef struct coordinates
{
    int x;
    int y;
} Coord_t;

int whosTurn = 0;

int validateMove(int initCol, int initRow, int destCol, int destRow)
{
    printf("%c %d %d : %d %d %c\n\n", Board[initRow][initCol].piece, initRow, initCol, destRow, destCol, Board[destRow][destCol].piece);

    struct cheessBoard Piece = Board[initRow][initCol];
    if (Piece.piece == '_')
    {
        return 0;
    }

    struct cheessBoard Destination = Board[destRow][destCol];
    if (Destination.piece != '_' && Destination.color == Piece.color)
    {
        return 0;
    }

    // if (whosTurn != Piece.color)
    // {
    //     return 0;
    // }

    int difCol = destCol - initCol;
    int difRow = destRow - initRow;

    // Common path checking function for sliding pieces
    int isPathClear(int initCol, int initRow, int destCol, int destRow)
    {
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
            if (Board[currRow][currCol].piece != '_')
            {
                return 0; // este blocat
            }
            currCol += stepCol;
            currRow += stepRow;
        }
        return 1; // este cale libera
    }

    switch (Piece.tag)
    {
    case PAWN:
        if (Piece.color == 1)
        { // Black Pawn
            if (initRow == 6 && difRow == 1 && difCol == 0 && Destination.tag == EMPTY)
            {
                // promotePawn(initCol, initRow);
                return 1;
            }
            if (initRow == 6 && difRow == 1 && abs(difCol) == 1 && Destination.tag != EMPTY)
            {
                // promotePawn(initCol, initRow);
                return 1;
            }
            if (difRow == 1 && difCol == 0 && Destination.tag == EMPTY)
            {
                return 1;
            }
            if (difRow == 2 && difCol == 0 && initRow == 1 && Destination.tag == EMPTY)
            {
                // Check square in between for 2-square move
                if (Board[initRow + 1][initCol].piece != '_')
                {
                    return 0;
                }
                return 1;
            }
            if (difRow == 1 && abs(difCol) == 1 && Destination.tag != EMPTY)
            {
                return 1;
            }
        }
        else
        { // White Pawn
            if (initRow == 1 && difRow == -1 && difCol == 0 && Destination.tag == EMPTY)
            {
                // promotePawn(initCol, initRow);
                return 1;
            }
            if (initRow == 1 && difRow == -1 && abs(difCol) == 1 && Destination.tag != EMPTY)
            {
                // promotePawn(initCol, initRow);
                return 1;
            }
            if (difRow == -1 && difCol == 0 && Destination.tag == EMPTY)
            {
                return 1;
            }
            if (difRow == -2 && difCol == 0 && initRow == 6 && Destination.tag == EMPTY)
            {
                // Check square in between for 2-square move
                if (Board[initRow - 1][initCol].piece != '_')
                {
                    return 0;
                }
                return 1;
            }
            if (difRow == -1 && abs(difCol) == 1 && Destination.tag != EMPTY)
            {
                return 1;
            }
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

int makeMove(Coord_t moveFrom, Coord_t moveTo) //, FILE *fileText
{

    printf("%d %d : %d %d\n", moveFrom.x, moveFrom.y, moveTo.x, moveTo.y);

    if (validateMove(moveFrom.y, moveFrom.x, moveTo.y, moveTo.x))
    {
        // printf("%c", Board[moveFrom.x][moveFrom.y].piece);
        Board[moveTo.x][moveTo.y] = Board[moveFrom.x][moveFrom.y]; // afcem trecerea catre destiantie
        Board[moveFrom.x][moveFrom.y].piece = '_';
        Board[moveFrom.x][moveFrom.y].tag = EMPTY;
        Board[moveFrom.x][moveFrom.y].value = 0;
        Board[moveFrom.x][moveFrom.y].color = 0;
        whosTurn = !whosTurn;
    }
    else
    {
        printf("Move is invalid\n");
    }
    return 1;
}

void intialiseChessBoard()
{
    char array[10] = "rnbqkbnr";
    int values[8] = {5, 3, 3, 9, 0, 3, 3, 5}; // fofr each piece
    int tag[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    for (int i = 0; i < 8; i++)
    { // initializare prima linie
        Board[0][i].piece = array[i];
        Board[0][i].value = values[i];
        Board[0][i].color = 1;
        Board[0][i].tag = tag[i];
        // a doua linie sunt doar pionii
        Board[1][i].piece = 'p';
        Board[1][i].value = 1;
        Board[1][i].tag = 1;
        Board[1][i].color = 1;
    }

    for (int i = 2; i < 6; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Board[i][j].piece = '_';
            Board[i][j].tag = EMPTY;
        }
    } // pozitiile libere de pe tabla

    for (int i = 0; i < 8; i++)
    { // initializare pionii albi
        Board[6][i].piece = 'P';
        Board[6][i].value = 1;
        Board[6][i].color = 0;
        Board[6][i].tag = 1;

        Board[7][i].color = 0; // initializare ultima linie
        Board[7][i].piece = array[i] - ('a' - 'A');
        Board[7][i].value = values[i];
        Board[7][i].tag = tag[i];
    }
}

void render_piece(SDL_Renderer *renderer, SDL_Texture *piece, int col, int row)
{
    int img_width, img_height;
    SDL_QueryTexture(piece, NULL, NULL, &img_width, &img_height);
    SDL_Rect dstrect = {
        100 + SQUARE_SIZE * col + ((SQUARE_SIZE - img_width) / 2),  // x position (centered)
        100 + SQUARE_SIZE * row + ((SQUARE_SIZE - img_height) / 2), // y position (centered)
        img_width,                                                  // width
        img_height                                                  // height
    };
    SDL_RenderCopy(renderer, piece, NULL, &dstrect);
}

// SDL_Rect boardToScreen(int x, int y)
// {
//     return (SDL_Rect){
//         100 + x * SQUARE_SIZE,
//         100 + y * SQUARE_SIZE,
//         SQUARE_SIZE,
//         SQUARE_SIZE};
// }

// SDL_Point screenToBoard(int x_coord, int y_coord)
// {
//     return (SDL_Point){
//         (x_coord - 100) / SQUARE_SIZE,
//         (y_coord - 100) / SQUARE_SIZE};
// }

// Convert mouse input into chess table coordinates
Coord_t handleMouseClick(SDL_MouseButtonEvent *click)
{
    Coord_t coord;
    coord.y = (click->x - 100) / SQUARE_SIZE;
    coord.x = (click->y - 100) / SQUARE_SIZE;
    return coord;
}

int main(int argc, char *argv[])
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    // 1. Initialize SDL
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

    SDL_Texture *blackKnight = IMG_LoadTexture(renderer, "./assets/knight1.png");
    SDL_Texture *blackPawn = IMG_LoadTexture(renderer, "./assets/pawn1.png");
    SDL_Texture *blackBishop = IMG_LoadTexture(renderer, "./assets/bishop1.png");
    SDL_Texture *blackRook = IMG_LoadTexture(renderer, "./assets/rook1.png");
    SDL_Texture *blackQueen = IMG_LoadTexture(renderer, "./assets/queen1.png");
    SDL_Texture *blackKing = IMG_LoadTexture(renderer, "./assets/king1.png");

    SDL_Texture *whiteKnight = IMG_LoadTexture(renderer, "./assets/knight.png");
    SDL_Texture *whitePawn = IMG_LoadTexture(renderer, "./assets/pawn.png");
    SDL_Texture *whiteBishop = IMG_LoadTexture(renderer, "./assets/bishop.png");
    SDL_Texture *whiteRook = IMG_LoadTexture(renderer, "./assets/rook.png");
    SDL_Texture *whiteQueen = IMG_LoadTexture(renderer, "./assets/queen.png");
    SDL_Texture *whiteKing = IMG_LoadTexture(renderer, "./assets/king.png");

    if (!blackKnight)
    {
        printf("Failed to load image: %s\n", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    intialiseChessBoard();

    void showChessTable()
    {
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (Board[i][j].piece == 'p')
                    render_piece(renderer, blackPawn, j, i);
                else if (Board[i][j].piece == 'P')
                    render_piece(renderer, whitePawn, j, i);
                else if (Board[i][j].piece == 'b')
                    render_piece(renderer, blackBishop, j, i);
                else if (Board[i][j].piece == 'n')
                    render_piece(renderer, blackKnight, j, i);
                else if (Board[i][j].piece == 'r')
                    render_piece(renderer, blackRook, j, i);
                else if (Board[i][j].piece == 'B')
                    render_piece(renderer, whiteBishop, j, i);
                else if (Board[i][j].piece == 'N')
                    render_piece(renderer, whiteKnight, j, i);
                else if (Board[i][j].piece == 'R')
                    render_piece(renderer, whiteRook, j, i);
                else if (Board[i][j].piece == 'q')
                    render_piece(renderer, blackQueen, j, i);
                else if (Board[i][j].piece == 'k')
                    render_piece(renderer, blackKing, j, i);
                else if (Board[i][j].piece == 'Q')
                    render_piece(renderer, whiteQueen, j, i);
                else if (Board[i][j].piece == 'K')
                    render_piece(renderer, whiteKing, j, i);
            }
        }
    }

    Coord_t From;
    Coord_t To;
    // 4. Main loop
    int running = 1;
    int move = 0;
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
                Coord_t Aux = handleMouseClick(&event.button);
                if (move % 2 == 0)
                {
                    move++;
                    From = Aux;
                }
                else
                {
                    move++;
                    To = Aux;
                    makeMove(From, To);
                    printf("%d %d : %d %d\n", From.x, From.y, Aux.x, Aux.y);
                }

                // printf("%d", move);
                whosTurn = move % 2;
            }
        }

        // Clear the screen to black
        SDL_SetRenderDrawColor(renderer, 48, 0, 72, 1);
        SDL_RenderClear(renderer);

        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                SDL_Rect square = {col * SQUARE_SIZE + 100, row * SQUARE_SIZE + 100, SQUARE_SIZE, SQUARE_SIZE};

                // Alternate colors
                if ((row + col) % 2 == 0)
                {
                    SDL_SetRenderDrawColor(renderer, 240, 209, 250, 0.8); // White
                }
                else
                {
                    SDL_SetRenderDrawColor(renderer, 185, 50, 230, 0.8); // Dark gray
                }
                SDL_RenderFillRect(renderer, &square);
            }
        }

        showChessTable();

        // Update the screen
        SDL_RenderPresent(renderer);
    }

    // 5. Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
#endif
    return 0;
}