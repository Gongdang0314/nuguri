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

// 맵 및 게임 요소 정의 (수정된 부분)
#define MAP_WIDTH 40  // 맵 너비를 40으로 변경
#define MAP_HEIGHT 20
#define MAX_STAGES 2
#define MAX_ENEMIES 15 // 최대 적 개수 증가
#define MAX_COINS 30   // 최대 코인 개수 증가

// 구조체 정의
typedef struct {
    int x, y;
    int dir; // 1: right, -1: left
} Enemy;

typedef struct {
    int x, y;
    int collected;
} Coin;

// 전역 변수
char map[MAX_STAGES][MAP_HEIGHT][MAP_WIDTH + 1];//3차원 배열 스테이지 마다 맵이 존재
int player_x, player_y;//플레이어의 위치를 전역
int stage = 0;//현재 스테이지
int score = 0;// 현재 점수판

// 플레이어 상태
int is_jumping = 0;//점프키 누르면 여기에 담았다가 점프 동작
int velocity_y = 0;// y방향 속도
int on_ladder = 0;// 사다리 탔는지 안탔는지 표시

// 게임 객체
Enemy enemies[MAX_ENEMIES];//적의 수만큼 다른 위치로 적들 소환 적 15마리
int enemy_count = 0;//현재 남은 적, 현재 해치운 적들수
Coin coins[MAX_COINS];//현재 맵에 흩뿌려져있는 코인 개수 30개
int coin_count = 0;//남은 코인수인가, 먹은 코인수

#ifdef _WIN32
DWORD orig_console;
HANDLE hStdin;
#else
// 터미널 설정
struct termios orig_termios;//현재 터미널 설정을 저장, 초기화 안됨
#endif

// 함수 선언
void disable_raw_mode();
void enable_raw_mode();
void load_maps();//맵 로드 함수
void init_stage();//stage 초기화 함수, 클리어시나 처음 게임 시작할때 작동
void draw_game();//게임 그리기, 게임판 그리기
void update_game(char input);//화면 갱신
void move_player(char input);// player의 입력을 받아서 그에 맞게 플레이어를 움직여주는 함수
void move_enemies();//적들의 구조체 내부 정보에 의거하여 적들이 움직이게 하는 함수
void check_collisions();//충돌검사해서 지나갈수있나 아니면 아이템 먹기 플레이어 사망
void delay(int ms);//usleep쓰는 부분이 있어서 window와 호환과 재사용성을 위한 함수
int kbhit();//입력받는다고 게임 전체 멈추면 안되서 씀

int main() {
    srand(time(NULL));//랜덤함수의 시드값 설정
    enable_raw_mode();
    load_maps();//맵 불러오기
    init_stage();//스테이지 초기화

    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8); // 콘솔 출력 인코딩을 UTF-8로 설정
    #endif

    char c = '\0';// 널문자
    int game_over = 0;// 아직 게임 오버 안됨 

    while (!game_over && stage < MAX_STAGES) {// 게임오버 안됬고 스테이지 끝까지 깨서 겜클리어 안했으면 게임 돌리기,q일경우 무한 반복문 빠져나감 겜 종료
        if (kbhit()) {//사용자가 입력하면
            c = getchar();
            if (c == 'q') {//q일경우에 겜 종료
                game_over = 1;
                continue;
            }
            if (c == '\x1b') {//16진수로 x가 16진수 표시, 1(16) + b(11) = 27 -> escape, Ansi escape code 사용
                getchar(); // '['
                switch (getchar()) {
                    case 'A': c = 'w'; break; // Up
                    case 'B': c = 's'; break; // Down
                    case 'C': c = 'd'; break; // Right
                    case 'D': c = 'a'; break; // Left
                }
            }
        } else {
            c = '\0';
        }

        update_game(c);
        draw_game();
        delay(90);

        if (map[stage][player_y][player_x] == 'E') {
            stage++;
            score += 100;
            if (stage < MAX_STAGES) {
                init_stage();
            } else {
                game_over = 1;
                printf("\x1b[2J\x1b[H");
                printf("축하합니다! 모든 스테이지를 클리어했습니다!\n");
                printf("최종 점수: %d\n", score);
            }
        }
    }

    disable_raw_mode();//??
    return 0;
}

