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
// #define MAP_WIDTH 40  // 맵 너비를 40으로 변경
// #define MAP_HEIGHT 20
// #define MAX_STAGES 2
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
// char map[MAX_STAGES][MAP_HEIGHT][MAP_WIDTH + 1];//3차원 배열 스테이지 마다 맵이 존재
char*** map = NULL;
int player_x, player_y;//플레이어의 위치를 전역
int stage = 0;//현재 스테이지
int score = 0;// 현재 점수판
int MAX_STAGES = 0;
int MAP_WIDTH = 0;
int MAP_HEIGHT = 0;
int lives = 3;// 현재 남은 목숨;

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
void show_title_screen();
void show_ending_screen();
void show_game_over_screen();
void load_map_size();//맵 크기 가져오는 함수
void malloc_map();//맵 배열 malloc으로 만드는 함수
void free_map();//맵 배열 malloc free해주는 함수
void hide_cursor();
void show_cursor();
void sound_beep_maker(int hz, int ms);//hz(헤르츠), ms(밀리세컨드)를 인자로 받아 비프음을 만드는 함수
void sound_game_start();// 게임 시작시 비프 효과음을 출력
void sound_coin();// 'C'획득시 비프 효과음을 출력
void sound_hit();// 몬스터에게 맞고 살아있을 시 비프 효과음을 출력
void sound_die();// 몬스터에게 맞고 죽을시 비프 효과음을 출력
void sound_stage_clear();// 스테이지 클리어시 비프 효과음을 출력
void sound_game_complete();// 게임을 완전히 클리어시 비프 효과음을 출력


