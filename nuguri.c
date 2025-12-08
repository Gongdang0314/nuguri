#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <time.h>

#define MAX_ENEMIES 15
#define MAX_COINS 30 

typedef struct {
    int x, y;
    int dir;
} Enemy;

typedef struct {
    int x, y;
    int collected;
} Coin;

char*** map = NULL;
int player_x, player_y;
int stage = 0;
int score = 0;
int MAX_STAGES = 0;
int MAP_WIDTH = 0;
int MAP_HEIGHT = 0;
int lives = 3;
#ifdef _WIN32
HANDLE hTimer = NULL;
#endif

int is_jumping = 0;
int velocity_y = 0;
int on_ladder = 0;

Enemy enemies[MAX_ENEMIES];
int enemy_count = 0;
Coin coins[MAX_COINS];
int coin_count = 0;

#ifdef _WIN32
// _getch
#else
struct termios orig_termios;
#endif

void disable_raw_mode();
void enable_raw_mode();
void load_maps();
void init_stage();
void draw_game();
void update_game(char input);
void move_player(char input);
void move_enemies();
void check_collisions();
void delay(int ms);
int kbhit();
void show_title_screen();
void show_ending_screen();
void show_game_over_screen();
void load_map_size();
void malloc_map();
void free_map();
void hide_cursor();
void show_cursor();
void sound_beep_maker(int hz, int ms);
void sound_game_start();
void sound_coin();
void sound_hit();
void sound_die();
void sound_stage_clear();
void sound_game_complete();


int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    hTimer = CreateWaitableTimer(NULL, TRUE, NULL); 
    #endif
    srand(time(NULL));
    hide_cursor();
    enable_raw_mode();
    sound_game_start();
    show_title_screen();
    printf("\x1b[2J\x1b[H");
    load_map_size();
    malloc_map();
    load_maps();
    init_stage();

    char c = '\0';
    int game_over = 0;

    while (!game_over && stage < MAX_STAGES) {
        #ifdef _WIN32
        if (kbhit()) {
            c = _getch();
            if (c == 'q') {
                game_over = 1;
                continue;
            }  
        #else 
        if(kbhit()) {
            c = getchar();
            if (c == 'q') {
                game_over = 1;
                continue;
            }
        #endif
        #ifdef _WIN32
            if (c == '\0' || c == '\xe0') { 
                switch (_getch()) { 
                    case 72: c = 'w'; break;
                    case 80: c = 's'; break;
                    case 77: c = 'd'; break;
                    case 75: c = 'a'; break;
                    default: c = '\0'; break;
                }
            }
        #else
            if (c == '\x1b') {
                getchar();
                switch (getchar()) { 
                    case 'A': c = 'w'; break;
                    case 'B': c = 's'; break;
                    case 'C': c = 'd'; break;
                    case 'D': c = 'a'; break;
                    default: c = '\0'; break;
                }
            } 
        #endif
        }else {
            c = '\0';
        }

        update_game(c);
        draw_game();
        delay(90);

        if (map[stage][player_y][player_x] == 'E') {
            stage++;
            score += 100;
            if (stage < MAX_STAGES) {
                sound_stage_clear();
                memset(coins, 0, sizeof(coins));
                init_stage();
            } else {
                sound_game_complete();
                game_over = 1;
                show_ending_screen();
            }
        }
    }

    show_cursor();
    disable_raw_mode();
    free_map();
    #ifdef _WIN32
    if (hTimer != NULL) {
        CloseHandle(hTimer);
    }
    #endif
    return 0;
}

