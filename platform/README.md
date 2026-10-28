# platform

The machine services the examples use, and nothing else. `include/nio_example.h` declares them (sleep,
key press, pause at exit, the hardware hint); each `<target>/` implements them in a few lines, and the build
links the implementation only into programs that call it. See `../docs/ARCHITECTURE.md`.

| File | Used by |
|---|---|
| `<target>/nio_example.c` | atari, bbc, msdos, linux, amiga: the clock and keyboard |
| `atari/nio_hint.c` | the Atari's PROC interrupt as the hardware hint |
| `common/nio_hint_poll.c` | every other target: the hint is just a sleep |

Add a function here only when an example truly cannot do without it, to the header and to every target.
