//  #include <SDL.h> // If headers are directly in include/
#include <SDL2/SDL.h>       // Correct if files are in include/SDL2/ // or #include "SDL.h" depending on setup
#include <SDL2/SDL_image.h> // Required for PNG/JPG
// #include <SDL2/SDL_ttf.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define BOARD_SIZE 8
#define SQUARE_SIZE (500 / BOARD_SIZE)

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

int main(int argc, char *argv[])
{
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

    // Get image dimensions

    // 4. Main loop
    int running = 1;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
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
        render_piece(renderer, blackRook, 0, 0);
        render_piece(renderer, blackKnight, 1, 0);
        render_piece(renderer, blackBishop, 2, 0);
        render_piece(renderer, blackQueen, 3, 0);
        render_piece(renderer, blackKing, 4, 0);
        render_piece(renderer, blackBishop, 5, 0);
        render_piece(renderer, blackKnight, 6, 0);
        render_piece(renderer, blackRook, 7, 0);
        for (int i = 0; i < 8; i++)
        {
            render_piece(renderer, blackPawn, i, 1);
            render_piece(renderer, whitePawn, i, 6);
        }

        render_piece(renderer, whiteRook, 0, 7);
        render_piece(renderer, whiteKnight, 1, 7);
        render_piece(renderer, whiteBishop, 2, 7);
        render_piece(renderer, whiteQueen, 3, 7);
        render_piece(renderer, whiteKing, 4, 7);
        render_piece(renderer, whiteBishop, 5, 7);
        render_piece(renderer, whiteKnight, 6, 7);
        render_piece(renderer, whiteRook, 7, 7);

        // Update the screen
        SDL_RenderPresent(renderer);
    }

    // 5. Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}