void disable_raw_mode() { 
#ifdef _WIN32
// getch
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); 
#endif
}
void enable_raw_mode() {
#ifdef _WIN32
// getch
#else
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

void load_maps() {
    FILE *file = fopen("map.txt", "r");
    if (!file) {
        perror("map.txt 파일을 열 수 없습니다.");
        exit(1);
    }
    int s = 0, r = 0;
    char line[MAP_WIDTH + 3];
    while (s < MAX_STAGES && fgets(line, sizeof(line), file)) {
        if ((line[0] == '\n' || line[0] == '\r') && r > 0) {
            s++;
            r = 0;
            continue;
        }
        if (r < MAP_HEIGHT) {
            line[strcspn(line, "\n\r")] = 0;
            strncpy(map[s][r], line, MAP_WIDTH + 1);
            r++;
        }
    }
    fclose(file);
}


void init_stage() {
    enemy_count = 0;
    coin_count = 0;
    is_jumping = 0;
    velocity_y = 0;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S') {
                player_x = x;
                player_y = y;
            } else if (cell == 'X' && enemy_count < MAX_ENEMIES) {
                enemies[enemy_count] = (Enemy){x, y, (rand() % 2) * 2 - 1};
                enemy_count++;
            } else if (cell == 'C' && coin_count < MAX_COINS) {
                if(lives == 3 || coins[coin_count].collected != 1){
                    coins[coin_count] = (Coin){x, y, 0};
                }
                coin_count++;
            }
        }
    }
}

void draw_game() {
    
    char display_map[MAP_HEIGHT][MAP_WIDTH + 1];
    for(int y=0; y < MAP_HEIGHT; y++) {
        for(int x=0; x < MAP_WIDTH; x++) {
            char cell = map[stage][y][x];
            if (cell == 'S' || cell == 'X' || cell == 'C') {
                display_map[y][x] = ' ';
            } else {
                display_map[y][x] = cell;
            }
        }
    }
    
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected) {
            display_map[coins[i].y][coins[i].x] = 'C';
        }
    }

    for (int i = 0; i < enemy_count; i++) {
        display_map[enemies[i].y][enemies[i].x] = 'X';
    }

    display_map[player_y][player_x] = 'P';

    
    char map_array[154 + (MAP_HEIGHT * (MAP_WIDTH + 1)) + 1];
    int current_length = 0;
    current_length += sprintf(map_array + current_length,"\x1b[HHeart: ");
    current_length += sprintf(map_array + current_length,"\x1b[1;7H");
    current_length += sprintf(map_array + current_length,"       "); 
    current_length += sprintf(map_array + current_length,"\x1b[1;7H");
    
    for(int i = 0; i < lives; i++){
        current_length += sprintf(map_array + current_length, "♥ ");
    }
    current_length += sprintf(map_array + current_length, "\n");
    current_length += sprintf(map_array + current_length, "Stage: %d | Score: %d\n", stage + 1, score);
    current_length += sprintf(map_array + current_length, "조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for(int x=0; x< MAP_WIDTH; x++){
            map_array[current_length++] = display_map[y][x];
        }
        map_array[current_length++] = '\n';
    }
    map_array[current_length] = '\0';

    printf("%s", map_array);
}

void update_game(char input) {
    move_player(input);
    move_enemies();
    check_collisions();
}

