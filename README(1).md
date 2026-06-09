# 2D Graphics Editor in C

A menu-driven 2D graphics editor implemented in C using a 2D character array as the drawing canvas. Comes in two versions: a plain terminal version and an interactive ncurses UI version.

## Features

- **Canvas**: 22 rows × 58 columns, background `_`, drawn with `*`
- **Shapes**:
  - Circle — Midpoint Circle Algorithm
  - Rectangle — 4 Bresenham lines
  - Line — Bresenham's Line Algorithm
  - Triangle — 3 connected Bresenham lines
- **Operations**: Add / Delete / Modify / List / Clear

## Two Versions

| Feature | Plain (`graphics_editor`) | ncurses (`graphics_editor_ncurses`) |
|---|---|---|
| Navigation | Type numbers | Arrow keys + Enter |
| Canvas update | Reprint each time | Live in-place |
| Layout | Scrolling text | Split panels |
| Menu | Numbered list | Highlighted selector |
| Colors | None | Green `*`, cyan highlight |

## Build & Run

```bash
make              # Build both
make run          # Run plain version
make run-ncurses  # Run ncurses version
make clean        # Remove binaries
```

Or manually:
```bash
# Plain version
gcc -Wall -std=c11 -o graphics_editor graphics_editor.c -lm
./graphics_editor

# ncurses version
gcc -Wall -std=c11 -o graphics_editor_ncurses graphics_editor_ncurses.c -lncurses -lm
./graphics_editor_ncurses
```

## ncurses UI Layout

```
┌─────────────┬──────────────────────────────────────────────────┐
│ 2D GRAPHICS │                    CANVAS                        │
│   EDITOR    │  ________________________________________________ │
│─────────────│  ________________________________________________ │
│ > Add Object│  ________________*****___________________________ │
│  Delete     │  ______________**_____**_________________________ │
│  Modify     │  _____________*_________*________________________ │
│  List       │  ...                                             │
│  Clear      │                                                  │
│  Exit       │                                                  │
│─────────────│                                                  │
│ ↑↓ navigate │                                                  │
│ ↵  select   │                                                  │
│ Objects: 2  │                                                  │
└─────────────┴──────────────────────────────────────────────────┘
 Status bar: messages appear here
```

## Implementation Details

- Objects stored in a struct array with `active` flag for soft-delete
- Re-render strategy: clear canvas, replay all active objects — ensures deleted/modified objects cleanly erase
- ncurses uses 4 windows: menu panel, canvas panel, status bar, floating input form

## File Structure

```
.
├── graphics_editor.c           # Plain terminal version
├── graphics_editor_ncurses.c   # ncurses interactive version
├── Makefile
└── README.md
```
