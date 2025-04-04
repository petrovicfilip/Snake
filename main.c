#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <time.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_ttf/SDL_textengine.h>

#define BLOCK_SIZE 30
#define ROWS 21
#define COLS 31
#define BASE_TIME_BONUS 15

int time_bonus = BASE_TIME_BONUS;

char highscore[] = "T4iZ69S-H1gh42oR38 ";
char key[] = "%%&#";

bool occupied_grid_blocks[ROWS * COLS];

typedef struct SnakePart SnakePart;
struct SnakePart
{
    SnakePart* next_part;
    Uint32 i; // vrsta, y koordinata / BLOCK_SIZE
    Uint32 j; // kolona, x koordinata / BLOCK_SIZE
    bool is_even;
}; 

typedef struct 
{
    bool fruit;
    SDL_FRect* rect;
    SnakePart* snake_part_on_me;

} GridBlock;

char* xor_cypher(char* text, char* key)
{
    int text_len = strlen(text);
    int key_len = strlen(key);
    char* new = (char*)malloc(text_len + 1);
    int k = key_len < text_len ? key_len : text_len;
    int i, j = 0;
    for (i = 0; i < text_len; i++, j = (j + 1) % k)
        new[i] = text[i] ^ key[j];
    new[i] = '\0';
    return new; 
}

void* makeOrOpenHighscoreFile()
{
    FILE* file = fopen("snake.bin", "r+b");
    if (file == NULL)
    {
        file = fopen("snake.bin", "w+b");
        if(file == NULL)
            return NULL;
        char* cypher = xor_cypher(highscore, key);
        fwrite(cypher, sizeof(char), 19, file);
        free(cypher);
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, file);
        fseek(file, 0, SEEK_SET);
        return file;
    }
    else
    {
        int len = strlen(highscore);
        char* label = (char*)malloc(len + 1);
        fread(label, sizeof(char), 19, file);
        label[19] = '\0';
        char* cypher = xor_cypher(label, key);
        if(strcmp(cypher, highscore) == 0)
        {
            fseek(file, 0, SEEK_SET);
            free(cypher);
            return file;
        }
        fclose(file);
        FILE* file = fopen("snake.bin", "w+b");
        if(file == NULL)
            return NULL;
        cypher = xor_cypher(highscore, key);
        fwrite(cypher, sizeof(char), 19, file);
        free(cypher);
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, file);
        fseek(file, 0, SEEK_SET);
        return file;
    }
}

int readHighscore(FILE* file)
{
    fseek(file, 19, SEEK_SET);
    int hs;
    fread(&hs, sizeof(int), 1, file);
    fseek(file, 0, SEEK_SET);
    return hs;
}

void updateHighscore(FILE* file, int new_hs)
{
    fseek(file, 19, SEEK_SET);
    //printf("Upisujem: %d\n", new_hs);
    int score;
    fwrite(&new_hs, sizeof(int), 1, file);
    uint32_t i, k;
    unsigned char j;
    srand(time(NULL));
    i = rand() % ((uint32_t)~0);
    j = rand() % ((unsigned char)~0);
    k = rand() % ((uint32_t)~0);
    fwrite(&i, sizeof(uint32_t), 1, file);
    fwrite(&j, sizeof(char), 1, file);
    fwrite(&k, sizeof(uint32_t), 1, file);
    fseek(file, 0, SEEK_SET);
}

void fillCircle(SDL_Renderer* renderer, int radius, int x_offset, int y_offset)
{
    for (int i = -radius; i < radius; i++)
        for (int j = -radius; j < radius; j++)
            if (i * i + j * j < radius * radius)
                SDL_RenderPoint(renderer, j + x_offset, i + y_offset);
}

void outlineCircle(SDL_Renderer* renderer, int radius, int x_offset, int y_offset)
{
    for (int i = -radius; i < radius; i++)
        for (int j = -radius; j < radius; j++)
        // pixeli su kvadratici pa ga malo tweakujemo, sta da se radi...
            if (i * i + j * j >= radius * radius - 25 && i * i + j * j <= radius * radius + 30)
                SDL_RenderPoint(renderer, j + x_offset, i + y_offset);
}

