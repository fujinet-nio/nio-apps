/*
 * nio_example.h - what an example needs from the machine it runs on.
 *
 * The examples under examples/ are plain C against fujinet-nio-lib and the C library, so the same
 * source builds for every target. The few things a program cannot get from those (wait a little,
 * notice a key press, use a hardware "something is ready" signal if the machine has one) are
 * declared here and implemented once per machine in platform/<target>/, which the Makefile compiles
 * in. Nothing in an example says which machine it is on.
 *
 * Keep this header small. An example that needs more is probably a diagnostic for one platform and
 * belongs under diagnostics/<target>/ instead.
 */
#ifndef NIO_EXAMPLE_H
#define NIO_EXAMPLE_H

#include <stdint.h>

/* The machine class, for the one thing sources legitimately vary on: how much memory they can spend on
 * buffers. Defined for the 8-bit cc65 targets (Atari, BBC). Prefer a smaller buffer under this to an
 * #ifdef on a compiler name in the example itself. */
#if defined(__CC65__)
#define NIO_SMALL_MEMORY 1
#endif

/* Where the examples look for their test servers when no address is given on the command line (the
 * 8-bit machines have no command line). Override at build time, for example
 *   make TARGET=atari HTTPBIN_URL=http://192.168.1.86:8080 TCP_URL=tcp://192.168.1.86:7777
 * Start the servers with fujinet-nio's scripts/start_test_services.sh (http and tcp). */
#ifndef FN_DEFAULT_HTTPBIN_URL
#define FN_DEFAULT_HTTPBIN_URL "http://127.0.0.1:8080"
#endif
#ifndef FN_DEFAULT_TCP_URL
#define FN_DEFAULT_TCP_URL "tcp://127.0.0.1:7777"
#endif

/* Wait about `ms` milliseconds (to the resolution the machine's clock allows: 20 ms on a 50 Hz
 * machine). Used between polls of a network session so the loop does not hammer the adapter. */
void nio_sleep_ms(uint16_t ms);

/* Non-zero if a key has been pressed since the last call (the key is consumed). A way for a polling
 * loop to let the user stop it. On a line-buffered console (Linux, Amiga CLI) it is Return. */
uint8_t nio_key_pressed(void);

/* Let the user read the output before the program ends, on a machine where leaving the program clears
 * it (an Atari booted from a FujiNet disk returns to the boot loader). Waits for a key there; does
 * nothing where the output stays on screen. Call it last, before returning from main(). */
void nio_exit_pause(void);

/*
 * The hardware hint.
 *
 * Some machines can be told by the FujiNet that something is ready to read without asking it. The
 * Atari's PROC line is one: the adapter pulses it and an interrupt handler records that. A program
 * that wants to wait for data uses these three calls, and works the same on a machine without the
 * signal: there the "hint" is just a sleep, and the program asks the adapter (fn_ready) after it.
 *
 *   hinted = nio_hint_begin();   non-zero when the machine has a hint and it is armed
 *   nio_hint_wait(ms);           returns when hinted or after `ms`, whichever is first
 *   nio_hint_end();              disarm
 *
 * A hint is only a nudge to look: it can arrive early, or for data something else already took.
 * Always check with fn_ready()/fn_read() and treat FN_ERR_NOT_READY as "nothing yet". Call
 * nio_hint_begin() after fn_init().
 */
uint8_t nio_hint_begin(void);
void nio_hint_wait(uint16_t max_ms);
void nio_hint_end(void);

#endif /* NIO_EXAMPLE_H */
