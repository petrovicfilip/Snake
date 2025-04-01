#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <time.h>

#define BLOCK_SIZE 20
#define ROWS 21
#define COLS 31

typedef struct
{
    Uint32 value;
} Cookie;

typedef struct SnakePart SnakePart;
struct SnakePart
{
    SnakePart* next_part;
    SDL_Color* color;
    Uint32 i; // vrsta, y koordinata / BLOCK_SIZE
    Uint32 j; // kolona, x koordinata / BLOCK_SIZE
}; 

typedef struct 
{
    Cookie* cookie;
    SDL_FRect* rect;
    SnakePart* snake_part_on_me; //ce mi treba najverovatnije...

} GridBlock;

void drawBlock(SDL_Renderer* renderer, GridBlock* block)
{
    //#9BBA5A
    if(block->snake_part_on_me == NULL)
    {
        SDL_SetRenderDrawColor(renderer, 0x9b, 0xba, 0x5A, 0xFF);
        SDL_RenderFillRect(renderer, block->rect);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0xFF);
        SDL_RenderFillRect(renderer, block->rect); 
    }
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderRect(renderer, block->rect);
}

GridBlock* allocBlock(int i, int j)
{
    GridBlock* block = (GridBlock*)malloc(sizeof(GridBlock));
    block->cookie = NULL;
    block->snake_part_on_me = NULL;
    block->rect = (SDL_FRect*)malloc(sizeof(SDL_FRect));
    block->rect->h = BLOCK_SIZE;
    block->rect->w = BLOCK_SIZE;
    block->rect->x = j * BLOCK_SIZE;
    block->rect->y = i * BLOCK_SIZE;
    return block;
}

SnakePart* makeSnake(GridBlock*** grid)
{
    SnakePart* head = (SnakePart*)malloc(sizeof(SnakePart));
    SnakePart* body_p1 = (SnakePart*)malloc(sizeof(SnakePart));
    SnakePart* body_p2 = (SnakePart*)malloc(sizeof(SnakePart));
    head->next_part = body_p1;
    body_p1->next_part = body_p2;
    body_p2->next_part = NULL;
    head->i = 3;
    body_p1->i = 2;
    body_p2->i = 1;
    head->j = body_p1->j = body_p2->j = COLS / 2;

    grid[3][COLS / 2]->snake_part_on_me = head;
    grid[2][COLS / 2]->snake_part_on_me = body_p1;
    grid[1][COLS / 2]->snake_part_on_me = body_p2;
    return head;
}

GridBlock*** allocGrid()
{
    GridBlock*** grid = (GridBlock***)malloc(sizeof(GridBlock**) * ROWS);
    for (int i = 0; i < ROWS; i++)
        grid[i] = (GridBlock**)malloc(sizeof(GridBlock*) * COLS);

    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            grid[i][j] = allocBlock(i , j);
    return grid;
}

void drawGrid(SDL_Renderer* renderer, GridBlock*** grid)
{
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            drawBlock(renderer, grid[i][j]); 
    SDL_FRect rect;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderRect(renderer, &rect);    
}

bool updateSnakePosition(SnakePart* head, GridBlock*** grid, Uint32 input)
{
    SnakePart* current = head;
    bool killed = false;
    int i, j = 0;
    switch (input) 
        {
            case SDLK_UP:
                i = head->i - 1;
                j = head->j;
                break;
            case SDLK_DOWN:
                i = head->i + 1;
                j = head->j;
                break;
            case SDLK_LEFT:
                i = head->i;
                j = head->j - 1;
                break;
            case SDLK_RIGHT:
                i = head->i;
                j = head->j + 1;
            default:
                printf("Nepoznata opcija!\n");
        }
    Uint32 next_new_i, next_new_j = 0;
    while (current != NULL)
    {
        if (i >= 0 && i < ROWS && j >= 0 && j < COLS)
        {
            grid[current->i][current->j]->snake_part_on_me = NULL;
            grid[i][j]->snake_part_on_me = current;
        }
        else
            killed = true;

        next_new_i = current->i;
        next_new_j = current->j;

        current->i = i;
        current->j = j;

        i = next_new_i;
        j = next_new_j;

        current = current->next_part;
    }
    return killed;
}

int main(int argc, char** argv)
{
    srand(time(NULL));
    if(!SDL_Init(SDL_INIT_VIDEO))
        printf("Neuspesna inicijalizacija SDL video moda.");

    SDL_Window* window = SDL_CreateWindow("Snake game", BLOCK_SIZE * COLS, BLOCK_SIZE * ROWS, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowResizable(window, false);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Event event;
    printf("ulazim  u allocgrid");
    GridBlock*** grid = allocGrid();
    SnakePart* snake_head = makeSnake(grid);
    SnakePart* snake_tail = snake_head->next_part->next_part;
    Uint32 last_arrow_input = SDLK_DOWN;
    // int x, y;
    // SDL_GetWindowPosition(window, &x, &y);
    // printf("%d, %d", x, y);
    // int top, left, bottom, right, abs_x, abs_y;
    //if (SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right) == 0)
    Uint32 fps = 10;
    Uint32 wanted_frame_time = 1000 / fps;
    Uint32 beg_of_frame;
    Uint32 frame_time;
    bool running = true;
    bool killed = false;
    while(running && !killed)
    {
        beg_of_frame = SDL_GetTicks();
        while(SDL_PollEvent(&event))
        {
            if ((event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) || event.type == SDL_EVENT_QUIT)
                running = false;
            switch (event.key.key) 
            {
                case SDLK_UP:
                    last_arrow_input = SDLK_UP;
                    break;
                case SDLK_DOWN:
                    last_arrow_input = SDLK_DOWN;
                    break;
                case SDLK_LEFT:
                    last_arrow_input = SDLK_LEFT;
                    break;
                case SDLK_RIGHT:
                    last_arrow_input = SDLK_RIGHT; 
                default:
                    printf("Nepoznata opcija!\n");
            }
        }
        killed = updateSnakePosition(snake_head, grid, last_arrow_input);
        drawGrid(renderer, grid);
        
        frame_time = SDL_GetTicks() - beg_of_frame; 
        if(frame_time < wanted_frame_time)
            SDL_Delay(wanted_frame_time - frame_time);

        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);

    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}