void drawBlock(SDL_Renderer* renderer, GridBlock* block)
{
    //#9BBA5A
    if(block->snake_part_on_me == NULL && !block->fruit)
    {
        SDL_SetRenderDrawColor(renderer, 0x9b, 0xba, 0x5A, 0xFF);
        SDL_RenderFillRect(renderer, block->rect);
    }
    else if(block->fruit)
    {
        SDL_SetRenderDrawColor(renderer, 0x9b, 0xba, 0x5A, 0xFF);
        SDL_RenderFillRect(renderer, block->rect);

        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 0xFF);
        fillCircle(renderer, BLOCK_SIZE / 2, block->rect->x + BLOCK_SIZE / 2, block->rect->y + BLOCK_SIZE / 2);
        SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
        outlineCircle(renderer, BLOCK_SIZE / 2, block->rect->x + BLOCK_SIZE / 2, block->rect->y + BLOCK_SIZE / 2);
    }
    else
    {
        if(block->snake_part_on_me->is_even)
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0xFF);
            SDL_RenderFillRect(renderer, block->rect);
        }
        // sarena zmijica...
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 127, 80, 0xFF);
            SDL_RenderFillRect(renderer, block->rect);
        }
    }
}

GridBlock* allocBlock(int i, int j)
{
    GridBlock* block = (GridBlock*)malloc(sizeof(GridBlock));
    block->fruit = false;
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
    head->is_even = true;
    body_p1->is_even = false;
    body_p2->is_even = true;
    head->next_part = body_p1;
    body_p1->next_part = body_p2;
    body_p2->next_part = NULL;
    head->i = 3;
    body_p1->i = 2;
    body_p2->i = 1;
    head->j = body_p1->j = body_p2->j = COLS / 2;
    occupied_grid_blocks[3 * COLS + COLS / 2] = occupied_grid_blocks[2 * COLS + COLS / 2] = occupied_grid_blocks[COLS + COLS / 2] = true; 

    grid[3][COLS / 2]->snake_part_on_me = head;
    grid[2][COLS / 2]->snake_part_on_me = body_p1;
    grid[1][COLS / 2]->snake_part_on_me = body_p2;
    return head;
}

void deleteSnake(SnakePart* head)
{
    SnakePart* current = head;
    while (current)
    {
        free(current);
        current = current->next_part;
    }
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

void clearGrid(GridBlock*** grid)
{
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
        {
            grid[i][j]->fruit = false;
            grid[i][j]->snake_part_on_me = NULL;
        } 
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

void deleteGrid(GridBlock*** grid)
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)     
            free(grid[i][j]->rect);
        free(grid[i]);
    }
    free(grid);
}

void updateSnakePosition(bool* out_arr, SnakePart* head, GridBlock*** grid, Uint32 input, bool grow)
{
    SnakePart* current = head;
    bool killed = false;
    Uint32 i, j = 0;
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
        }
    Uint32 next_new_i, next_new_j = 0;
    Uint32 head_i = i;
    Uint32 head_j = j;
    if (i >= 0 && i < ROWS && j >= 0 && j < COLS)
    {
        out_arr[1] = grid[i][j]->fruit ? true : false;
        grid[i][j]->fruit = false;
    }
    else
    {
        out_arr[0] = true;
        out_arr[1] = false;
        return;
    }
    SnakePart* bef = NULL;
    while (current != NULL)
    {
        grid[current->i][current->j]->snake_part_on_me = NULL;
        if(grid[i][j]->snake_part_on_me == NULL)
            grid[i][j]->snake_part_on_me = current;
        else if (current == head)
        {
            killed = true;
            break;
        }

        occupied_grid_blocks[current->i * COLS + current->j] = false;
        occupied_grid_blocks[i * COLS + j] = true;

        next_new_i = current->i;
        next_new_j = current->j;

        current->i = i;
        current->j = j;

        i = next_new_i;
        j = next_new_j;
        
        bef = current;
        current = current->next_part;
    }
    if (grow)
    {
        SnakePart* new_part = (SnakePart*)malloc(sizeof(SnakePart));
        bef->next_part = new_part;
        new_part->i = next_new_i;
        new_part->j = next_new_j;
        new_part->next_part = NULL;
        new_part->is_even = !bef->is_even;
        occupied_grid_blocks[next_new_i * COLS + next_new_j] = true;
    }
    out_arr[0] = killed;
}

void addPoints(Uint64 time_of_last_eaten, Uint64 time_of_before_last_eaten,  long long* score)
{
    if (time_of_last_eaten - time_of_before_last_eaten < 3000)
    {
        *score += time_bonus;
        time_bonus += 10;
    }
    else
    {
        *score += 5;
        time_bonus = BASE_TIME_BONUS;
    }
}

void killSnake(SDL_Renderer* renderer, SnakePart* head, GridBlock*** grid, int fps)
{
    SnakePart* current = head;
    Uint64 wanted_frame_time = (Uint64) 1000 / fps;
    Uint64 beg_of_frame;
    Uint64 frame_time;
    while (current != NULL)
    {
        beg_of_frame = SDL_GetTicks();
        grid[current->i][current->j]->snake_part_on_me = NULL;
        drawGrid(renderer, grid);

        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);

        current = current->next_part;

        frame_time = SDL_GetTicks() - beg_of_frame;
        if (frame_time < wanted_frame_time)
            SDL_Delay(wanted_frame_time - frame_time);
    }
}

