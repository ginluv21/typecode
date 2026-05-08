# typecode

> A terminal-based typing trainer for programmers. Built in pure C with ncurses.

---

## What is typecode?

typecode is a TUI typing trainer designed specifically for programmers. Unlike generic typing tools, typecode trains you on real code — syntax, patterns, and constructs from the languages you actually use every day.

It feels like a real Linux terminal utility, not a toy. Think `btop`, `lazygit`, `htop` — but for improving your typing speed on code.

**What you train:**
- Blind typing (touch typing without looking at the keyboard)
- Fast input of programming syntax
- Muscle memory for language-specific constructs
- Special characters: `{}`, `[]`, `()`, `<>`, `;`, `->`, `=>`, `::`, `&&`, `||`

---

## Features

### MVP
- TUI main menu with keyboard navigation
- Typing engine with real-time feedback (green / red / yellow cursor)
- Live metrics: WPM, accuracy, error count
- Lessons loaded from plain text files
- 5 built-in lessons (home row, top row, bottom row, numbers, symbols)
- Results screen after each session

### v1.0
- 20 progressive lessons
- Programming Languages mode: C, Python, JavaScript, Bash, Go
- Load any custom file from the menu and type it
- Statistics saved between sessions (best WPM, history, accuracy)
- Hardcore mode — no Backspace allowed
- Practice modes: Time Attack (30 / 60 / 120s) and Infinite loop
- Settings screen with persistent config

### v2.0+ (planned)
- Keyboard heatmap — see which keys you struggle with most
- Cyrillic / Russian lessons
- Vim mode navigation (`hjkl`)
- Speed Challenge with ranks (S / A / B / C)
- Animated splash screen
- Man page

---

## Interface

### Main menu

```
╔══════════════════════════════════════════════╗
║                  typecode                   ║
╠══════════════════════════════════════════════╣
║  [1] Lessons                                ║
║  [2] Programming Languages                  ║
║  [3] Practice                               ║
║  [4] Statistics                             ║
║  [5] Settings                               ║
║  [0] Exit                                   ║
╚══════════════════════════════════════════════╝
```

### Typing session

```
╔══════════════════════════════════════════════╗
║ C Language Practice                [NORMAL] ║
╠══════════════════════════════════════════════╣

  for (int i = 0; i < n; i++) {
      printf("%d\n", i);
  }

  for (int i = 0; i <
                   ^

╠══════════════════════════════════════════════╣
║ WPM: 74   Accuracy: 97%   Errors: 2   0:42 ║
╚══════════════════════════════════════════════╝
```

- **Green** — correct characters
- **Red** — mistakes
- **Yellow** — current cursor position

### Results screen

```
╔══════════════════════════════════════════════╗
║              Session Complete               ║
╠══════════════════════════════════════════════╣
║  WPM        74                              ║
║  Accuracy   97%                             ║
║  Errors     2                               ║
║  Time       0:42                            ║
╠══════════════════════════════════════════════╣
║  [R] Retry    [Q] Main menu                 ║
╚══════════════════════════════════════════════╝
```

---

## Code examples you will type

**C**
```c
for (int i = 0; i < n; i++) {
    printf("%d\n", arr[i]);
}
```

**Python**
```python
if __name__ == "__main__":
    result = [x * 2 for x in range(10)]
```

**JavaScript**
```javascript
const result = arr.map(x => x * 2).filter(x => x > 5);
```

**Bash**
```bash
grep -r "main" src/ | awk '{print $1}'
```

**Go**
```go
func main() {
    ch := make(chan int, 10)
    go worker(ch)
}
```

---

## Installation

### Requirements

- Linux
- GCC
- ncurses (`libncurses-dev`)

### Build from source

```bash
git clone https://github.com/ginluv21/typecode.git
cd typecode
make
./typecode
```

### Install ncurses (if missing)

```bash
# Ubuntu / Debian
sudo apt install libncurses-dev

# Arch
sudo pacman -S ncurses

# Fedora
sudo dnf install ncurses-devel
```

---

## Project structure

```
typecode/
├── src/
│   ├── main.c          # Entry point, ncurses init
│   ├── ui.c / ui.h     # Drawing, colors, layout
│   ├── typing.c / .h   # Typing engine, input loop
│   ├── stats.c / .h    # Statistics, session saving
│   ├── lessons.c / .h  # Lesson loader, file parser
│   └── heatmap.c / .h  # Key frequency analysis
├── include/            # Shared headers
├── lessons/
│   ├── latin/          # Basic typing lessons
│   ├── c/              # C code snippets
│   ├── python/         # Python snippets
│   ├── javascript/     # JS snippets
│   ├── bash/           # Bash snippets
│   └── go/             # Go snippets
├── data/               # Runtime data (stats, config)
├── build/              # Compiled objects (gitignored)
├── Makefile
└── README.md
```

---

## Custom file mode

You can load any file from your system and type it:

```
Main menu → Practice → Load custom file
Enter path: /home/user/projects/myapp/main.c
```

typecode will load the file and let you type through it. Great for practicing on your own codebase.

---

## Makefile targets

```bash
make          # Build the project
make run      # Build and run
make debug    # Build with -g -fsanitize=address
make clean    # Remove build artifacts
```

---

## Keyboard shortcuts

| Key | Action |
|-----|--------|
| `↑` / `↓` | Navigate menu |
| `1`–`6` | Direct menu selection |
| `Enter` | Confirm |
| `Backspace` | Fix last character |
| `Esc` | Exit current session |
| `R` | Retry lesson |
| `Q` | Back to menu |

---

## Statistics

typecode tracks your progress between sessions:

- Best WPM ever
- Average WPM over last 10 sessions
- Average accuracy
- Per-lesson history

Stats are saved to `~/.typecode/stats.txt`.

---

## Hardcore mode

Enable in Settings. Backspace is disabled — every mistake counts and cannot be fixed. Forces you to slow down and think before you type.

---

## Tech stack

| | |
|---|---|
| Language | C (C11) |
| TUI | ncurses |
| Build | GCC + Makefile |
| Platform | Linux |
| Dependencies | libncurses only |

No C++. No heavy frameworks. No unnecessary dependencies.

---

## Roadmap

- [x] Project structure and GitHub issues
- [ ] MVP: core typing engine
- [ ] MVP: main menu
- [ ] MVP: 5 base lessons
- [ ] v1.0: 20 lessons + programming languages
- [ ] v1.0: statistics + custom file loading
- [ ] v1.0: hardcore mode + practice modes
- [ ] v2.0+: heatmap, cyrillic, vim mode

Full task list: [GitHub Project](https://github.com/users/ginluv21/projects/3)

---

## Contributing

This project is open source and beginner-friendly. Built by developers learning C.

1. Fork the repo
2. Create a branch: `git checkout -b feature/your-feature`
3. Commit your changes
4. Open a Pull Request

Check the [Issues](https://github.com/ginluv21/typecode/issues) for tasks labeled `MVP` or `v1.0`.

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---

*Built with ncurses and a desire to type faster.*