int main() {
    srand(time(NULL));//랜덤함수의 시드값 설정
    hide_cursor();//커서 숨기기
    enable_raw_mode();
    sound_game_start();
    show_title_screen();
    printf("\x1b[2J\x1b[H");//타이틀시작화면지우기
    load_map_size();
    malloc_map();
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
                sound_stage_clear();
                init_stage();
            } else {
                sound_game_complete();
                game_over = 1;
                show_ending_screen();
            }
        }
    }

    show_cursor();//커서 보이기
    disable_raw_mode();
    free_map();
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
    printf("\x1b[H");
    printf("Heart: ");
    printf("\x1b[1;7H");//이전화면 하트 지우기
    printf("       ");//하트는 갱신되어야해서 지워줌, 아니면 이전 화면의 하트 잔상이 남음
    printf("\x1b[1;7H");//다시 하트 그리기전에 위치 잡아주기
    for(int i = 0; i < lives; i++){
        printf("♥ ");
    }
    printf("\n");
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
    
    if (input == 'w' || input == 's' || on_ladder)
    { // 꼭 사다리 위가 아니여도 위치조정하게하여 벽에 붙은 사다리로 이동할수있게함, onladder 조건 추가로 else 중력 적용 로직으로 가는거 막음, 사다리에 매달려있으니

        char changed = 0;//w일때 2번적용됨, play가 둘다 업데이트되어서

        if (on_ladder && (input == 'w' || input == 's')) {//기존 코드의 사다리위에서 상하동작 로직
            if(next_y >= 0 && next_y < MAP_HEIGHT && map[stage][next_y][player_x] != '#') {
                player_y = next_y;
                is_jumping = 0;
                velocity_y = 0;
                changed = 1;
            }
        } 

        if (input == 'w' && on_ladder)
        { // 맨위 사다리위에서 w를 누를때
            if (player_y - 1 >= 0 && map[stage][player_y - 1][player_x] == '#')
            { // 한칸위만 벽일때, 통과가능
                if (player_y - 2 >= 0 && map[stage][player_y - 2][player_x] != '#' && !changed)//w 연속 두번 적용 막기
                {
                    next_y = player_y - 2; // 맨위가 벽이여도 벽이 한칸이면 이동, 벽을 통과함(벽에 안끼게 하기위해서)
                    player_y = next_y;     // 실제 위치값 업데이트
                    is_jumping = 0;        // 점프가 아니여야 점프로직이 사다리로직 방해안함
                    velocity_y = 0;        // 전역변수라 점프아니면 초기화하는게 오류방지에 좋음, 아니면 다음에 값이 남음
                }
                else
                {
                    next_y = player_y; // 한칸만 벽이여야 통과가능 벽 2칸 이상은 안됨, 이동 불가
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
            }
        }

        else if (input == 's')
        {
            if (on_ladder && (player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] == '#')
            { // 1사다리 벽 사다리일때,플레이어가 1사다리에 있을때, 발밑의 벽1개넘어의 사다리를 타야할때
                if ((player_y + 2 < MAP_HEIGHT) && map[stage][player_y + 2][player_x] == 'H')
                {
                    next_y = player_y + 2; // 벽을 넘어 사다리로 바로이동(벽끼임 방지)
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
                else
                {
                    next_y = player_y; // 발밑이 벽 사다리형태가 아니면, 이동 불가
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
            }
            else if (!on_ladder && (player_y + 1 < MAP_HEIGHT) && map[stage][player_y + 1][player_x] == '#')
            { // 발밑 벽 사다리일때, 플레이어가 벽1개넘어의 사다리를 타야할때
                if ((player_y + 2 < MAP_HEIGHT) && map[stage][player_y + 2][player_x] == 'H')
                {
                    next_y = player_y + 2; // 벽을 넘어 사다리로 바로이동(벽끼임 방지)
                    player_y = next_y;
                    is_jumping = 0;
                    velocity_y = 0;
                }
                else
                {
                    next_y = player_y; // 발밑이 벽 사다리형태가 아니면, 이동 불가
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

            if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] == '#') {//바로 위가 벽 점프막기
                next_y = player_y;//0칸 점프
                velocity_y = 0;
                is_jumping = 0;//점프 아님
            }
            else if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] != '#' && map[stage][player_y - 2][player_x] != '#' && map[stage][player_y - 3][player_x] != '#') {
                //3칸점프할수있으면 그대로 냅두기, -2 ,-1 
            }
            else if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] != '#' && map[stage][player_y - 2][player_x] != '#') {//점프할 공간 2칸, 한번에 2칸만 점프하기
                next_y = player_y - 2;//2칸 점프
                velocity_y = 0;//위로는 더 안감
                is_jumping = 0;//점프아님 이제 추락함
            }
            else if (velocity_y < 0 && next_y < MAP_HEIGHT && map[stage][player_y - 1][player_x] != '#') {//점프할 공간 1칸, 1칸만 점프하기
                next_y = player_y - 1;//1칸 점프
                velocity_y = 0;//위로는 더 안감
                is_jumping = 0;//점프아님 이제 추락함
            }

            if(velocity_y > 0){  // 점프 후 낙하 시 바닥이 뚫리는 현상 제거 (velocity=3이면 player_y+3이기 때문에 그 사이 벽은 체크가 안됨 반복문으로 하나하나 체크)
                for(int i = 1; i <= velocity_y && (player_y + i) < MAP_HEIGHT; i++){
                    if(map[stage][player_y + i][player_x] == '#'){
                        next_y = player_y + i - 1;
                        is_jumping = 0;
                        velocity_y = 0;
                        break;
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

    if (next_x >= 0 && next_x < MAP_WIDTH && map[stage][player_y][next_x] != '#') player_x = next_x;//상하로 움직있는 다음 수평을 검사하면 중간에 벽뚫릴일이없음, 수평검사해놓고 위로 올라가버리면 위의 칸 상황은 다를 수있기때문에 위에칸에서도 수평검사가 필요, 마지막에 수평 검사하면 점프중간에 벽과 겹칠일이없음
    
    if (player_y >= MAP_HEIGHT) init_stage();
}


// 적 이동 로직
void move_enemies() {
    for (int i = 0; i < enemy_count; i++) {
        int next_x = enemies[i].x + enemies[i].dir;
        if (next_x < 0 || next_x >= MAP_WIDTH || map[stage][enemies[i].y + 1][enemies[i].x] != '#' && (enemies[i].y + 1 < MAP_HEIGHT && map[stage][enemies[i].y + 1][next_x] == ' '))//공중에서 리젠된 X는 공중에서 왔다갔다하기
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

// 충돌 감지 로직
void check_collisions() {
    for (int i = 0; i < enemy_count; i++) {
       if (player_x == enemies[i].x && player_y == enemies[i].y) {
            lives--;
            if(lives <= 0){
                sound_die();
                show_game_over_screen();
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
printf("   #######    ###    ########  ##     ##    \n");
printf("\n\n");
    printf("          최종 점수 : %d\n", score);
    printf("     Press any key to exit...\n");



#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
}

//맵 크기중에 제일 큰거 저장, 스테이지 개수 저장
void load_map_size() {
    FILE *file = fopen("map.txt", "r");
    if (!file) {
        perror("map.txt 파일을 열 수 없습니다.");
        exit(1);
    }

    char line[1000];//동적 할당을 위해 충분히 큰값을 너비로
    int current_stage_height = 0;
    int total_stages = 0;
    int max_map_width = 0;
    int max_map_height = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n\r")] = '\0';
        int len = strlen(line);

        if (len == 0) {//빈줄이면 다음 스테이지임
            if (current_stage_height > 0) {
                total_stages++;//스테이지 개수 추가
                if (current_stage_height > max_map_height) {//맵의 최대 높이 찾기
                    max_map_height = current_stage_height;
                }
                current_stage_height = 0;//새맵되어서 맵 높이 초기화
            }
            continue;
        }

        if (len > 0) {//높이마다 최대 너비 찾기 
            current_stage_height++;
            if (len > max_map_width) {
                max_map_width = len;
            }
        }
    }

    fclose(file);// 자원 반환
    
    if (current_stage_height > 0) {// 마지막 줄 빈줄이없음 바로 map.txt의 마지막줄임, 스테이지 추가 시켜야함 이것도 맵임
        total_stages++;
        if (current_stage_height > max_map_height) {//맵 최대 높이 찾기
            max_map_height = current_stage_height;
        }
    }
    
    //전역변수에 map.txt 가져온값 넣기
    MAX_STAGES = total_stages;
    MAP_WIDTH = max_map_width;
    MAP_HEIGHT = max_map_height;
}

void malloc_map() {
    map = (char***)malloc(MAX_STAGES * sizeof(char**));//스테이지 개수만큼 공간 잡은 배열
    if (map == NULL) { 
        perror("스테이지 배열 메모리 할당 실패"); 
        exit(1); 
    }

    for (int s = 0; s < MAX_STAGES; s++) {
        map[s] = (char**)malloc(MAP_HEIGHT * sizeof(char*));//맵 높이 만큼 공간 잡은 배열
        if (map[s] == NULL) { 
            perror("스테이지 맵 높이 배열 메모리 할당 실패");
            
            for (int i = 0; i < s; i++) {//할당된 반대로 해제해주기, 할당된거부터 해제하면 안쪽해제할때 할당된 메모리 위치 못찾음
                free(map[i]);
            }
            free(map);
            
            exit(1); 
        }

        for (int r = 0; r < MAP_HEIGHT; r++) {
            map[s][r] = (char*)malloc((MAP_WIDTH + 1) * sizeof(char));//맵 너비+1(널문자)만큼 공간잡은 배열, 
            if (map[s][r] == NULL) { 
                perror("맵 너비 배열 메모리 할당 실패");
                
                for (int j = 0; j < r; j++) {
                    free(map[s][j]);
                }
                for (int i = 0; i <= s; i++) {
                    free(map[i]);
                }
                free(map); 
                
                exit(1); 
            }
            memset(map[s][r], '\0', MAP_WIDTH + 1);//배열 전체널 문자로 초기화
        }
    }
}

void free_map() {
    if (map == NULL) {//공간 안잡혀있다면 바로 return
        return; 
    }
    
    for (int s = 0; s < MAX_STAGES; s++) 
    { 
        if (map[s] != NULL)//공간 잡혀있으면 해제 
        {
            for (int r = 0; r < MAP_HEIGHT; r++) 
            { 
                if (map[s][r] != NULL) {
                    free(map[s][r]);//맵 x축 해제
                }
            }
            
            free(map[s]);//맵 y축 해제
        }
    }
    
    free(map);//맵 Stage 해제
}

void hide_cursor(){
    printf("\x1b[?25l");
}

void show_cursor(){
    printf("\x1b[?25h");
}


//비프음으로 효과음 maker
void sound_beep_maker(int hz, int ms) {
#ifdef _WIN32
    Beep(hz, ms);
#else
    //sudo apt install beep 필요
    //error : could not any device -> sudo modprobe (-r) pcspkr
    char command[100];
    
    snprintf(command, sizeof(command), "beep -f %d -l %d 2>/dev/null", hz, ms);
    
    system(command);
#endif
}

// 게임 시작 시 효과음
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
// 동전 먹을 때 효과음
void sound_coin() {
    sound_beep_maker(1760, 80); 
    delay(10); 
    sound_beep_maker(2093, 120); 
}

// 타격시 효과음
void sound_hit() {
    sound_beep_maker(800, 300); 
}

// 죽을 때 효과음
void sound_die() {
    sound_beep_maker(800, 600); 
    delay(5); 
    sound_beep_maker(200, 400); 
}

// 스테이지 클리어시 효과음
void sound_stage_clear() {
    sound_beep_maker(1046, 150); 
    delay(50); 
    
    sound_beep_maker(1318, 150); 
    delay(50);
    
    sound_beep_maker(1568, 200); 
    delay(50);
    
    sound_beep_maker(2093, 400); 
}

//게임 완전 클리어시 효과음
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