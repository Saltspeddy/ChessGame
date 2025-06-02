#ifdef _WIN32
#include <SDL2/SDL.h>       // Correct if files are in include/SDL2/ // or #include "SDL.h" depending on setup
#include <SDL2/SDL_image.h> // Required for PNG/JPG
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 850
#define WINDOW_HEIGHT 600
#define BOARD_SIZE 8
#define SQUARE_SIZE (400 / BOARD_SIZE)
#define MAX_MOVES 512

char moves[MAX_MOVES][10]; // Array to store moves for pgn file
int moveCount = 0;         // Number of moves for pgn file
int running = 1;

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
                            // 6 piese se reprezinta pe 3 biti
    char piece;

} Piece;
Piece Board[BOARD_SIZE][BOARD_SIZE];

typedef struct coordinates
{
    int x;
    int y;
} Coord_t;

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

int validateMove(int initCol, int initRow, int destCol, int destRow)
{
    printf("%c %d %d : %d %d %c\n\n", Board[initRow][initCol].piece, initRow, initCol, destRow, destCol, Board[destRow][destCol].piece);

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
            if (Board[currRow][currCol].tag != EMPTY)
            {
                return 0; // este blocat
            }
            currCol += stepCol;
            currRow += stepRow;
        }
        return 1; // este cale libera
    }

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
            // Single move forward
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
    Board[0][0] = Board[0][7] = (Piece){ROOK, 1, 1};
    Board[7][0] = Board[7][7] = (Piece){ROOK, 0, 1};
    // Knights
    Board[0][1] = Board[0][6] = (Piece){KNIGHT, 1, 1};
    Board[7][1] = Board[7][6] = (Piece){KNIGHT, 0, 1};
    // Bishops
    Board[0][2] = Board[0][5] = (Piece){BISHOP, 1, 1};
    Board[7][2] = Board[7][5] = (Piece){BISHOP, 0, 1};
    // Queens
    Board[0][3] = (Piece){QUEEN, 1, 1};
    Board[7][3] = (Piece){QUEEN, 0, 1};
    // Kings
    Board[0][4] = (Piece){KING, 1, 1};
    Board[7][4] = (Piece){KING, 0, 1};
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
        char *token = strtok(line, " ");
        while (token)
        {
            // Skip move numbers (e.g., "1.", "2.")
            if (strchr(token, '.'))
            {
                token = strtok(NULL, " ");
                continue;
            }

            // Store the move
            strncpy(moves[moveCount], token, sizeof(moves[moveCount]) - 1);
            moves[moveCount][sizeof(moves[moveCount]) - 1] = '\0';
            moveCount++;

            token = strtok(NULL, " ");
        }
    }

    fclose(file);
}

int pgn_to_coords(const char *move, Coord_t *from, Coord_t *to)
{
    // Handle castling
    if (strcmp(move, "O-O") == 0)
    {
        // Kingside castling
        if (whosTurn == 0)
        { // White
            *from = (Coord_t){7, 4};
            *to = (Coord_t){7, 6};
        }
        else
        { // Black
            *from = (Coord_t){0, 4};
            *to = (Coord_t){0, 6};
        }
        return 1;
    }
    else if (strcmp(move, "O-O-O") == 0)
    {
        // Queenside castling
        if (whosTurn == 0)
        { // White
            *from = (Coord_t){7, 4};
            *to = (Coord_t){7, 2};
        }
        else
        { // Black
            *from = (Coord_t){0, 4};
            *to = (Coord_t){0, 2};
        }
        return 1;
    }

    // Handle standard moves (e.g., e4, Nf3)
    int col = move[strlen(move) - 2] - 'a';       // Column (file)
    int row = 8 - (move[strlen(move) - 1] - '0'); // Row (rank)

    // Find the piece to move
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            Piece p = Board[r][c];
            if (p.tag != EMPTY && p.color == whosTurn)
            {
                // Check if the piece can move to the target square
                if (validateMove(c, r, col, row))
                {
                    *from = (Coord_t){r, c};
                    *to = (Coord_t){row, col};
                    return 1;
                }
            }
        }
    }

    return 0; // Move not found
}

