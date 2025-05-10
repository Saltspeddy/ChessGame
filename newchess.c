#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE 8

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

} Board[SIZE][SIZE];

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

// mutare speciala
void promotePawn(int initCol, int initRow)
{
    struct cheessBoard Piece = Board[initRow][initCol];
    printf("Choose what piece to promote to (eg. q/Q):");
    char p;
    short int rightColor = 0;
    while (!rightColor)
    {
        scanf("%c", &p);
        if (p == 'p' || p == 'P')
        {
            printf("You cannot promote to a pawn\n");
            rightColor = 0;
        }
        else if (Piece.color == 0 && isupper(p))
        {
            rightColor = 1;
        }
        else if (Piece.color == 1 && islower(p))
        {
            rightColor = 1;
        }
        else
        {
            printf("You cannot choose to promote to a different color\n");
            rightColor = 0;
        }
    }

    switch (p)
    {
    case 'n':
        Board[initRow][initCol].piece = 'n';
        Board[initRow][initCol].color = 1;
        Board[initRow][initCol].tag = 2;
        Board[initRow][initCol].value = 3;
        break;
    case 'N':
        Board[initRow][initCol].piece = 'N';
        Board[initRow][initCol].color = 0;
        Board[initRow][initCol].tag = 2;
        Board[initRow][initCol].value = 3;
        break;
    case 'b':
        Board[initRow][initCol].piece = 'b';
        Board[initRow][initCol].color = 1;
        Board[initRow][initCol].tag = 3;
        Board[initRow][initCol].value = 3;
        break;
    case 'B':
        Board[initRow][initCol].piece = 'B';
        Board[initRow][initCol].color = 0;
        Board[initRow][initCol].tag = 3;
        Board[initRow][initCol].value = 3;
        break;
    case 'r':
        Board[initRow][initCol].piece = 'r';
        Board[initRow][initCol].color = 1;
        Board[initRow][initCol].tag = 4;
        Board[initRow][initCol].value = 5;
        break;
    case 'R':
        Board[initRow][initCol].piece = 'R';
        Board[initRow][initCol].color = 0;
        Board[initRow][initCol].tag = 4;
        Board[initRow][initCol].value = 5;
        break;
    case 'q':
        Board[initRow][initCol].piece = 'q';
        Board[initRow][initCol].color = 1;
        Board[initRow][initCol].tag = 5;
        Board[initRow][initCol].value = 9;
        break;
    case 'Q':
        Board[initRow][initCol].piece = 'Q';
        Board[initRow][initCol].color = 0;
        Board[initRow][initCol].tag = 5;
        Board[initRow][initCol].value = 9;
        break;
    }
}

// void isCheckmate( char * moveTo){
//     int initCol =moveTo[0] - 'a';
//     int initRow =7 - (moveTo[1] - '1');
//     for( int i = 0 ; i < 8 ; i++){
//         for( int j = 0 ; j < 8 ; j++){
//             if( toupper(Board[i][j].piece) == 'K' && Board[i][j].color != Board[initRow][initCol].color){
//                 if(Board[i][j].color == 0){
//                     printf("White in CHECK!!!");
//                 }
//                 else{
//                     printf("Black in CHECK!!!");
//                 }
//             }
//         }
//     }
// }

short int whosTurn = 0;

int validateMove(int initCol, int initRow, int destCol, int destRow)
{
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

    if (whosTurn != Piece.color)
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
                promotePawn(initCol, initRow);
                return 1;
            }
            if (initRow == 6 && difRow == 1 && abs(difCol) == 1 && Destination.tag != EMPTY)
            {
                promotePawn(initCol, initRow);
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
                promotePawn(initCol, initRow);
                return 1;
            }
            if (initRow == 1 && difRow == -1 && abs(difCol) == 1 && Destination.tag != EMPTY)
            {
                promotePawn(initCol, initRow);
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

void isCheck(char *moveTo)
{
    int initCol = moveTo[0] - 'a';
    int initRow = 7 - (moveTo[1] - '1');
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (toupper(Board[i][j].piece) == 'K' && Board[i][j].color != Board[initRow][initCol].color)
            {
                if (validateMove(initCol, initRow, i, j))
                {
                    if (Board[i][j].color == 0)
                        printf("White in CHECK!!!");
                    else
                        printf("Black in CHECK!!!");
                }
            }
        }
    }
}

