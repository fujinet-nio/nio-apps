# nio-apps

How to write programs for FujiNet-NIO: short, heavily commented examples of the library, one folder per
device, that build unchanged for every machine the library supports (Atari, BBC Micro, MS-DOS, Amiga and
Linux). Start here if you are writing an application; read the example for the thing you want to do, then
copy it.

Product utilities (`fapp`, `fmount`, `fls`, ...) are in `../nio-core-apps`, the `config-nio` application is
in `../nio-config`, and the library itself is `../fujinet-nio-lib`. This repository is for learning,
smoke-testing and diagnostics.

## What is in it

```
examples/            portable C against fujinet-nio-lib: read these
  <device>/          one folder per FujiNet device (clock, network, wifi, ...)
    README.md        what the device is for and which example shows what
    <example>/       one program per folder, <example>/<program>.c, documented at the top
diagnostics/         tools about one machine or driver (Amiga DOS internals, Atari raw SIO, MS-DOS probes)
platform/            the few machine services the examples use, implemented once per machine
docs/                architecture, and a short guide to writing an app
makefiles/           the build, per target and per compiler
tools/               helper scripts (disk image creation)
```

The rule that keeps the examples readable: **an example contains no machine-specific code.** It uses the
C library and `fujinet-nio.h`. When it needs something no portable code can give it (wait a moment, notice
a key press, use a hardware "something is ready" signal) it calls `platform/include/nio_example.h`, and the
build links that machine's few lines of glue. See `docs/ARCHITECTURE.md`.

## The examples

| Device | Program | What it shows |
|---|---|---|
| fuji | `fujiinfo` | Start the library, ask the adapter what it is, cope with older firmware. The smallest program. |
| clock | `clock` | Read and set the FujiNet clock; time formats and time zones. |
| wifi | `wifitest` | Read Wi-Fi status and configuration and scan, with caller-owned buffers. Read-only. |
| wifi | `wificonfigtest` | End-to-end Wi-Fi configuration test. |
| appstore | `astest` | The per-application key/value store that lives on the FujiNet. Self-checking. |
| network | `httpget` | A web request from start to finish. **`FN_ERR_NOT_READY` is not an error.** Read this first. |
| network | `httppost` | Send a request body; committing it with a zero-length write. |
| network | `jsonq` | Have the FujiNet pull one value out of a JSON document for you. |
| network | `tcppipe` | A permanent connection used as a pipe: `FN_OPEN_KEEP`, framing, `fn_read_available`. |
| network | `waitany` | Several sessions at once: `fn_ready`, and the hardware hint (Atari PROC) to wait without polling. |
| network | `fhttpbin` | A table-driven check of the HTTP client against httpbin. |
| disk | `diskinfo` | Mount a disk image from a URL, read its geometry and a sector, unmount. |

Where each builds:

| Program | Atari | BBC | MS-DOS | Amiga | Linux |
|---|:-:|:-:|:-:|:-:|:-:|
| `fujiinfo` `clock` `wifitest` `wificonfigtest` `astest` `httpget` `httppost` `fhttpbin` | yes | yes | yes | yes | yes |
| `jsonq` `tcppipe` `waitany` `diskinfo` | yes | no | yes | yes | yes |

(The BBC library has no DiskDevice client, `fn_ready` or `fn_read_available` yet, and its JSON translation
goes through a different interface. An example that does not fit a machine says so in a `NOT_ON` file in its
folder; `make` leaves it out there.)

`diagnostics/` is not a tutorial. It holds tools that are about one machine: `diagnostics/amiga/` (driver and
DOS-node tests run by the Amiberry integration suite), `diagnostics/atari/fsioraw` (one raw SIO command) and
`diagnostics/msdos/` (driver probes). Each file starts with what it is for.

## Build

Programs are built for one *target* at a time. They land flat in `build/<target>/bin/`, named after the
program (`.xex` for Atari, `.exe` for MS-DOS, plain for the others).

```sh
source ../../scripts/env.sh                 # toolchain paths (in the workspace)
make                                         # every configured target
make TARGET=atari      FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=bbc                              # also bbc-clib
make TARGET=msdos
make TARGET=linux
make amiga                                   # wb13, wb31 and wb32; or AMIGA_PROFILE=wb32 for one
make TARGET=atari httpget                    # just one program
make TARGET=atari disk                       # an ATR with all the programs (needs dir2atr); msdos, bbc likewise
```

The 8-bit machines have no command line, so the address of the test servers is compiled in. Point it at
your own PC:

```sh
make TARGET=atari HTTPBIN_URL=http://192.168.1.86:8080 TCP_URL=tcp://192.168.1.86:7777
```

Start the servers on that PC with `../fujinet-nio/scripts/start_test_services.sh http` (httpbin) and
`... tcp` (an echo server), and make sure its firewall lets the FujiNet connect.

## Run

- **Atari, BBC, MS-DOS, Amiga:** put the program on a disk the machine can start (`make ... disk`, or the
  boot disks in `../nio-core-apps`) with a FujiNet attached. See the platform notes in `docs/WRITING-AN-APP.md`.
- **Linux (the quickest way to try an example):** run a POSIX FujiNet-NIO (`../fujinet-nio`,
  `./build.sh -cp fujibus-pty-debug`, then run `build/fujibus-pty-debug/fujinet-nio`; it prints a
  `PtyChannel ... slave: /dev/pts/N` line) and point the program at that pseudo-terminal:

  ```sh
  FN_PORT=/dev/pts/20 build/linux/bin/httpget http://127.0.0.1:8080/get
  ```

  `FN_PORT` and `FN_BAUD` are read by the library's Linux transport.

## Adding an example

1. Make `examples/<device>/<name>/<name>.c`. The program is named after the file, and names must be unique
   across the whole repository (`make` stops if two clash; MS-DOS names are best kept to 8 characters).
2. Start the file with a comment that says what it shows, which calls it uses, and the one or two ideas to
   take from it. Write for someone who has not read the library's header.
3. Use `fujinet-nio.h` and the C library only. If you need a machine service, see whether
   `nio_example.h` has it; if not, add it to the header and to each `platform/<target>/` directory, and keep
   it as small as you can.
4. If it cannot build everywhere, put the targets it cannot build for (`atari bbc ...`) in a `NOT_ON` file
   beside it.
5. Add a row to the table above and to the device's `README.md`.