void move_player(char input) {
    int next_x = player_x, next_y = player_y;
    char floor_tile = (player_y + 1 < MAP_HEIGHT) ? map[stage][player_y + 1][player_x] : '#';
    char current_tile = map[stage][player_y][player_x];

    on_ladder = (current_tile == 'H');

    switch (input) {
        case 'a': next_x--; break;
        case 'd': next_x++; break;
        case 'w': if (on_ladder) next_y--; break;
        case 's': if (on_ladder && (player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] != '#') next_y++; break;
        case ' ':
            if (!is_jumping && (floor_tile == '#' || on_ladder)) {
                is_jumping = 1;
                velocity_y = -2;
            }
            break;
    }
    
    if (input == 'w' || input == 's' || on_ladder)
    {

        char changed = 0;

        if (on_ladder && (input == 'w' || input == 's')) {
            if(next_y >= 0 && next_y < MAP_HEIGHT && map[stage][next_y][player_x] != '#') {
                player_y = next_y;
                is_jumping = 0;
                velocity_y = 0;
                changed = 1;
            }
        } 

        if (input == 'w' && on_ladder)
        { 
            if (player_y - 1 >= 0 && map[stage][player_y - 1][player_x] == '#')
            { 
                if (player_y - 2 >= 0 && map[stage][player_y - 2][player_x] != '#' && !changed)
                {
                    next_y = player_y - 2; 
                    player_y = next_y;     
                    is_jumping = 0;        
                    velocity_y = 0;     
                }
                else
                {
                    next_y = player_y;
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
            }
        }

        else if (input == 's')
        {
            if (on_ladder && (player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] == '#')
            { 
                if ((player_y + 2 < MAP_HEIGHT) && map[stage][player_y + 2][player_x] == 'H')
                {
                    next_y = player_y + 2;
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
                else
                {
                    next_y = player_y;
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
            }
            else if (!on_ladder && (player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] == '#')
            { 
                if ((player_y + 2 < MAP_HEIGHT) && map[stage][player_y + 2][player_x] == 'H')
                {
                    next_y = player_y + 2;
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
                else
                {
                    next_y = player_y;
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
            }
        }
    }
    else {
        if (is_jumping) {
            next_y = player_y + velocity_y;
            if(next_y < 0) next_y = 0;
            velocity_y++;

            if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] == '#') {
                next_y = player_y;
                velocity_y = 0;
                is_jumping = 0;
            }
            else if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] != '#' && map[stage][player_y - 2][player_x] != '#' && map[stage][player_y - 3][player_x] != '#') {
                //3칸점프
            }
            else if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] != '#' && map[stage][player_y - 2][player_x] != '#') {
                next_y = player_y - 2;
                velocity_y = 0;
                is_jumping = 0;
            }
            else if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] != '#') {
                next_y = player_y - 1;
                velocity_y = 0;
                is_jumping = 0;
            }

            if(velocity_y > 0){ 
                for(int i = 1; i <= velocity_y && (player_y + i) < MAP_HEIGHT; i++){
                    if(map[stage][player_y + i][player_x] == '#'){
                        next_y = player_y + i - 1;
                        is_jumping = 0;
                        velocity_y = 0;
                        break;
                    }
                }
            }

            if (velocity_y < 0)
            { 
                for (int i = 1; i <= -velocity_y; i++)
                {
                    if (map[stage][player_y - i][player_x] == 'X')
                    {
                        next_y = player_y - i;
                        is_jumping = 0;
                        velocity_y = 0;
                    }
                }
            }
            
            if (velocity_y > 0)
            { 
                for (int i = 1; i <= velocity_y; i++)
                {
                    if (map[stage][player_y + i][player_x] == 'X')
                    {
                        next_y = player_y + i;
                        is_jumping = 0;
                        velocity_y = 0;
                    }
                }
            }

            if (velocity_y < 0)
                for (int i = 1; i <= -velocity_y; i++)
                {
                    if (map[stage][player_y - i][player_x] == 'C')
                    {
                        for (int j = 0; j < coin_count; j++)
                        {
                            if (!coins[j].collected && coins[j].x == player_x && coins[j].y == player_y - i)
                            {   
                                sound_coin();
                                coins[j].collected = 1;
                                score += 20;
                            }
                        }
                    }
                }

            if (velocity_y > 0)
                for (int i = 1; i <= velocity_y; i++)
                {
                    if (map[stage][player_y + i][player_x] == 'C')
                    {
                        for (int j = 0; j < coin_count; j++)
                        {
                            if (!coins[j].collected && coins[j].x == player_x && coins[j].y == player_y + i)
                            {   
                                sound_coin();
                                coins[j].collected = 1;
                                score += 20;
                            }
                        }
                    }
                }
            
            if (next_y < MAP_HEIGHT) {
                player_y = next_y;
            }
            
            if ((player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] == '#') {
                is_jumping = 0;
                velocity_y = 0;
            }
        } else {
            if (floor_tile != '#' && floor_tile != 'H') {
                 if (player_y + 1 < MAP_HEIGHT) player_y++;
                 else init_stage();
            }
        }
    }

    if (next_x >= 0 && next_x < MAP_WIDTH && map[stage][player_y][next_x] != '#') player_x = next_x;
    
    if (player_y >= MAP_HEIGHT) init_stage();
}


