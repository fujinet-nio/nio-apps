# Writing a NIO application

A guide to the ideas, in the order you meet them. Each is shown in a small program in `examples/`; read the
program, this page only tells you which and why.

## 1. Start the library and check everything (`fujiinfo`)

`fn_init()` first, always; then check every call's `uint8_t` status (`FN_OK` is 0, anything else is an
`FN_ERR_*` code, `fn_error_string()` names it). A call may return `FN_ERR_UNSUPPORTED` because the adapter's
firmware is older than your library: treat that as "unknown" and carry on.

## 2. The network is asynchronous: NOT_READY is normal (`httpget`)

When you `fn_open()` a URL the FujiNet starts fetching it and **returns at once**. `fn_info()` and `fn_read()`
answer `FN_ERR_NOT_READY` until there is something to give you; wait a little and ask again. This is true of
every kind of session and every call, and is the most important idea on the page.

- A session has a **handle**; always `fn_close()` it (the adapter has a small, fixed number of sessions).
- Reads and writes carry an **offset**: how many bytes of the body you have already read or written. Keep it
  yourself. Streams (TCP, TLS) need it sequential.
- The body ends when a read sets `FN_READ_EOF`. A read of fewer bytes than you asked is not the end.

## 3. Sending data (`httppost`)

POST and PUT have a body. Open with `FN_OPEN_BODY_UNKNOWN`, `fn_write()` the body (loop: it may accept fewer
bytes than you offered), then **commit it with a zero-length write**. Only then is the request sent, and until
then `fn_info()`/`fn_read()` are NOT_READY.

## 4. Let the FujiNet do the parsing (`jsonq`)

`fn_open_translated()` with `FN_TRANSLATE_JSON` and a JSON Pointer returns just the value you asked for, as
text. Cheap for an 8-bit machine; it needs the whole document downloaded first, so NOT_READY lasts longer.

## 5. A permanent connection (`tcppipe`)

A program that talks to one server for its whole run opens a `tcp://` (or `tls://`) session once and uses it
as a pipe. Open with `FN_OPEN_KEEP` so the adapter does not drop it for being quiet (it reaps idle sessions
after 20 minutes otherwise); `FN_OPEN_STREAM_NO_PROBE` if your own protocol frames the data. Then:

- A stream has no "end of response". Reads return what has arrived; NOT_READY means "nothing yet"; only
  `FN_READ_EOF` means the server closed.
- One read is not one message. Your protocol's framing says when you have a whole message.
- `fn_read_available()` (valid when the read returned `FN_READ_AVAILABLE`) says how much more the adapter
  already holds, so you know whether the next read will wait.

## 6. Waiting for more than one thing (`waitany`)

Do not spin on `fn_read()` across your sessions: every NOT_READY is a full exchange with the adapter.
`fn_ready()` lists *only the sessions that can be used now* (and what each can do and how many bytes it
holds) in one small call. Between asks, wait with `nio_hint_wait()`:

- **Atari:** the FujiNet can pulse the PROC line when a session becomes readable (enable it with
  `sio.tune proc 1` on the FujiNet console). `nio_hint_begin()` arms the interrupt and `nio_hint_wait()`
  returns the moment it fires, so the program uses no bus time while idle. Without the hardware (or with the
  setting off) it sleeps the whole interval.
- **Other machines:** it is a plain sleep; `fn_ready()` after it tells you what to do.

A hint is only a nudge to look: it can be early or for data something else took. `fn_ready()`/`fn_read()` are
the truth. A program that keeps calling `fn_ready()` is not idle to the adapter.

## 7. State that outlives the program (`astest`) and disks (`diskinfo`)

The app store keeps per-application key/value data on the FujiNet. The DiskDevice mounts disk images (from
the FujiNet's SD card, a web server, TNFS) into numbered slots that the machine sees as its drives.

## Running on each machine

- **Linux** is the fastest way to try an example: build `fujinet-nio` for POSIX and run it, then
  `FN_PORT=/dev/pts/N build/linux/bin/<program>` (see the README). A local httpbin and echo server come from
  `fujinet-nio/scripts/start_test_services.sh`.
- **Atari:** `make TARGET=atari disk` makes an ATR of every program (needs `dir2atr`); boot it from the
  FujiNet or an emulator. A program that would otherwise lose its output on return to the boot loader waits
  for a key at the end (`nio_exit_pause`).
- **MS-DOS, BBC, Amiga:** `make TARGET=... disk` (MS-DOS, BBC) or copy the Amiga executables from
  `build/amiga/<profile>/bin/`. The workspace `docs/` describe the emulator set-ups.

## Memory on the 8-bit machines

The 6502 targets have a few tens of KB. Use small buffers (`NIO_SMALL_MEMORY` is defined for them), keep
state in static variables rather than on the C stack, and give each library call a buffer you own (the app
store and Wi-Fi scan take caller-owned buffers for this reason). If a program does not fit, say so in a
`NOT_ON` file rather than fighting for the last bytes.
