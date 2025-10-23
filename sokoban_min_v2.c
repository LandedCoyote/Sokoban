// sokoban_min.c
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

enum { H = 7, W = 9 };  // ステージサイズ
enum Tile { TILE_EMPTY=0, TILE_WALL=1, TILE_PLAYER=2, TILE_BOX=3, TILE_GOAL=4 };  //各タイル番号

typedef struct {
    int random_box_count;         // ランダム生成時の目標箱数
    int random_extra_walls_min;   // ランダム生成時の追加壁数最小
    int random_extra_walls_max;   // ランダム生成時の追加壁数最大
} GenerationConfig;

static const GenerationConfig kGenerationConfig = {
    .random_box_count = 3,
    .random_extra_walls_min = 0,
    .random_extra_walls_max = 4,
};

// ステージデータ（同じサイズのステージを複数保持）
static const int gMaps[][H*W] = {
    {
        1,1,1,1,1,1,1,1,1,
        1,0,0,0,4,0,0,0,1,
        1,0,0,0,0,0,0,0,1,  // ← プレイヤー(2)は初期位置検出用
        1,0,0,0,3,0,0,0,1,
        1,0,0,0,0,0,0,0,1,
        1,0,0,0,2,0,0,0,1,
        1,1,1,1,1,1,1,1,1
    },
    {
        1,1,1,1,1,1,1,1,1,
        1,0,4,0,4,0,4,0,1,
        1,0,3,0,3,0,0,0,1,
        1,0,3,0,0,0,0,0,1,
        1,0,0,0,2,0,0,0,1,
        1,0,0,0,0,0,0,0,1,
        1,1,1,1,1,1,1,1,1
    }
};
static const int kStageCount = sizeof(gMaps) / sizeof(gMaps[0]);

static int base_map_[H*W];  // 固定マップ（床/壁/ゴール）
static int box_map_[H*W];   // 荷物の存在フラグ
static int px, py;          // プレイヤー位置（列=px, 行=py）
static struct termios oldt; // 端末設定の退避
static char current_stage_label[64]; // 現在のステージ名表示
static int next_predefined_stage;    // 次に遊ぶ既存マップの番号
static int random_stage_counter;     // 生成マップの通し番号

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
static int is_stage_cleared(void);

