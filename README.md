# Sokoban Minimal (C / Terminal Edition) / 倉庫番ミニ（C / ターミナル版）


A **minimal Sokoban** implementation written in **pure C**, designed to run on **UNIX-like** terminals such as WSL (zsh / bash).
No external libraries — just raw terminal control, ANSI escape sequences, and good old C logic.

**UNIX系ターミナル（WSL, bash, zshなど）上で動作する、純C言語によるミニマルな倉庫番実装です。**
外部ライブラリは一切不要。ANSIエスケープと`termios`で制御しています。

---
## Features / 特徴

- Runs entirely in your terminal
    ターミナルだけで動作

- Arrow keys or WASD for movement
    矢印キーまたはWASDで操作

- Real-time input using `termios` (raw mode)
    `termios`によるリアルタイム入力対応（非カノニカルモード）

- Simple ASCII graphics:
  - `@` → Player
  - `$` → Box
  - `. `→ Goal
  - `#` → Wall
  - `*` → Box on Goal
    シンプルなASCII描画
	  - `@`: プレイヤー
	  - `$`: 荷物
	  - `.`: ゴール
	  - `#`: 壁
	  - `*`: ゴール上の荷物

- Stage clear detection when all boxes reach goals
    すべての荷物がゴールに乗るとステージクリア判定

---
## Build & Run / ビルドと実行

### 1. Compile / コンパイル

```bash
gcc sokoban_min.c -o sokoban_min
````

### 2. Run / 実行

```bash
./sokoban_min
```

### 3. Controls / 操作方法

| Key   | Action     | 説明   |
| ----- | ---------- | ---- |
| ↑ / W | Move Up    | 上に移動 |
| ↓ / S | Move Down  | 下に移動 |
| ← / A | Move Left  | 左に移動 |
| → / D | Move Right | 右に移動 |
| Q     | Quit       | 終了   |

---
## Technical Notes / 技術メモ

- Implements non-canonical terminal input via `<termios.h>`
    `<termios.h>`を用いた非カノニカル入力制御

- Works in raw mode for instant key response
    キー入力に即時反応するRAWモード動作

- Uses ANSI escape sequences to clear screen and hide cursor
    ANSIエスケープシーケンスによる画面制御とカーソル非表示

- Designed for clarity and learning — no dependencies
    依存ライブラリなし、学習目的にも最適
   
- Ideal for learning real-time terminal programming in C
    C言語でのリアルタイム入力・描画の基礎理解に向く

---
## Update History / 更新履歴

###  Sokoban Minimal v2 — Stage Generator & Menu Update / 倉庫番ミニ v2 ― ステージ生成＆メニュー対応

This update expands the original single-map Sokoban into a **multi-stage** and **auto-generated** version with a simple in-game menu.

今回のアップデートでは、単一マップ構成だった倉庫番を拡張し、**複数ステージ対応**および**自動生成マップ機能**を追加しました。

---
###  New Features / 新機能

- **Multiple Stage Support / 複数ステージ対応**
    The game now stores several predefined maps in `gMaps` and automatically advances to the next stage after each clear.
    `gMaps`配列で複数の固定マップを保持し、クリア後に自動的に次のステージへ進行。

- **Random Map Generation Mode / 自動生成モード**
    Generates random maps with walls, boxes, goals, and player position.
    Includes a **built-in solver** that ensures each map is actually solvable.
    壁・箱・ゴール・プレイヤー位置をランダムに配置し、**解けるマップのみ**を採用するソルバを内蔵。

- **Main Menu Added / メインメニュー追加**
    Choose between Predefined Maps, Random Generation, or Quit at startup.
    起動時に「既存マップ」「ランダム生成」「終了」から選択可能。

- **Replay & Progress Handling / 進行と再挑戦管理**
    After clearing all predefined stages, the game loops back to Stage 1.
    Random maps are numbered sequentially for tracking.
    全既存ステージをクリアするとステージ1に戻り、ランダムマップには通し番号を付与。

---
###  Technical Improvements / 技術的改善

- Split map data into `base_map_` (walls/goals) and `box_map_` (boxes).
    マップ構造を`base_map_`（壁・ゴール）と`box_map_`（荷物）に分離。

- Added stage labels (`current_stage_label`) for on-screen headers.
    画面上部にステージ情報を表示するヘッダを追加。

- Integrated a lightweight BFS-based solver to ensure solvability.
    BFS（幅優先探索）を用いた軽量ソルバで解可能性を判定。

- Modularized draw/input/game loops for better structure.
    描画・入力・判定処理をモジュール化し、拡張性を向上。

- Full `termios` raw-mode retained while adding stage/menu logic.
    `termios`によるRAW入力を維持しつつ、メニューや生成処理を追加。

---
###  How to Run / 実行方法

```bash
gcc sokoban_min_v2.c -o sokoban_min
./sokoban_min
```

---
###  Future Plans / 今後の予定

- Improve random map generator for balanced difficulty
    ランダム生成アルゴリズムの改良（難易度調整）

- Load stages from external files
    外部ファイルからのマップ読み込み対応

- Add step counter and retry feature
    手数カウンタ・リトライ機能の追加

---
## Future Ideas / 今後のアイデア

- Multi-stage level loading from file / 外部ファイルからのマップ読込
- Undo / restart feature / アンドゥ・リスタート機能
- Step counter and score system / 手数カウンタ・スコアシステム

---
## License / ライセンス

This project is released under the **MIT License**.
You can freely use, modify, and distribute it — just keep the license notice.

本プロジェクトは **MITライセンス** の下で公開されています。
ライセンス表記を残す限り、自由に使用・改変・再配布が可能です。

---
## Author / 作者

LAND – 情報工学科の大学生 / Hobbyist Engineer
Focus: Embedded systems, C/C++, and game logic on minimal environments.
