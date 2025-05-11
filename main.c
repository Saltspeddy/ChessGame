#ifdef _WIN32
#include <windows.h>
#include <SDL2/SDL.h>       // Correct if files are in include/SDL2/ // or #include "SDL.h" depending on setup
#include <SDL2/SDL_image.h> // Required for PNG/JPG
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 750
#define WINDOW_HEIGHT 600
#define BOARD_SIZE 8
#define SQUARE_SIZE (400 / BOARD_SIZE)

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
        80 + SQUARE_SIZE * col + ((SQUARE_SIZE - img_width) / 2),  // x position (centered)
        80 + SQUARE_SIZE * row + ((SQUARE_SIZE - img_height) / 2), // y position (centered)
        img_width,                                                 // width
        img_height                                                 // height
    };
    SDL_RenderCopy(renderer, piece, NULL, &dstrect);
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

void start_game()
{
    printf("Start button clicked\n");
    // TODO: Add game starting logic here
}

void save_game()
{
    printf("Save button clicked\n");
    // TODO: Add save functionality here
}

void reload_game()
{
    printf("Reload button clicked\n");
    // TODO: Add reload logic here
}

void open_progress_window()
{
    // SDL_Window *progressWindow = SDL_CreateWindow("Progress",
    //                                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 300, 300, SDL_WINDOW_SHOWN);
    // if (!progressWindow)
    // {
    //     SDL_Log("Failed to create progress window: %s", SDL_GetError());
    //     return;
    // }

    // SDL_Renderer *progressRenderer = SDL_CreateRenderer(progressWindow, -1, SDL_RENDERER_ACCELERATED);
    // if (!progressRenderer)
    // {
    //     SDL_DestroyWindow(progressWindow);
    //     SDL_Log("Failed to create progress renderer: %s", SDL_GetError());
    //     return;
    // }

    // int running = 1;
    // SDL_Event e;
    // while (running)
    // {
    //     while (SDL_PollEvent(&e))
    //     {
    //         if (e.type == SDL_QUIT)
    //         {
    //             running = 0; // just quit this window's loop
    //         }
    //     }

    //     SDL_SetRenderDrawColor(progressRenderer, 200, 200, 255, 255);
    //     SDL_RenderClear(progressRenderer);
    //     SDL_RenderPresent(progressRenderer);
    // }

    // SDL_DestroyRenderer(progressRenderer);
    // SDL_DestroyWindow(progressWindow);
    printf("Progress\n");
    // ⚠️ DO NOT call SDL_Quit() or TTF_Quit() here!
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

    //
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

    SDL_Color White = {255, 255, 255, 255};
    SDL_Color Grey = {0, 0, 0, 100};
    SDL_Color Pink = {240, 209, 250, 255}; // bright pink, outilnes text from menu
    SDL_Color DarkPink = {185, 50, 230, 255};

    SDL_Surface *textSurface5 = TTF_RenderText_Blended(largeFont, "PixelChess", Grey);
    SDL_Texture *textTexture5 = SDL_CreateTextureFromSurface(renderer, textSurface5);
    SDL_Rect textRect5 = {52, 17, textSurface5->w, textSurface5->h};
    SDL_FreeSurface(textSurface5); // shadow for title

    SDL_Surface *textSurface1 = TTF_RenderText_Blended(largeFont, "PixelChess", White);
    SDL_Texture *textTexture1 = SDL_CreateTextureFromSurface(renderer, textSurface1);
    SDL_Rect textRect1 = {60, 10, textSurface1->w, textSurface1->h};
    SDL_FreeSurface(textSurface1);

    SDL_Surface *textSurface2 = TTF_RenderText_Blended(smallFont, " a    b    c    d     e    f    g    h", White);
    SDL_Texture *textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
    SDL_Rect textRect2 = {92, 480, textSurface2->w, textSurface2->h};
    SDL_FreeSurface(textSurface2);

    SDL_Surface *textSurface3 = TTF_RenderText_Blended(largeFont, "Menu", Grey);
    SDL_Texture *textTexture3 = SDL_CreateTextureFromSurface(renderer, textSurface3);
    SDL_Rect textRect3 = {548, 37, textSurface3->w, textSurface3->h};
    SDL_FreeSurface(textSurface3); // shadow for menu

    SDL_Surface *textSurface4 = TTF_RenderText_Blended(largeFont, "Menu", White);
    SDL_Texture *textTexture4 = SDL_CreateTextureFromSurface(renderer, textSurface4);
    SDL_Rect textRect4 = {555, 30, textSurface4->w, textSurface4->h};
    SDL_FreeSurface(textSurface4);

    SDL_Surface *textSurfaceStart = TTF_RenderText_Blended(mediumFont, "Start", DarkPink);
    SDL_Texture *textTextureStart = SDL_CreateTextureFromSurface(renderer, textSurfaceStart);
    SDL_Rect textRectStart = {572, 130, textSurfaceStart->w, textSurfaceStart->h};
    SDL_FreeSurface(textSurfaceStart);

    SDL_Surface *textSurfaceSave = TTF_RenderText_Blended(mediumFont, "Save", DarkPink);
    SDL_Texture *textTextureSave = SDL_CreateTextureFromSurface(renderer, textSurfaceSave);
    SDL_Rect textRectSave = {572, 210, textSurfaceSave->w, textSurfaceSave->h};
    SDL_FreeSurface(textSurfaceSave);

    SDL_Surface *textSurfaceReload = TTF_RenderText_Blended(mediumFont, "Reload", DarkPink);
    SDL_Texture *textTextureReload = SDL_CreateTextureFromSurface(renderer, textSurfaceReload);
    SDL_Rect textRectReload = {563, 290, textSurfaceReload->w, textSurfaceReload->h};
    SDL_FreeSurface(textSurfaceReload);

    SDL_Surface *textSurfaceProgress = TTF_RenderText_Blended(mediumFont, "Progress", DarkPink);
    SDL_Texture *textTextureProgress = SDL_CreateTextureFromSurface(renderer, textSurfaceProgress);
    SDL_Rect textRectProgress = {546, 370, textSurfaceProgress->w, textSurfaceProgress->h};
    SDL_FreeSurface(textSurfaceProgress);

    SDL_Surface *textSurfaceStart1 = TTF_RenderText_Blended(mediumFont, "Start", Pink);
    SDL_Texture *textTextureStart1 = SDL_CreateTextureFromSurface(renderer, textSurfaceStart1);
    SDL_Rect textRectStart1 = {577, 128, textSurfaceStart1->w, textSurfaceStart1->h};
    SDL_FreeSurface(textSurfaceStart1);

    SDL_Surface *textSurfaceSave1 = TTF_RenderText_Blended(mediumFont, "Save", Pink);
    SDL_Texture *textTextureSave1 = SDL_CreateTextureFromSurface(renderer, textSurfaceSave1);
    SDL_Rect textRectSave1 = {577, 208, textSurfaceSave1->w, textSurfaceSave1->h};
    SDL_FreeSurface(textSurfaceSave1);

    SDL_Surface *textSurfaceReload1 = TTF_RenderText_Blended(mediumFont, "Reload", Pink);
    SDL_Texture *textTextureReload1 = SDL_CreateTextureFromSurface(renderer, textSurfaceReload1);
    SDL_Rect textRectReload1 = {568, 288, textSurfaceReload1->w, textSurfaceReload1->h};
    SDL_FreeSurface(textSurfaceReload1);

    SDL_Surface *textSurfaceProgress1 = TTF_RenderText_Blended(mediumFont, "Progress", Pink);
    SDL_Texture *textTextureProgress1 = SDL_CreateTextureFromSurface(renderer, textSurfaceProgress1);
    SDL_Rect textRectProgress1 = {551, 368, textSurfaceProgress1->w, textSurfaceProgress1->h};
    SDL_FreeSurface(textSurfaceProgress1);

    const char *multilineText = "1\n2\n3\n4\n5\n6\n7\n8";

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
                Coord_t Aux = handleMouseClick(&event.button);
                if (Aux.x > 7 || Aux.x < 0 || Aux.y > 7 || Aux.y < 0)
                {
                    int mx = event.button.x;
                    int my = event.button.y;

                    if (mx >= textRectStart.x && mx <= textRectStart.x + textRectStart.w &&
                        my >= textRectStart.y && my <= textRectStart.y + textRectStart.h)
                    {
                        start_game();
                        started = 1;
                    }
                    else if (mx >= textRectSave.x && mx <= textRectSave.x + textRectSave.w && my >= textRectSave.y && my <= textRectSave.y + textRectSave.h)
                    {
                        save_game();
                    }
                    else if (mx >= textRectReload.x && mx <= textRectReload.x + textRectReload.w && my >= textRectReload.y && my <= textRectReload.y + textRectReload.h)
                    {
                        reload_game();
                    }
                    else if (mx >= textRectProgress.x && mx <= textRectProgress.x + textRectProgress.w &&
                             my >= textRectProgress.y && my <= textRectProgress.y + textRectProgress.h)
                    {
                        open_progress_window();
                    }
                }
                else if (started == 1)
                {
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
        }

        // Clear the screen to black
        SDL_SetRenderDrawColor(renderer, 48, 0, 72, 1);
        SDL_RenderClear(renderer);

        // the offset
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // for transparency
        SDL_Rect extraRect = {50, 100, 410, 410};                  // x, y, width, height
        SDL_SetRenderDrawColor(renderer, 176, 75, 165, 150);
        SDL_RenderFillRect(renderer, &extraRect);

        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                SDL_Rect square = {col * SQUARE_SIZE + 80, row * SQUARE_SIZE + 80, SQUARE_SIZE, SQUARE_SIZE};

                // Alternate colors
                if ((row + col) % 2 == 0)
                {
                    SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255); // White
                }
                else
                {
                    SDL_SetRenderDrawColor(renderer, 185, 50, 230, 255); // for black //dark purple
                }
                SDL_RenderFillRect(renderer, &square);
            }
        }

        SDL_RenderCopy(renderer, textTexture5, NULL, &textRect5); // CONTEAZA ORDINEA
        SDL_RenderCopy(renderer, textTexture1, NULL, &textRect1);
        SDL_RenderCopy(renderer, textTexture2, NULL, &textRect2);
        SDL_RenderCopy(renderer, textTexture3, NULL, &textRect3);
        SDL_RenderCopy(renderer, textTexture4, NULL, &textRect4);
        SDL_RenderCopy(renderer, textTextureStart, NULL, &textRectStart);
        SDL_RenderCopy(renderer, textTextureSave, NULL, &textRectSave);
        SDL_RenderCopy(renderer, textTextureReload, NULL, &textRectReload);
        SDL_RenderCopy(renderer, textTextureProgress, NULL, &textRectProgress);
        SDL_RenderCopy(renderer, textTextureStart1, NULL, &textRectStart1);
        SDL_RenderCopy(renderer, textTextureSave1, NULL, &textRectSave1);
        SDL_RenderCopy(renderer, textTextureReload1, NULL, &textRectReload1);
        SDL_RenderCopy(renderer, textTextureProgress1, NULL, &textRectProgress1);
        render_multiline_text(renderer, smallFont, multilineText, White, 63, 107, 18);

        SDL_Rect outline1 = {534, 120, 160, 60};
        SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255); // Green
        SDL_RenderDrawRect(renderer, &outline1);
        SDL_Rect outline2 = {534, 200, 160, 60};
        SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255); // Green
        SDL_RenderDrawRect(renderer, &outline2);
        SDL_Rect outline3 = {534, 280, 160, 60};
        SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255); // Green
        SDL_RenderDrawRect(renderer, &outline3);
        SDL_Rect outline4 = {534, 360, 160, 60};
        SDL_SetRenderDrawColor(renderer, 240, 209, 250, 255); // Green
        SDL_RenderDrawRect(renderer, &outline4);

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