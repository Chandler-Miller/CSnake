#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <ncurses.h>
#include <math.h>

struct Node {
    int X;
    int Y;
    int x_direction;
    int y_direction;
    struct Node* next;
};

struct PathNode {
    int X;
    int Y;
    float G;
    float H;
    struct PathNode* Parent;
};

struct Snake {
    struct Node* parts;
    int x_speed;
    int y_speed;
    int length;
    int x_direction;
    int y_direction;
};

struct Food {
    int X;
    int Y;
};

int width = 60;
int height = 30;
bool gameover = false;

bool equal_node(const struct PathNode* primary, const struct PathNode* other) {
    return (primary->X == other->X) && (primary->Y == other->Y);
}

struct PathNode** reconstruct_path(struct PathNode* node, int* length) {
    struct PathNode** path = malloc(sizeof(struct PathNode*));
    *path = NULL; // Initialize with a NULL pointer

    while (node != NULL) {
        path = realloc(path, (*length + 1) * sizeof(struct PathNode*));
        path[*length] = node;
        node = node->Parent;
        (*length)++;
    }

    return path;
}


struct PathNode** a_star_search(struct PathNode* start, struct PathNode* dest, int** grid, int grid_rows, int grid_cols, int* length) {
    struct PathNode** open_list = malloc(sizeof(struct PathNode*));
    int open_list_length = 1;

    struct PathNode** closed_list = malloc(sizeof(struct PathNode*));
    int closed_list_length = 0;

    while (open_list_length > 0) {
        struct PathNode* current = open_list[0];
        int current_index = 0;

        for (int i = 1; i < open_list_length; i++) {
            if (open_list[i]->G + open_list[i]->H < current->G + current->H) {
                current = open_list[i];
                current_index = i;
            }
        }

        if (equal_node(current, dest)) {
            return reconstruct_path(current, length);
        }

        open_list[current_index] = open_list[open_list_length - 1];
        open_list_length--;
        closed_list = realloc(closed_list, (closed_list_length + 1) * sizeof(struct PathNode*));
        closed_list[closed_list_length] = current;
        closed_list_length++;

        struct PathNode** neighbors = malloc(4 * sizeof(struct PathNode*));
        int num_neighbors = 0;

        int directions[4][2] = {
            {-1, 0}, // left
            {1, 0},  // right
            {0, -1}, // up
            {0, 1}   // down
        };

        for (int i = 0; i < 4; i++) {
            int x = current->X + directions[i][0];
            int y = current->Y + directions[i][1];

            if (x >= 0 && x < grid_cols && y >= 0 && y < grid_rows) {
                struct PathNode* neighbor = malloc(sizeof(struct PathNode));
                neighbor->X = x;
                neighbor->Y = y;
                neighbors[num_neighbors] = neighbor;
                num_neighbors++;
            }
        }

        for (int i = 0; i < num_neighbors; i++) {
            struct PathNode* neighbor = neighbors[i];

            if (grid[neighbor->Y][neighbor->X] == 1) {
                continue;
            }

            double newG = current->G + sqrt(pow(neighbor->X - current->X, 2) + pow(neighbor->Y - current->Y, 2));

            bool in_open_list = false;
            for (int j = 0; j < open_list_length; j++) {
                if (equal_node(open_list[j], neighbor)) {
                    in_open_list = true;
                    break;
                }
            }

            if (!in_open_list || newG < neighbor->G) {
                neighbor->Parent = current;
                neighbor->G = newG;
                neighbor->H = sqrt(pow(neighbor->X - dest->X, 2) + pow(neighbor->Y - dest->Y, 2));

                if (!in_open_list) {
                    open_list = realloc(open_list, (open_list_length + 1) * sizeof(struct PathNode*));
                    open_list[open_list_length] = neighbor;
                    open_list_length++;
                }
            }
        }

        free(neighbors);
    }

    return NULL;
}

void push(struct Node** snake_head, int new_x, int new_y) {
    struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
    new_node->X = new_x;
    new_node->Y = new_y;
    new_node->next = NULL;

    if (*snake_head == NULL) {
        *snake_head = new_node;
        return;
    }

    struct Node* last = *snake_head;
    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_node;
}

void free_snake(struct Node* snake) {
    struct Node* current = snake;
    while (current != NULL) {
        struct Node* temp = current;
        current = current->next;
        free(temp);
    }
}

void update_snake(struct Snake* snake, struct Food* food) {
    struct Node* current = snake->parts;
    struct Node* prev = NULL;
    int tempX, tempY;

    while (current != NULL) {
        tempX = current->X;
        tempY = current->Y;

        current->X += current->x_direction;
        current->Y += current->y_direction;

        current->x_direction = (prev != NULL) ? prev->X - current->X : snake->x_direction;
        current->y_direction = (prev != NULL) ? prev->Y - current->Y : snake->y_direction;

        prev = current;
        current = current->next;
    }

    // Check collision with self
    current = snake->parts->next; // Skip the head
    while (current != NULL) {
        if (snake->parts->X == current->X && snake->parts->Y == current->Y) {
            gameover = true;
            return;
        }
        current = current->next;
    }

    if (snake->parts->X < 0 || snake->parts->X >= width || snake->parts->Y <= 1 || snake->parts->Y >= height) {
        gameover = true;
        return;
    }

    // Check collision with food
    if (snake->parts->X == food->X && snake->parts->Y == food->Y) {
        // Generate new food position
        food->X = rand() % width;
        food->Y = rand() % (height - 3) + 2;

        // Increase snake length
        struct Node* tail = snake->parts;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        push(&(snake->parts), tail->X, tail->Y);
        snake->length++;
    }
}