void move_enemies() {
    for (int i = 0; i < enemy_count; i++) {
        int next_x = enemies[i].x + enemies[i].dir;
        if (next_x < 0 || next_x >= MAP_WIDTH || map[stage][enemies[i].y + 1][enemies[i].x] != '#' && (enemies[i].y + 1 < MAP_HEIGHT && map[stage][enemies[i].y + 1][next_x] == ' '))
        {
            enemies[i].x = next_x;
        }
        else if (next_x < 0 || next_x >= MAP_WIDTH || map[stage][enemies[i].y][next_x] == '#' || (enemies[i].y + 1 < MAP_HEIGHT && map[stage][enemies[i].y + 1][next_x] == ' ')) {
            enemies[i].dir *= -1;
        } else {
            enemies[i].x = next_x;
        }
    }
}

void check_collisions() {
    for (int i = 0; i < enemy_count; i++) {
       if (player_x == enemies[i].x && player_y == enemies[i].y) {
            lives--;
            if(lives <= 0){
                sound_die();
                show_game_over_screen();
                free_map();
                #ifdef _WIN32
                if(hTimer != NULL) CloseHandle(hTimer);
                #endif
                exit(0);
            } else {
                sound_hit();
                init_stage();
                return;
            }
        }

    }
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y) {
            sound_coin();
            coins[i].collected = 1;
            score += 20;
        }
    }
}

int kbhit() {
#ifdef _WIN32
    return _kbhit();
#else
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
#endif
}
void delay(int ms)
{
#ifdef _WIN32
    LARGE_INTEGER sleep_time;
    sleep_time.QuadPart = -10000LL * ms;

    SetWaitableTimer(hTimer, &sleep_time, 0, NULL, NULL, FALSE);

    WaitForSingleObject(hTimer, INFINITE);
    
#else
  usleep(ms * 1000); 
#endif
}
void show_title_screen() {
    printf("\x1b[2J\x1b[H");
    printf("    #   #  #  #   ###  #  #  ###   ###  \n");
    printf("    ##  #  #  #  #     #  #  #  #   #   \n");
    printf("    # # #  #  #  #  #  #  #  # ##   #   \n");
    printf("    #  ##  #  #  #  #  #  #  # #    #   \n");
    printf("    #   #   ##   ####   ##   # ##  ###  \n");

    printf("Press any key to start...\n");

#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
}

void show_ending_screen() {
    printf("\x1b[2J\x1b[H");
    printf("  #######    ###     ##     ##  #######\n");
    printf(" ##     ##  ## ##    ###   ###  ##     \n");
    printf(" ##   #### ##   ##   #### ####  ###### \n");
    printf(" ##     ## #######   ## ### ##  ##     \n");
    printf("  #######  ##   ##   ##     ##  #######\n");
    printf("\n");
    printf("  ####  ##     #####   ###    ######  #\n");
    printf(" ##     ##     ##     ## ##   ##  ##  #\n");
    printf(" ##     ##     ####   ## ##   ######  #\n");
    printf(" ##     ##     ##     #####   ##  ##   \n");
    printf("  ####  #####  #####  ## ##   ##   ## #\n");
    printf("\n\n");
    printf("            최종점수 : %d\n", score);

#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
}

void show_game_over_screen() {
    printf("\x1b[2J\x1b[H");

  printf("\n\n");
printf("   #######    ###     ##     ##  ########   \n");
printf("  ##     ##  ## ##    ###   ###  ##         \n");
printf("  ##   #### ##   ##   #### ####  ######     \n");
printf("  ##     ## #######   ## ### ##  ##         \n");
printf("   #######  ##   ##   ##     ##  ########   \n");
printf("\n");
printf("   #######  ##     ## ########  ########    \n");
printf("  ##     ## ##     ## ##        ##     ##   \n");
printf("  ##     ## ##     ## ######    ########    \n");
printf("  ##     ##  ##   ##  ##        ##   ##     \n");
printf("   #######     ###    ########  ##     ##    \n");
printf("\n\n");
    printf("          최종 점수 : %d\n", score);
    printf("     Press any key to exit...\n");



#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
}

