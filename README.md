# Sokoban Minimal (C / Terminal Edition)

A **minimal Sokoban** implementation written in **pure C**, designed to run on **UNIX-like** terminals such as WSL (zsh / bash).
No external libraries — just raw terminal control, ANSI escape sequences, and good old C logic.

---

## Features

- Runs entirely in your terminal
- Arrow keys or WASD for movement
- Real-time input using termios (raw mode)
- Simple ASCII graphics:
  - `@` → Player
  - `$` → Box
  - `. `→ Goal
  - `#` → Wall
  - `*` → Box on Goal
- Stage clear detection when all boxes reach goals

---

## Build & Run
### 1. Compile
```bash
gcc sokoban_min.c -o sokoban_min
```

### 2. Run
```bash
./sokoban_min
```

### 3. Controls
- Key	Action
  - ↑ / W	Move Up
  - ↓ / S	Move Down
  - ← / A	Move Left
  - → / D	Move Right
  - Q	Quit

---

## Technical Notes

- Implements non-canonical terminal input via <termios.h>
- Works in raw mode for instant key response
- Uses ANSI escape sequences to control screen clearing and cursor visibility
- Designed for clarity and learning — no dependencies, no frameworks
- Perfect for those who want to:
- Learn C-based terminal programming
- Understand game loops and real-time input
- Experiment with minimalist design philosophy

---

## File Structure
```
sokoban_min.c   # Main source file
README.md       # This documentation
LICENSE         # MIT License
```

---

## Future Ideas
- Multi-stage level loading from file
- Undo / restart feature
- Step counter and score system
- Map editor (CLI or GUI)

---

## License
This project is released under the MIT License.
You can freely use, modify, and distribute it — just keep the license notice.

---

## Author

LAND – 情報工学科の大学生 / Hobbyist Engineer
Focus: Embedded systems, C/C++, and game logic on minimal environments.