static void load_predefined_stage(int stage_index) {
    int player_found = 0;
    const int *map_data = gMaps[stage_index];
    for (int y=0;y<H;y++) {
        for (int x=0;x<W;x++) {
            int index = idx(y, x);
            int tile = map_data[index];
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
    snprintf(current_stage_label, sizeof(current_stage_label),
             "Predefined %d/%d", stage_index + 1, kStageCount);
}

static int build_random_stage_layout(void) {
    // 初期化: 外周は壁、内部は空にする
    for (int y=0; y<H; ++y) {
        for (int x=0; x<W; ++x) {
            int index = idx(y, x);
            if (y==0 || y==H-1 || x==0 || x==W-1) {
                base_map_[index] = TILE_WALL;
            } else {
                base_map_[index] = TILE_EMPTY;
            }
            box_map_[index] = 0;
        }
    }

    int cells[H*W];
    int count = 0;
    for (int y=1; y<H-1; ++y) {
        for (int x=1; x<W-1; ++x) {
            cells[count++] = idx(y, x);
        }
    }

    if (count <= 0) {
        px = 1; py = 1;
        return 0;
    }

    // シャッフル
    for (int i=count-1; i>0; --i) {
        int j = rand() % (i + 1);
        int tmp = cells[i];
        cells[i] = cells[j];
        cells[j] = tmp;
    }

    int pos = 0;
    if (pos >= count) return 0;
    int player_index = cells[pos++];
    px = player_index % W;
    py = player_index / W;

    int num_boxes = kGenerationConfig.random_box_count;
    if (num_boxes < 1) num_boxes = 1;
    int max_boxes = (count - pos) / 2;
    if (max_boxes < 1) max_boxes = 1;
    if (num_boxes > max_boxes) num_boxes = max_boxes;

    for (int i=0; i<num_boxes && pos < count; ++i) {
        base_map_[cells[pos++]] = TILE_GOAL;
    }
    int boxes_placed = 0;
    for (int i=0; i<num_boxes && pos < count; ++i) {
        box_map_[cells[pos++]] = 1;
        boxes_placed++;
    }

    if (boxes_placed == 0) {
        return 0;
    }

    // 余ったセルにランダムで壁を置く
    int min_walls = kGenerationConfig.random_extra_walls_min;
    int max_walls = kGenerationConfig.random_extra_walls_max;
    if (max_walls < min_walls) {
        int tmp = min_walls;
        min_walls = max_walls;
        max_walls = tmp;
    }
    if (min_walls < 0) min_walls = 0;
    if (max_walls < 0) max_walls = 0;
    int extra_walls = min_walls;
    if (max_walls > min_walls) {
        extra_walls = min_walls + rand() % (max_walls - min_walls + 1);
    }
    int remaining_cells = count - pos;
    if (extra_walls > remaining_cells) {
        extra_walls = remaining_cells;
    }
    for (int i=0; i<extra_walls && pos < count; ++i) {
        base_map_[cells[pos++]] = TILE_WALL;
    }

    return 1;
}

typedef struct {
    uint64_t boxes_bits;
    unsigned char player_cell;
} SolverState;

static int solver_boxes_on_goals(uint64_t boxes_bits, const int *bit_to_cell, int cell_count) {
    for (int bit=0; bit<cell_count; ++bit) {
        if (!(boxes_bits & (1ULL << bit))) continue;
        int cell = bit_to_cell[bit];
        if (base_map_[cell] != TILE_GOAL) {
            return 0;
        }
    }
    return 1;
}

static int solver_enqueue(SolverState *queue, int max_states, int *tail,
                          uint64_t boxes_bits, int player_cell,
                          uint64_t *visited_boxes, unsigned char *visited_player,
                          unsigned char *visited_used, int visited_capacity) {
    uint64_t key = boxes_bits ^ ((uint64_t)player_cell * 1099511628211ULL);
    int mask = visited_capacity - 1;
    int slot = (int)(key & mask);
    while (visited_used[slot]) {
        if (visited_boxes[slot] == boxes_bits &&
            visited_player[slot] == (unsigned char)player_cell) {
            return 0;
        }
        slot = (slot + 1) & mask;
    }
    visited_used[slot] = 1;
    visited_boxes[slot] = boxes_bits;
    visited_player[slot] = (unsigned char)player_cell;
    if (*tail >= max_states) return -1;
    queue[(*tail)++] = (SolverState){ boxes_bits, (unsigned char)player_cell };
    return 1;
}

static int is_current_stage_solvable(void) {
    int cell_to_bit[H*W];
    int bit_to_cell[H*W];
    memset(cell_to_bit, -1, sizeof(cell_to_bit));

    int cell_count = 0;
    for (int i=0; i<H*W; ++i) {
        if (base_map_[i] != TILE_WALL) {
            cell_to_bit[i] = cell_count;
            bit_to_cell[cell_count] = i;
            cell_count++;
        }
    }

    if (cell_count <= 0) return 0;

    uint64_t start_boxes = 0;
    int num_boxes = 0;
    for (int i=0; i<H*W; ++i) {
        if (!box_map_[i]) continue;
        int bit = cell_to_bit[i];
        if (bit < 0) return 0;
        start_boxes |= (1ULL << bit);
        num_boxes++;
    }
    if (num_boxes == 0) return 0;

    int player_cell = idx(py, px);
    if (cell_to_bit[player_cell] < 0) return 0;

    const int kMaxStates = 1 << 17;       // 131072
    const int kVisitedCapacity = 1 << 18; // 262144 (power of two)
    SolverState *queue = malloc(sizeof(SolverState) * kMaxStates);
    if (!queue) return 0;

    uint64_t *visited_boxes = malloc(sizeof(uint64_t) * kVisitedCapacity);
    unsigned char *visited_player = malloc(sizeof(unsigned char) * kVisitedCapacity);
    unsigned char *visited_used = malloc(sizeof(unsigned char) * kVisitedCapacity);
    if (!visited_boxes || !visited_player || !visited_used) {
        free(queue);
        free(visited_boxes);
        free(visited_player);
        free(visited_used);
        return 0;
    }
    memset(visited_used, 0, sizeof(unsigned char) * kVisitedCapacity);

    int tail = 0;
    int enqueue_result = solver_enqueue(queue, kMaxStates, &tail,
                                        start_boxes, player_cell,
                                        visited_boxes, visited_player,
                                        visited_used, kVisitedCapacity);
    if (enqueue_result == -1) {
        free(queue);
        free(visited_boxes);
        free(visited_player);
        free(visited_used);
        return 0;
    }

    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, -1, 1 };

    int solvable = 0;
    int head = 0;
    while (head < tail) {
        SolverState st = queue[head++];
        if (solver_boxes_on_goals(st.boxes_bits, bit_to_cell, cell_count)) {
            solvable = 1;
            break;
        }

        int py_local = st.player_cell / W;
        int px_local = st.player_cell % W;

        for (int dir=0; dir<4; ++dir) {
            int nx = px_local + dx[dir];
            int ny = py_local + dy[dir];
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            int next_cell = idx(ny, nx);
            if (base_map_[next_cell] == TILE_WALL) continue;
            int next_bit = cell_to_bit[next_cell];
            uint64_t new_boxes = st.boxes_bits;
            if (next_bit >= 0 && (st.boxes_bits & (1ULL << next_bit))) {
                int bx = nx + dx[dir];
                int by = ny + dy[dir];
                if (bx < 0 || bx >= W || by < 0 || by >= H) continue;
                int box_cell = idx(by, bx);
                if (base_map_[box_cell] == TILE_WALL) continue;
                int box_bit = cell_to_bit[box_cell];
                if (box_bit < 0) continue;
                if (st.boxes_bits & (1ULL << box_bit)) continue;
                new_boxes &= ~(1ULL << next_bit);
                new_boxes |= (1ULL << box_bit);
            } else {
                if (next_bit >= 0 && (st.boxes_bits & (1ULL << next_bit))) {
                    continue;
                }
            }

            int res = solver_enqueue(queue, kMaxStates, &tail,
                                     new_boxes, next_cell,
                                     visited_boxes, visited_player,
                                     visited_used, kVisitedCapacity);
            if (res == -1) {
                solvable = 0;
                goto solver_cleanup;
            }
            if (res == 0) {
                continue;
            }
        }
    }

solver_cleanup:
    free(queue);
    free(visited_boxes);
    free(visited_player);
    free(visited_used);
    return solvable;
}

static void build_fallback_stage_layout(void) {
    for (int y=0; y<H; ++y) {
        for (int x=0; x<W; ++x) {
            int index = idx(y, x);
            if (y==0 || y==H-1 || x==0 || x==W-1) {
                base_map_[index] = TILE_WALL;
            } else {
                base_map_[index] = TILE_EMPTY;
            }
            box_map_[index] = 0;
        }
    }
    int interior_cells[H*W];
    int interior_count = 0;
    for (int y=1; y<H-1; ++y) {
        for (int x=1; x<W-1; ++x) {
            interior_cells[interior_count++] = idx(y, x);
        }
    }
    if (interior_count >= 3) {
        int player_cell = interior_cells[0];
        px = player_cell % W;
        py = player_cell / W;
        int box_cell = interior_cells[1];
        int goal_cell = interior_cells[2];
        base_map_[goal_cell] = TILE_GOAL;
        box_map_[box_cell] = 1;
        return;
    }

    // 極端に小さいマップ向けの最低限の回避策
    int route_cells[3];
    int route_count = 0;
    if (H > 1) {
        int row = (H > 2) ? 1 : 0;
        for (int x=1; x<W && route_count<3; ++x) {
            int cell = idx(row, x);
            base_map_[cell] = TILE_EMPTY;
            route_cells[route_count++] = cell;
        }
    }
    if (W > 1 && route_count < 3) {
        int col = (W > 2) ? 1 : 0;
        for (int y=1; y<H && route_count<3; ++y) {
            int cell = idx(y, col);
            int duplicate = 0;
            for (int i=0; i<route_count; ++i) {
                if (route_cells[i] == cell) {
                    duplicate = 1;
                    break;
                }
            }
            if (duplicate) continue;
            base_map_[cell] = TILE_EMPTY;
            route_cells[route_count++] = cell;
        }
    }
    if (route_count >= 3) {
        int player_cell = route_cells[0];
        int box_cell = route_cells[1];
        int goal_cell = route_cells[2];
        px = player_cell % W;
        py = player_cell / W;
        box_map_[box_cell] = 1;
        base_map_[goal_cell] = TILE_GOAL;
    } else {
        px = 1;
        py = 1;
    }
}

static void generate_random_stage(void) {
    random_stage_counter++;
    snprintf(current_stage_label, sizeof(current_stage_label),
             "Random #%d", random_stage_counter);

    const int kMaxAttempts = 256;
    for (int attempt=0; attempt<kMaxAttempts; ++attempt) {
        if (!build_random_stage_layout()) continue;
        if (is_stage_cleared()) continue;
        if (is_current_stage_solvable()) {
            return;
        }
    }

    build_fallback_stage_layout();
}

static void draw(void) {
    // 画面クリア & カーソル先頭へ
    printf("\x1b[2J\x1b[H");
    printf("%s\n", current_stage_label);
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

enum StageResult { STAGE_QUIT=0, STAGE_CLEARED=1 };

static enum StageResult play_stage(void) {
    draw();
    if (is_stage_cleared()) {
        printf("[%s] Clear!\n", current_stage_label);
        fflush(stdout);
        sleep(1);
        return STAGE_CLEARED;
    }

    for (;;) {
        int k = read_key();
        if (k < 0) return STAGE_QUIT;
        if (k=='q' || k=='Q') return STAGE_QUIT;

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
                printf("[%s] Clear!\n", current_stage_label);
                fflush(stdout);
                sleep(1);
                return STAGE_CLEARED;
            }
        }
    }
}