int calculateTotalPoints(int color)
{
    int sum = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (Board[i][j].color == color)
            {
                sum += Board[i][j].value;
            }
        }
    }
    return sum;
}

void generateLegalMoves(char *moveFrom)
{
    int initCol = moveFrom[0] - 'a';
    int initRow = 7 - (moveFrom[1] - '1');
    printf("%d %d\n", initRow, initCol);
    printf("\n   ");
    for (int i = 0; i < 8; i++)
    {
        printf("%c ", 'a' + i);
    }
    printf("\n");
    for (int i = 0; i < 8; i++)
    {
        printf("%d  ", 8 - i);
        for (int j = 0; j < 8; j++)
        {
            // int boolean = validateMove(initCol,initRow, j, i);
            // printf("%d ", boolean);
            if (validateMove(initCol, initRow, j, i))
            {
                printf("\033[95m%c\033[0m ", Board[i][j].piece);
            }
            else
            {
                printf("%c ", Board[i][j].piece);
            }
        }
        printf("\n");
    }
}

void showChessBoard()
{
    printf("\n   ");
    for (int i = 0; i < 8; i++)
    {
        printf("%c ", 'a' + i);
    }
    printf("\n");
    for (int i = 0; i < 8; i++)
    {
        printf("%d  ", 8 - i);
        for (int j = 0; j < 8; j++)
        {
            printf("%c ", Board[i][j].piece);
        }
        printf("\n");
    }
}

int makeMove(char *moveFrom, char *moveTo, FILE *fileText)
{
    if (strlen(moveFrom) < 2 || strlen(moveTo) < 2)
    {
    }
    int initCol = moveFrom[0] - 'a';
    int initRow = 7 - (moveFrom[1] - '1');
    int destCol = moveTo[0] - 'a';
    int destRow = 7 - (moveTo[1] - '1');
    if (validateMove(initCol, initRow, destCol, destRow))
    {
        fwrite(moveFrom, sizeof(char), strlen(moveFrom), fileText); // salvam toate mutarile validate pe parcurs
        fwrite("\t", sizeof(char), 1, fileText);
        fwrite(moveTo, sizeof(char), strlen(moveTo), fileText);
        fwrite("\n", sizeof(char), 1, fileText);
        printf("%c", Board[initRow][initCol].piece);
        Board[destRow][destCol] = Board[initRow][initCol]; // afcem trecerea catre destiantie
        Board[initRow][initCol].piece = '_';
        Board[initRow][initCol].tag = EMPTY;
        Board[initRow][initCol].value = 0;
        Board[initRow][initCol].color = 0;
        whosTurn = !whosTurn;
    }
    else
    {
        printf("Move is invalid\n");
    }
    return 1;
}

void saveBoardState(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        printf("Error opening file for saving board state");
    }

    // Save whose turn it is (0 = White, 1 = Black)
    fprintf(file, "%d\n", whosTurn);

    // Save the board state
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            if (Board[i][j].tag == EMPTY)
            {
                fprintf(file, "_");
            }
            else
            {
                fprintf(file, "%c", Board[i][j].piece); // Convert type to letter
            }
        }
        fprintf(file, "\n"); // Newline after each row
    }

    fclose(file);
    printf("Board state saved to %s\n", filename);
}

// the menu for the game
void showMenuGame()
{
    printf("______________\n");
    printf("    Menu\n");
    printf("1.Start game.\n");
    printf("2.Points for each player at given moment.\n");
    printf("3.Exit menu.\n");
    printf("______________\n");
}

void trimNewline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

// Basic move format validation
int isValidMoveFormat(const char *move)
{
    if (strlen(move) != 2)
        return 0;
    return (move[0] >= 'a' && move[0] <= 'h') &&
           (move[1] >= '1' && move[1] <= '8');
}

