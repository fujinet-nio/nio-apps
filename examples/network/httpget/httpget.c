/**
 * @file httpget.c
 * @brief Fetch a web page: the network session lifecycle in its smallest form.
 *
 * A network request in FujiNet-NIO is a *session*. You open it, ask it questions, read from it, and
 * close it. The same calls serve HTTP, HTTPS, TCP and TLS; the scheme in the URL picks which.
 *
 *     fn_open(&handle, method, url, flags)   start a session; returns at once, the request is in flight
 *     fn_info(handle, &status, &len, &flags)  the HTTP status and length, once the response has begun
 *     fn_read(handle, offset, buf, n, &got, &flags)   the body, a chunk at a time
 *     fn_close(handle)                        always, even after an error
 *
 * The idea this program is here to show: **FN_ERR_NOT_READY is not an error.** The FujiNet is fetching
 * the page over the network while your program runs. fn_info() and fn_read() answer FN_ERR_NOT_READY
 * until there is something to give you; the right response is to wait a moment and ask again. The same
 * rule holds for every call on every kind of session, so learn it here.
 *
 * Other things to notice:
 *
 *   - Reads carry an *offset*: the number of body bytes you have already taken. Keep it yourself and
 *     add what each read returned. (For TCP-like streams the offset must be sequential.)
 *   - The body ends when fn_read() sets FN_READ_EOF in its flags (it may also return zero bytes).
 *   - fn_read() with a buffer smaller than the data simply returns a chunk; loop for the rest.
 *   - FN_OPEN_FOLLOW_REDIR asks the FujiNet to follow HTTP redirects for you.
 *
 * Run: httpget [url]     (default: <httpbin>/get, see nio_example.h; start it with
 *                          fujinet-nio/scripts/start_test_services.sh http)
 * The 8-bit machines have no command line: set the address at build time, for example
 *     make TARGET=atari HTTPBIN_URL=http://192.168.1.86:8080
 */

#include "fujinet-nio.h"
#include "nio_example.h"

#include <stdio.h>
#include <string.h>

#ifdef NIO_SMALL_MEMORY
#define CHUNK 128
#else
#define CHUNK 512
#endif

/* Wait this long between asking again, and give up after this many tries (about 20 seconds). */
#define RETRY_PAUSE_MS 100
#define RETRY_LIMIT    200

static uint8_t chunk[CHUNK + 1];

int main(int argc, char **argv)
{
    char url[160];
    fn_handle_t handle;
    uint16_t status = 0;
    uint32_t length = 0;
    uint8_t info_flags = 0;
    uint32_t offset = 0;
    uint16_t got;
    uint8_t read_flags;
    uint16_t tries;
    uint8_t result;

    if (argc > 1) {
        strncpy(url, argv[1], sizeof url - 1);
        url[sizeof url - 1] = '\0';
    } else {
        strcpy(url, FN_DEFAULT_HTTPBIN_URL "/get");
    }

    result = fn_init();
    if (result != FN_OK) {
        printf("init: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    printf("GET %s\n", url);

    /* Start the session. This returns as soon as the FujiNet has the request; it does not wait for the
     * page. Method 0 would mean a raw TCP connection; FN_METHOD_GET asks for HTTP. */
    result = fn_open(&handle, FN_METHOD_GET, url, FN_OPEN_FOLLOW_REDIR);
    if (result != FN_OK) {
        printf("open: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    /* Wait for the response to begin: fn_info() says NOT_READY until the status line has arrived. */
    for (tries = 0; tries < RETRY_LIMIT; ++tries) {
        result = fn_info(handle, &status, &length, &info_flags);
        if (result != FN_ERR_NOT_READY) {
            break;
        }
        nio_sleep_ms(RETRY_PAUSE_MS);
    }
    if (result != FN_OK) {
        printf("info: %u (%s)\n", (unsigned) result, fn_error_string(result));
        fn_close(handle);
        return 1;
    }
    printf("HTTP %u", (unsigned) status);
    if (info_flags & FN_INFO_HAS_LENGTH) {
        printf(", %lu bytes", (unsigned long) length);
    }
    putchar('\n');

    /* Read the body. offset is how much we have taken so far; every read starts where the last one
     * stopped. NOT_READY means "the next bytes have not arrived yet", not "failed". */
    tries = 0;
    for (;;) {
        got = 0;
        read_flags = 0;
        result = fn_read(handle, offset, chunk, CHUNK, &got, &read_flags);
        if (result == FN_ERR_NOT_READY) {
            if (++tries >= RETRY_LIMIT) {
                puts("read: gave up waiting");
                break;
            }
            nio_sleep_ms(RETRY_PAUSE_MS);
            continue;
        }
        if (result != FN_OK) {
            printf("read: %u (%s)\n", (unsigned) result, fn_error_string(result));
            break;
        }
        tries = 0;
        chunk[got] = '\0';      /* the page is text; fwrite() would do for binary data */
        fputs((const char *) chunk, stdout);
        offset += got;
        if (read_flags & FN_READ_EOF) {
            break;              /* that was the last of it */
        }
    }
    printf("\n[%lu bytes read]\n", (unsigned long) offset);

    /* Always close: the FujiNet keeps a small, fixed number of sessions and a leaked one wastes a slot. */
    fn_close(handle);
    nio_exit_pause();
    return 0;
}