enum MenuChoice { MENU_PREDEFINED=1, MENU_RANDOM, MENU_QUIT };

static enum MenuChoice show_menu(void) {
    for (;;) {
        printf("\x1b[2J\x1b[H");
        printf("=== Sokoban ===\n");
        if (next_predefined_stage < kStageCount) {
            printf("1: 既存マップをプレイ (次: %d/%d)\n",
                   next_predefined_stage + 1, kStageCount);
        } else {
            printf("1: 既存マップをプレイ (全ステージクリア済み → ステージ1から再開)\n");
        }
        printf("2: 自動生成マップをプレイ\n");
        printf("Q: ゲーム終了\n");
        fflush(stdout);

        int k = read_key();
        if (k=='1') return MENU_PREDEFINED;
        if (k=='2') return MENU_RANDOM;
        if (k=='q' || k=='Q') return MENU_QUIT;
    }
}

int main(void) {
    set_raw_mode();
    srand((unsigned)time(NULL));
    next_predefined_stage = 0;
    random_stage_counter = 0;

    int running = 1;
    while (running) {
        enum MenuChoice choice = show_menu();
        switch (choice) {
            case MENU_PREDEFINED: {
                if (next_predefined_stage >= kStageCount) {
                    printf("\x1b[2J\x1b[H");
                    printf("全既存マップをクリアしました。ステージ1から再開します。\n");
                    fflush(stdout);
                    sleep(1);
                    next_predefined_stage = 0;
                }
                load_predefined_stage(next_predefined_stage);
                next_predefined_stage++;
                if (play_stage() == STAGE_QUIT) {
                    running = 0;
                }
                break;
            }
            case MENU_RANDOM:
                generate_random_stage();
                if (play_stage() == STAGE_QUIT) {
                    running = 0;
                }
                break;
            case MENU_QUIT:
                running = 0;
                break;
        }
    }

    return 0;
}