void loadBoard()
{
    FILE *file = fopen("saveBoard.txt", "r"); // use "r" to read
    if (file == NULL)
    {
        perror("Error opening moves file");
        return;
    }
    char line[16]; // big enough for one line (incl. \n and \0)
    fgets(line, sizeof(line), file);
    whosTurn = line[0] - '0';

    for (int i = 0; i < 8; i++) // assuming 8 lines for an 8x8 board
    {
        if (fgets(line, sizeof(line), file) != NULL)
        {
            for (int j = 0; j < 8; j++)
            {
                Board[i][j].piece = line[j];
                if (isupper(line[j]))
                {
                    Board[i][j].color = 0;
                }
                else
                {
                    Board[i][j].color = 1;
                }
                switch (toupper(line[j]))
                {
                case 'P':
                    Board[i][j].value = 1;
                    Board[i][j].tag = PAWN;
                    break;
                case 'B':
                    Board[i][j].value = 3;
                    Board[i][j].tag = BISHOP;
                    break;
                case 'N':
                    Board[i][j].value = 3;
                    Board[i][j].tag = KNIGHT;
                    break;
                case 'R':
                    Board[i][j].value = 5;
                    Board[i][j].tag = ROOK;
                    break;
                case 'K':
                    Board[i][j].value = 0;
                    Board[i][j].tag = KING;
                    break;
                case 'Q':
                    Board[i][j].value = 9;
                    Board[i][j].tag = QUEEN;
                    break;
                default:
                    Board[i][j].tag = EMPTY;
                    break;
                }
            }
        }
        else
        {
            printf("Error reading line %d\n", i + 1);
            break;
        }
    }

    fclose(file);
    showChessBoard(); // assuming this displays the loaded board
}

int main()
{
    printf("Do you want to load a custom board?\n[y/n]: ");
    char answer = ' ';
    scanf("%c", &answer);
    if (answer == 'n')
    {
        intialiseChessBoard();
    }
    else
    {
        loadBoard();
    }
    FILE *file = fopen("moves.txt", "w");
    if (file == NULL)
    {
        perror("Error opening moves file");
        return 1;
    }
    short int inputChoice = 0;

    do
    {
        showMenuGame();
        if (scanf("%hd", &inputChoice) != 1)
        {
            printf("Invalid input!\n");
            while (getchar() != '\n')
                ; // Clear invalid input
            continue;
        }
        while (getchar() != '\n')
            ; // Clear the newline
        switch (inputChoice)
        {
        case 1:
        {
            char moveFrom[10], moveTo[10];
            showChessBoard();

            while (1)
            {
                printf("Please enter starting position or q(quit) to quit the gameplay or s(save) to save the board:\n");

                printf("Move from: ");
                if (fgets(moveFrom, sizeof(moveFrom), stdin) == NULL)
                    break;
                trimNewline(moveFrom);

                if (strcmp(moveFrom, "q") == 0)
                    break;
                if (strcmp(moveFrom, "s") == 0)
                {
                    saveBoardState("saveBoard.txt");
                    // printf("Game saved to saveBoard.txt\n");  // Add confirmation
                    // continue;
                }
                generateLegalMoves(moveFrom);

                printf("Move to: ");
                if (fgets(moveTo, sizeof(moveTo), stdin) == NULL)
                    break;
                trimNewline(moveTo);
                isCheck(moveTo);
                if (strcmp(moveTo, "q") == 0)
                    break;
                if (strcmp(moveTo, "s") == 0)
                {
                    saveBoardState("saveBoard.txt");
                    // printf("Game saved to saveBoard.txt\n");  // Add confirmation
                    // continue;
                }

                if (!isValidMoveFormat(moveFrom) || !isValidMoveFormat(moveTo))
                {
                    printf("Invalid format! Use like 'a2'\n");
                    continue;
                }

                makeMove(moveFrom, moveTo, file);
                showChessBoard();
            }
            break;
        }
        case 2:
            printf("\nScores:\n");
            printf("Black: %d points\n", calculateTotalPoints(1));
            printf("White: %d points\n", calculateTotalPoints(0));
            // saveBoardState("saveBoard.txt");
            break;
        case 3:
            printf("Exiting menu\n");
            break;
        default:
            printf("Invalid choice! Try again\n");
            break;
        }
    } while (inputChoice != 3);

    fclose(file);
    return 0;
}