void draw(struct Node* snake, struct Food* food, struct PathNode* start, struct PathNode* dest, int** grid) {
    clear();

    start->X = snake->X;
    start->Y = snake->Y;
    start->G = 0.0;
    start->H = 0.0;
    start->Parent = NULL;

    dest->X = food->X;
    dest->Y = food->Y;
    dest->G = 0.0;
    dest->H = 0.0;
    dest->Parent = NULL;

    // Draw top boundary
    for (int i = 0; i < width; ++i) {
        mvprintw(0, i, "#");
        grid[0][i] = 1;
    }

    // Draw side boundaries
    for (int y = 1; y <= height; ++y) {
        mvprintw(y, 0, "#");
        mvprintw(y, width + 1, "#");
        grid[y][0] = 1;
    }

    // Draw snake
    struct Node* current = snake;
    while (current != NULL) {
        mvprintw(current->Y, current->X, "*");
        grid[current->Y][current->X] = 1;
        current = current->next;
    }

    // Draw food
    mvprintw(food->Y, food->X, "@");
    grid[food->Y][food->X] = 3;

    // Draw bottom boundary
    for (int i = 0; i < width + 2; i++) {
        mvprintw(height + 1, i, "#");
        grid[height + 1][i] = 1;
    }

    refresh();
}

void game_over() {
    gameover = true;
    clear();
    mvprintw(height / 2, (width - 9) / 2, "GAME OVER!");
    refresh();
    sleep(3);
}

void path_snake(struct PathNode** path, struct Snake* snake, int length) {
    if (length > 1) {
        int x = path[1]->X - path[0]->X;
        int y = path[1]->Y - path[0]->Y;

        snake->x_direction = x;
        snake->y_direction = y;
        snake->x_speed = x;
        snake->y_speed = y;
    }
}


void gameloop(struct Snake* snake, struct Food* food, struct PathNode* start, struct PathNode* dest, int grid_rows, int grid_cols) {
    int input;
    int grid[30][60] = {0};

    while (!gameover) {
        input = getch();
        switch (input) {
            case ERR: // No input available
                break;
            case 'w': // Up
            case 'W':
                if (snake->y_direction != 1) {
                    snake->y_direction = -1;
                    snake->x_direction = 0;
                    snake->y_speed = -1;
                    snake->x_speed = 0;
                }
                break;
            case 's': // Down
            case 'S':
                if (snake->y_direction != -1) {
                    snake->y_direction = 1;
                    snake->x_direction = 0;
                    snake->y_speed = 1;
                    snake->x_speed = 0;
                }
                break;
            case 'a': // Left
            case 'A':
                if (snake->x_direction != 1) {
                    snake->x_direction = -1;
                    snake->y_direction = 0;
                    snake->x_speed = -1;
                    snake->y_speed = 0;
                }
                break;
            case 'd': // Right
            case 'D':
                if (snake->x_direction != -1) {
                    snake->x_direction = 1;
                    snake->y_direction = 0;
                    snake->x_speed = 1;
                    snake->y_speed = 0;
                }
                break;
            case 'q': // Quit
            case 'Q':
                game_over();
                break;
            }

        if (gameover) {
            break;
        }

        update_snake(snake, food);
        draw(snake->parts, food, start, dest, grid);
        int length = 0;
        struct PathNode** path = a_star_search(start, dest, grid, grid_rows, grid_cols, &length);
        path_snake(path, snake, length);
        usleep(100000);
    }

    game_over();
}

int main() {
    initscr(); // Initialize ncurses
    cbreak(); // Disable line buffering
    noecho(); // Disable automatic echoing of typed characters
    keypad(stdscr, TRUE); // Enable special keys, e.g., arrow keys
    nodelay(stdscr, TRUE); // Enable non-blocking input
    curs_set(0); // Hide the cursor

    int grid_rows = height;
    int grid_cols = width;
    
    //malloc(grid_rows * sizeof(int*));
    // for (int i = 0; i < grid_rows; i++) {
    //     grid[i] = malloc(grid_cols * sizeof(int));
    // }

    struct Node* head = (struct Node*) malloc(sizeof(struct Node));
    head->X = width / 2;
    head->Y = height / 2;
    head->x_direction = 1;
    head->y_direction = 0;
    head->next = NULL;

    struct PathNode start = {
        .X = head->X,
        .Y = head->Y,
        .G = 0.0,
        .H = 0.0,
        .Parent = NULL
    };
    // malloc(sizeof(struct PathNode*));
    // start->X = head->X;
    // start->Y = head->Y;
    // start->G = 0.0;
    // start->H = 0.0;
    // start->Parent = NULL;

    struct Snake snake;
    snake.parts = head;
    snake.x_speed = 1;
    snake.y_speed = 0;
    snake.length = 1;
    snake.x_direction = 1;
    snake.y_direction = 0;

    for (int i = 1; i <= 2; i++) {
        push(&(snake.parts), width / 2 - i, height / 2);
    }

    struct Food* food = (struct Food*) malloc(sizeof(struct Food));
    food->X = rand() % width;
    food->Y = rand() % (height - 3) + 2;

    struct PathNode* dest = malloc(sizeof(struct PathNode));
    dest->X = food->X;
    dest->Y = food->Y;
    dest->G = 0.0;
    dest->H = 0.0;
    dest->Parent = NULL;

    gameloop(&snake, food, &start, dest, grid_rows, grid_cols);

    free_snake(snake.parts);

    free(food);
    free(dest);

    endwin(); // Clean up ncurses

    return 0;
}
