# Connect 4 — Modern Edition

A polished Qt/C++ Connect 4 game with an alpha-beta AI opponent, animated UI, and built-in auto-updater.

![screenshot](Images/screenshot.png)

---

## Features

| Feature | Details |
|---|---|
| **AI opponent** | Alpha-beta minimax, depth-configurable via slider (2–8), center-column heuristic, immediate win/block detection |
| **Two-player mode** | Pass-and-play on the same machine |
| **Mid-game AI toggle** | Enable AI at any point in a PvP game — it picks up as the next Yellow move immediately |
| **Win overlay** | Animated overlay on the game page; winning pieces are highlighted, board stays visible |
| **Draw detection** | Detects and announces a full-board draw |
| **Score tracking** | Per-session score counters for both players |
| **Animated pieces** | Bouncing drop animation with `OutBounce` easing |
| **Floating logo** | Home-screen logo floats with layout-aware repositioning |
| **Auto-updater** | Checks GitHub Releases on startup, downloads the right platform asset, launches the installer |

---

## Building

### Requirements

- Qt 6.x **or** Qt 5.15+ (both supported)
- CMake ≥ 3.16
- A C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)

### Steps

```bash
git clone https://github.com/YOUR_GITHUB_USER/connect4.git
cd connect4
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Run:
```bash
./build/connect4          # Linux / macOS
build\connect4.exe        # Windows
```

---

## AI design

The AI is always **Player 2 (Yellow)**. It uses **alpha-beta pruning** on a minimax tree:

- **Depth** is set by the difficulty slider (2 = fast/easy, 8 = strong).
- **Move ordering** checks center columns first (`{2,3,1,4,0,5}`) to maximise pruning efficiency.
- **Immediate checks** before full search: win-in-one → block-in-one → full search.
- **Evaluation** scores every 4-cell horizontal/vertical/diagonal window across the board.
- **Terminal guards** detect already-won or full boards at the top of every node so the search never crashes on a nearly-full board.
- **Fallback move** ensures the AI always plays something valid even on a completely evaluated losing path.

### Known limits

- The AI does not detect **double-threat forks** at low depths (depth < 5). Increase the slider for stronger play.
- At depth 8 on a nearly-empty board the search can take 1–3 seconds; the UI remains responsive because computation runs on a dedicated `QThread`.

---

## License

MIT — see `LICENSE`.