// 터미널 Raw 모드 활성화/비활성화
void disable_raw_mode() { 
#ifdef _WIN32
    SetConsoleMode(hStdin, orig_console);
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); 
#endif
}
void enable_raw_mode() {
#ifdef _WIN32
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &orig_console);
    atexit(disable_raw_mode);
    DWORD raw_mode = orig_console;
    raw_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(hStdin, raw_mode);
#else
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

// 맵 파일 로드
void load_maps() {
    FILE *file = fopen("map.txt", "r");
    if (!file) {
        perror("map.txt 파일을 열 수 없습니다.");
        exit(1);
    }
    int s = 0, r = 0;
    char line[MAP_WIDTH + 2]; // 버퍼 크기는 MAP_WIDTH에 따라 자동 조절됨
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


// 현재 스테이지 초기화
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
                coins[coin_count++] = (Coin){x, y, 0};
            }
        }
    }
}

// 게임 화면 그리기
void draw_game() {
    printf("\x1b[2J\x1b[H");
    printf("Stage: %d | Score: %d\n", stage + 1, score);
    printf("조작: ← → (이동), ↑ ↓ (사다리), Space (점프), q (종료)\n");

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

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for(int x=0; x< MAP_WIDTH; x++){
            printf("%c", display_map[y][x]);
        }
        printf("\n");
    }
}

// 게임 상태 업데이트
void update_game(char input) {
    move_player(input);
    move_enemies();
    check_collisions();
}

// 플레이어 이동 로직
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

    if (next_x >= 0 && next_x < MAP_WIDTH && map[stage][player_y][next_x] != '#') player_x = next_x;
    
    if (on_ladder && (input == 'w' || input == 's')) {
        if(next_y >= 0 && next_y < MAP_HEIGHT && map[stage][next_y][player_x] != '#') {
            player_y = next_y;
            is_jumping = 0;
            velocity_y = 0;
        }
    } 
    else {
        if (is_jumping) {
            next_y = player_y + velocity_y;
            if(next_y < 0) next_y = 0;
            velocity_y++;

            if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][next_y][player_x] == '#') {
                velocity_y = 0;
            } else if (next_y < MAP_HEIGHT) {
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
    
    if (player_y >= MAP_HEIGHT) init_stage();
}


// 적 이동 로직
void move_enemies() {
    for (int i = 0; i < enemy_count; i++) {
        int next_x = enemies[i].x + enemies[i].dir;
        if (next_x < 0 || next_x >= MAP_WIDTH || map[stage][enemies[i].y][next_x] == '#' || (enemies[i].y + 1 < MAP_HEIGHT && map[stage][enemies[i].y + 1][next_x] == ' ')) {
            enemies[i].dir *= -1;
        } else {
            enemies[i].x = next_x;
        }
    }
}

// 충돌 감지 로직
void check_collisions() {
    for (int i = 0; i < enemy_count; i++) {
        if (player_x == enemies[i].x && player_y == enemies[i].y) {
            score = (score > 50) ? score - 50 : 0;
            init_stage();
            return;
        }
    }
    for (int i = 0; i < coin_count; i++) {
        if (!coins[i].collected && player_x == coins[i].x && player_y == coins[i].y) {
            coins[i].collected = 1;
            score += 20;
        }
    }
}

// 비동기 키보드 입력 확인
int kbhit() {
#ifdef _WIN32
    _kbhit();
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
  Sleep(ms);
#else
  usleep(ms * 1000); // <unistd.h>에 선언된 usleep의 단위인 마이크로초에 * 1000 = 밀리초
#endif
}