void tutorial_mode(SDL_Renderer *renderer)
{
    printf("Tutorial mode started\n");

    parse_pgn("tutorial.pgn"); // Load the PGN file

    for (int i = 0; i < moveCount; i++)
    {
        Coord_t from, to;
        if (pgn_to_coords(moves[i], &from, &to))
        {
            makeMove(from, to);
            printf("Move %d: %s\n", i + 1, moves[i]);

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

            SDL_Delay(1000); // Pause for 1 second between moves
        }
        else
        {
            printf("Invalid move: %s\n", moves[i]);
        }
    }

    printf("Tutorial mode finished\n");
}

int ai_make_move()
{
    // Iterate over all Black pieces and find a valid move
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            Piece p = Board[row][col];
            if (p.tag != EMPTY && p.color == 1)
            { // AI controls Black pieces
                for (int destRow = 0; destRow < BOARD_SIZE; destRow++)
                {
                    for (int destCol = 0; destCol < BOARD_SIZE; destCol++)
                    {
                        if (validateMove(col, row, destCol, destRow))
                        {
                            Coord_t moveFrom = {row, col};
                            Coord_t moveTo = {destRow, destCol};
                            makeMove(moveFrom, moveTo);
                            printf("AI moved: (%d, %d) -> (%d, %d)\n", row, col, destRow, destCol);
                            return 1; // Move successful
                        }
                    }
                }
            }
        }
    }
    return 0; // No valid moves found
}
void trainer_mode(SDL_Renderer *renderer)
{
    printf("Trainer mode (Human vs AI) started\n");

    int move = 0;            // Keeps track of whose turn it is (0 = Human/White, 1 = AI/Black)
    Coord_t From = {-1, -1}; // Initialize with invalid coordinates
    Coord_t To = {-1, -1};   // Initialize with invalid coordinates

    while (running)
    {
        if (move % 2 == 0)
        { // Human's turn
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    running = 0;
                    break;
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN)
                {
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

        SDL_RenderPresent(renderer);
    }
}

Coord_t handleMouseClick(SDL_MouseButtonEvent *click);

void casual_mode(SDL_Renderer *renderer)
{
    printf("Casual mode (Human vs Human) started\n");

    int move = 0;            // Keeps track of whose turn it is (0 = White, 1 = Black)
    Coord_t From = {-1, -1}; // Initialize with invalid coordinates
    Coord_t To = {-1, -1};   // Initialize with invalid coordinates

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
                break;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                Coord_t clicked = handleMouseClick(&event.button);

                // Ensure the click is within the chessboard bounds
                if (clicked.x >= 0 && clicked.x < BOARD_SIZE && clicked.y >= 0 && clicked.y < BOARD_SIZE)
                {
                    if (From.x == -1 && From.y == -1)
                    { // Selecting a piece
                        Piece selectedPiece = Board[clicked.x][clicked.y];
                        if (selectedPiece.tag != EMPTY && selectedPiece.color == move % 2)
                        {
                            From = clicked; // Store the selected piece's coordinates
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
                    }
                }
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

        SDL_RenderPresent(renderer);
    }
}
void exit_game()
{
    printf("Exit button clicked\n");
    // SDL_Quit();
    // exit(0);
    running = 0; // Set running to 0 to exit the main loop
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
    SDL_Window *progressWindow = SDL_CreateWindow("Progress", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 300, 300, SDL_WINDOW_SHOWN);
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
        SDL_SetRenderDrawColor(progressRenderer, 200, 200, 255, 255);
        SDL_RenderClear(progressRenderer);

        // Add some text
        SDL_Color textColor = {0, 0, 0, 255};
        SDL_Surface *textSurface = TTF_RenderText_Blended(progressFont, "Game Progress", textColor);
        if (textSurface)
        {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(progressRenderer, textSurface);
            SDL_Rect textRect = {50, 50, textSurface->w, textSurface->h};
            SDL_RenderCopy(progressRenderer, textTexture, NULL, &textRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
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

    SDL_Color White = {255, 255, 255, 255};   // white
    SDL_Color Grey = {0, 0, 0, 100};          // shadow
    SDL_Color Pink = {240, 209, 250, 255};    // bright pink, outilnes text from menu
    SDL_Color DarkPink = {185, 50, 230, 255}; // dark pink, text from menu

    const char *multilineText = "1\n2\n3\n4\n5\n6\n7\n8";

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
                            reload_game();
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
