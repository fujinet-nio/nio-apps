# nio-apps architecture

`nio-apps` is the "here is how to write a NIO application" repository, plus a small set of diagnostics.
It is not the home of product utilities (`nio-core-apps`), the configuration application (`nio-config`) or
the library (`fujinet-nio-lib`).

## The layers

```
  examples/<device>/<example>/<program>.c      what you read and copy
        |                      |
        |  fujinet-nio.h       |  nio_example.h
        v                      v
  ../fujinet-nio-lib      platform/include/nio_example.h  <- declared once, neutral
  (every machine's                     |
   transport inside)                   v
                          platform/<target>/*.c            <- implemented per machine, linked at build time
```

- **Examples** are ordinary C. They call the library and the C library and nothing else, so one source file
  builds for every target. They do not test which machine they are on.
- **The library** (`fujinet-nio-lib`) already hides how each machine reaches the FujiNet (serial, the Atari
  SIO bus, an Amiga device, the BBC fn-rom). Use it for everything that touches the adapter. There is no
  other path to the adapter in this repository except the explicitly low-level diagnostics.
- **`nio_example.h`** is the narrow seam for the handful of things the library cannot provide and portable C
  cannot either: sleeping, noticing a key press, pausing at exit on a machine that clears the screen, and the
  hardware "something is ready" hint. Each target has a small `platform/<target>/nio_example.c`. Only the
  Atari has a real hint (`platform/atari/nio_hint.c`, the PROC line); everywhere else the hint is a sleep
  (`platform/common/nio_hint_poll.c`), so an example written against the hint works on every machine.
  Keep the header small: the more it grows, the less an example teaches.
- **Machine class.** `NIO_SMALL_MEMORY` (defined in the header for the 8-bit cc65 targets) is for the one
  thing sources do vary on, the size of their buffers. Prefer it to testing a compiler name.

The one remaining place an example may differ by machine is a *data type the library itself defines
differently*: `FN_TIME_T` is a 64-bit integer where there is one and an 8-byte structure on cc65. The `clock`
example isolates that in one documented helper.

## Only link what is used

Memory on an 8-bit machine is precious, so the build links the platform glue only into a program that
calls it: a program that mentions `nio_sleep_ms`, `nio_key_pressed`, `nio_exit_pause` or `nio_hint_*` gets
`platform/<target>/nio_example.c`, and one that calls `nio_hint_*` also gets the hint file. On the Atari that
is what keeps the PROC interrupt code out of every program except those that ask for it. Do not give examples
shared helper files that they would all link.

## Where a program goes

| It is... | It goes in |
|---|---|
| a "how to" for one library feature, buildable on every (or most) machines | `examples/<device>/<name>/` |
| about one machine, driver or OS internals, or a reproduction for a bug | `diagnostics/<target>/` |
| an Amiga diagnostic that needs Workbench 2 or later | `diagnostics/amiga/wb2/` (left out of the 1.3 profile) |
| a user-facing utility or part of a boot disk | `../nio-core-apps` |
| the configuration application | `../nio-config` |

## Build rules (makefiles/build.mk)

- One program per `.c` file under `examples/*/*/` or `diagnostics/<target>/`; the program is named after
  the file; all programs of a target go to `build/<target>/bin/`. Program names must be unique across the
  repository (the build stops otherwise). The workspace scripts and the Amiberry tests find binaries by name
  in those directories, so renaming a program is a cross-repository change.
- An example's folder may contain a `NOT_ON` file naming the targets it cannot build for. Nothing else is
  needed to include or exclude a program.
- Every program links `fujinet-nio-lib` for its target, except those named in `NO_NIO_LIB_PROGRAMS`.
- Amiga builds are always profile-qualified (`build/amiga/wb13|wb31|wb32/`); `make amiga` builds the driver
  support library first.

## History

This layout replaced a flat `apps/test/` directory with `src/platform/<target>/fnctl.c` and a shared
`fnsvc.c` that every program linked. Those were copies of the `nio-core-apps` code that drives the DOS-level
FujiNet control device (`fnctl`) and the F-utility service helpers (`fnsvc`); no example used them, but they
were linked into most programs anyway (7 to 10 KB of an Atari program, 7 to 11 KB of an Amiga one: `wifitest` on the Atari went from 18.4 KB to 9.8 KB), and
they are the opposite of "use the library where you can". They are gone from here and remain in
`nio-core-apps`, where the utilities that need them live.