void load_map_size() {
    FILE *file = fopen("map.txt", "r");
    if (!file) {
        perror("map.txt 파일을 열 수 없습니다.");
        exit(1);
    }

    char line[1000];
    int current_stage_height = 0;
    int total_stages = 0;
    int max_map_width = 0;
    int max_map_height = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n\r")] = '\0';
        int len = strlen(line);

        if (len == 0) {
            if (current_stage_height > 0) {
                total_stages++;
                if (current_stage_height > max_map_height) {
                    max_map_height = current_stage_height;
                }
                current_stage_height = 0;
            }
            continue;
        }

        if (len > 0) { 
            current_stage_height++;
            if (len > max_map_width) {
                max_map_width = len;
            }
        }
    }

    fclose(file);
    
    if (current_stage_height > 0) {
        total_stages++;
        if (current_stage_height > max_map_height) {
            max_map_height = current_stage_height;
        }
    }
    
    MAX_STAGES = total_stages;
    MAP_WIDTH = max_map_width;
    MAP_HEIGHT = max_map_height;
}

void malloc_map() {
    map = (char***)malloc(MAX_STAGES * sizeof(char**));
    if (map == NULL) { 
        perror("스테이지 배열 메모리 할당 실패"); 
        exit(1); 
    }

    for (int s = 0; s < MAX_STAGES; s++) {
        map[s] = (char**)malloc(MAP_HEIGHT * sizeof(char*));
        if (map[s] == NULL) { 
            perror("스테이지 맵 높이 배열 메모리 할당 실패");
            
            for (int i = 0; i < s; i++) {
                for(int j = 0; j < MAP_HEIGHT; j++){
                    free(map[i][j]);
                }
                free(map[i]);
            }
            free(map);
            
            exit(1); 
        }

        for (int r = 0; r < MAP_HEIGHT; r++) {
            map[s][r] = (char*)malloc((MAP_WIDTH + 1) * sizeof(char));
            if (map[s][r] == NULL) { 
                perror("맵 너비 배열 메모리 할당 실패");
                
                for (int j = 0; j < r; j++) {
                    free(map[s][j]);
                }
                free(map[s]);
                for (int i = 0; i < s; i++) {
                    for(int j = 0; j < MAP_HEIGHT; j++){
                        free(map[i][j]);
                    }
                    free(map[i]);
                }
                free(map); 
                
                exit(1); 
            }
            memset(map[s][r], '\0', MAP_WIDTH + 1);
        }
    }
}

void free_map() {
    if (map == NULL) {
        return; 
    }
    
    for (int s = 0; s < MAX_STAGES; s++) 
    { 
        if (map[s] != NULL)
        {
            for (int r = 0; r < MAP_HEIGHT; r++) 
            { 
                if (map[s][r] != NULL) {
                    free(map[s][r]);
                }
            }
            
            free(map[s]);
        }
    }
    
    free(map);
}

void hide_cursor(){
    printf("\x1b[?25l");
}

void show_cursor(){
    printf("\x1b[?25h");
}


void sound_beep_maker(int hz, int ms) {
#ifdef _WIN32
    Beep(hz, ms);
#else
    printf("\a");
    fflush(stdout);
#endif
}

void sound_game_start() {
    sound_beep_maker(784, 80);  
    delay(10); 
    sound_beep_maker(880, 80);  
    delay(10);
    sound_beep_maker(1046, 80); 
    delay(10);
    sound_beep_maker(1318, 80); 
    delay(10); 
    sound_beep_maker(1568, 150); 
}

void sound_coin() {
    printf("\a");
    fflush(stdout); 
}

void sound_hit() {
    sound_beep_maker(800, 300); 
}

void sound_die() {
    sound_beep_maker(800, 600); 
    delay(5); 
    sound_beep_maker(200, 400); 
}

void sound_stage_clear() {
    sound_beep_maker(1046, 150); 
    delay(50); 
    
    sound_beep_maker(1318, 150); 
    delay(50);
    
    sound_beep_maker(1568, 200); 
    delay(50);
    
    sound_beep_maker(2093, 400); 
}

void sound_game_complete() {
    sound_beep_maker(523, 120);  
    delay(50); 
    sound_beep_maker(659, 120);  
    delay(50);
    sound_beep_maker(784, 150);  
    delay(50);
    sound_beep_maker(1046, 150); 
    delay(50); 
    sound_beep_maker(1318, 200); 
    delay(100); 
    sound_beep_maker(2093, 700); 
}