// sokoban_min.c
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

enum { H = 5, W = 7 };  // ステージサイズ
enum Tile { TILE_EMPTY=0, TILE_WALL=1, TILE_PLAYER=2, TILE_BOX=3, TILE_GOAL=4 };  //各タイル番号

// 初期マップデータ
static const int gMapInit[H*W] = {
    1,1,1,1,1,1,1,
    1,0,0,4,0,0,1,
    1,0,3,2,0,0,1,  // ← プレイヤー(2)は初期位置検出用
    1,0,0,0,0,0,1,
    1,1,1,1,1,1,1
};

static int base_map_[H*W];  // 固定マップ（床/壁/ゴール）
static int box_map_[H*W];   // 荷物の存在フラグ
static int px, py;          // プレイヤー位置（列=px, 行=py）
static struct termios oldt; // 端末設定の退避

// --- 端末制御 ---
static void restore_tty(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    // カーソル表示ON
    printf("\x1b[?25h");
    fflush(stdout);
}
static void set_raw_mode(void) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &oldt);
    t = oldt;
    t.c_lflag &= ~(ICANON | ECHO);    // カノニカル/エコーOFF
    t.c_cc[VMIN]  = 1;                // 1文字でreadが返る
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    atexit(restore_tty);
    // カーソル非表示
    printf("\x1b[?25l");
}

// --- ユーティリティ ---
static inline int idx(int y, int x) { return y*W + x; }

static void init_map(void) {
    int player_found = 0;
    for (int y=0;y<H;y++) {
        for (int x=0;x<W;x++) {
            int index = idx(y, x);
            int tile = gMapInit[index];
            base_map_[index] = TILE_EMPTY;
            box_map_[index]  = 0;
            switch (tile) {
                case TILE_WALL:
                    base_map_[index] = TILE_WALL;
                    break;
                case TILE_GOAL:
                    base_map_[index] = TILE_GOAL;
                    break;
                case TILE_BOX:
                    box_map_[index] = 1;
                    break;
                case TILE_PLAYER:
                    px = x;
                    py = y;
                    player_found = 1;
                    break;
                default:
                    break;
            }
        }
    }
    // プレイヤー初期位置が見つからなければデフォルト
    if (!player_found) {
        py = 1; px = 1;
    }
}

static void draw(void) {
    // 画面クリア & カーソル先頭へ
    printf("\x1b[2J\x1b[H");
    for (int y=0;y<H;y++) {
        for (int x=0;x<W;x++) {
            int index = idx(y, x);
            // プレイヤー位置なら最優先で描く
            if (y==py && x==px) { putchar('@'); continue; }
            if (box_map_[index]) {
                int base = base_map_[index];
                putchar(base == TILE_GOAL ? '*' : '$');
                continue;
            }
            switch (base_map_[index]) {
                case TILE_WALL:  putchar('#'); break;
                case TILE_GOAL:  putchar('.'); break;
                default:         putchar(' '); break;
            }
        }
        putchar('\n');
    }
    fflush(stdout);
}

// 入力1キー読む。矢印は ESC [ A/B/C/D を処理
static int read_key(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    if (c == 0x1b) { // ESC
        unsigned char seq[2];
        // 期待: '[' と 方向コード
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return 0x1b;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return 0x1b;
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return 'U'; // Up
                case 'B': return 'D'; // Down
                case 'C': return 'R'; // Right
                case 'D': return 'L'; // Left
            }
        }
        return 0; // 未対応
    }
    // wasd もサポート
    if (c=='w'||c=='W') return 'U';
    if (c=='s'||c=='S') return 'D';
    if (c=='a'||c=='A') return 'L';
    if (c=='d'||c=='D') return 'R';
    return c; // それ以外はそのまま返す（qなど）
}

// 進行可否判定
static int is_walkable(int y, int x) {
    if (x<0||x>=W||y<0||y>=H) return 0;
    int index = idx(y, x);
    if (base_map_[index] == TILE_WALL) return 0;
    if (box_map_[index]) return 0;
    return 1;
}

// ステージクリア判定
static int is_stage_cleared(void) {
    for (int i=0; i<H*W; i++) {
        if (box_map_[i] && base_map_[i] != TILE_GOAL) return 0;
    }
    return 1;
}

int main(void) {
    set_raw_mode();
    init_map();
    draw();
    if (is_stage_cleared()) {
        printf("Game Clear\n");
        fflush(stdout);
        sleep(1);
        return 0;
    }

    for (;;) {
        int k = read_key();
        if (k < 0) break;
        if (k=='q' || k=='Q') break;

        int nx = px, ny = py;
        if (k=='U') ny--;
        else if (k=='D') ny++;
        else if (k=='L') nx--;
        else if (k=='R') nx++;

        if (nx!=px || ny!=py) {
            enum MoveCase { MOVE_FREE=0, MOVE_BLOCKED, MOVE_BOX };
            enum MoveCase move_case = MOVE_FREE;
            if (nx<0||nx>=W||ny<0||ny>=H) {
                move_case = MOVE_BLOCKED;
            } else {
                int dest_index = idx(ny, nx);
                if (base_map_[dest_index] == TILE_WALL) {
                    move_case = MOVE_BLOCKED;
                } else if (box_map_[dest_index]) {
                    move_case = MOVE_BOX;
                }
            }
            switch (move_case) {
                case MOVE_BLOCKED:
                    // 進行不可
                    break;
                case MOVE_BOX: {
                    int dx = nx - px;
                    int dy = ny - py;
                    int bx = nx + dx;
                    int by = ny + dy;
                    if (bx<0||bx>=W||by<0||by>=H) break;
                    int box_dest = idx(by, bx);
                    if (base_map_[box_dest] == TILE_WALL) break;
                    if (box_map_[box_dest]) break;
                    box_map_[idx(ny, nx)] = 0;
                    box_map_[box_dest] = 1;
                    px = nx; py = ny;
                    break;
                }
                case MOVE_FREE:
                    if (is_walkable(ny, nx)) {
                        px = nx; py = ny;
                    }
                    break;
            }
            draw();
            if (is_stage_cleared()) {
                printf("Game Clear\n");
                fflush(stdout);
                sleep(1);
                break;
            }
        }
    }

    return 0;
}