// first-fit ako ne nadje iz prve
void placeFruit(GridBlock*** grid)
{
    srand(time(NULL));
    int len = ROWS * COLS;
    long long rand1 = rand();
    srand(time(NULL));
    long long rand2 = rand();
    // printa za rand2 da je nula a nije najjaca forica, al radi ok..., nije losa distribucija vockica
    int random = (rand1 * rand2) % len;
    int row = random / COLS;
    int col = random % COLS;
    if (!occupied_grid_blocks[random])
        grid[row][col]->fruit = true;
    else
    {
        int counter = random + 1;
        bool found = false;
        while(counter != random && !found)
        {
            if (!occupied_grid_blocks[counter])
            {
                int row = counter / COLS;
                int col = counter % COLS;
                grid[row][col]->fruit = true;
                found = true;
            }
            counter = (counter + 1) % len;
        }
    }
}

int main(int argc, char** argv)
{
    FILE* file = (FILE*)makeOrOpenHighscoreFile();
    int hs;
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Neuspesna inicijalizacija SDL video moda.\n");
        return -1;
    }
    int len = ROWS * COLS;
    for (int i = 0; i < len; i++)
        occupied_grid_blocks[i] = false; 

    SDL_Window* window = SDL_CreateWindow("Snake game", BLOCK_SIZE * COLS, BLOCK_SIZE * ROWS, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowResizable(window, false);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Event event;
    GridBlock*** grid = allocGrid();

    Uint32 fps = 10;
    Uint64 wanted_frame_time = (Uint64) 1000 / fps;
    Uint64 beg_of_frame;
    Uint64 frame_time;
    bool running = true;
    while (running)
    {   
        SnakePart* snake_head = makeSnake(grid);
        SnakePart* snake_tail = snake_head->next_part->next_part;
        Uint32 last_arrow_input = SDLK_DOWN;
        clearGrid(grid);
        bool killed = false;
        bool grow = false;
        long long points = 0;
        placeFruit(grid);
        Uint64 time_last_eaten = SDL_GetTicks();
        if (file != NULL)
        {
            hs = readHighscore(file);
            printf("Rekord -> %d\n", hs);
        }
        while(running && !killed)
        {
            beg_of_frame = SDL_GetTicks();
            if (SDL_PollEvent(&event))
            {
                if ((event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) || event.type == SDL_EVENT_QUIT)
                    running = false;
                switch (event.key.key) 
                {
                    case SDLK_UP:
                        if(last_arrow_input != SDLK_DOWN)
                            last_arrow_input = SDLK_UP;
                        break;
                    case SDLK_DOWN:
                        if(last_arrow_input != SDLK_UP)
                            last_arrow_input = SDLK_DOWN;
                        break;
                    case SDLK_LEFT:
                        if(last_arrow_input != SDLK_RIGHT)
                            last_arrow_input = SDLK_LEFT;
                        break;
                    case SDLK_RIGHT:
                        if(last_arrow_input != SDLK_LEFT)
                            last_arrow_input = SDLK_RIGHT; 
                }
            }
            bool outarr[2];
            updateSnakePosition(outarr, snake_head, grid, last_arrow_input, grow);
            killed = outarr[0];
            if (killed)       
                break;
            grow = outarr[1];
            if (grow)
            {
                placeFruit(grid);
                Uint64 time_now_eaten = SDL_GetTicks();
                addPoints(time_now_eaten, time_last_eaten, &points);
                printf("Poeni -> %d\n", points);
                time_last_eaten = time_now_eaten;
            }
            drawGrid(renderer, grid);

            SDL_RenderPresent(renderer);
            SDL_RenderClear(renderer);

            frame_time = SDL_GetTicks() - beg_of_frame; 
            if(frame_time < wanted_frame_time)
                SDL_Delay(wanted_frame_time - frame_time);
        }
        if (killed)
        {
            if (points > hs)
                {
                    updateHighscore(file, points);
                    printf("Novi rekord! -> %d\n", readHighscore(file));
                    hs = points;
                }
            killSnake(renderer, snake_head, grid, 30);
        }
        bool restart = false;
        while(!restart && running)
        {
            while(SDL_PollEvent(&event))
            {
                if ((event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) 
                    || event.type == SDL_EVENT_QUIT)
                    running = false;
                else if (event.key.key == SDLK_R)
                {   
                    time_bonus = BASE_TIME_BONUS;
                    system("cls"); 
                    restart = true;
                }
            }   
        }
        deleteSnake(snake_head);
    }
    fclose(file);
    deleteGrid(grid);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}