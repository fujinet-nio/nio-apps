/**
 * @file httppost.c
 * @brief Send data to a web server: a POST with a request body.
 *
 * httpget showed the session lifecycle for a request with no body. A request that sends data (POST,
 * PUT) adds one step between fn_open() and reading the answer: you *write the body*.
 *
 *     fn_open(&h, FN_METHOD_POST, url, FN_OPEN_BODY_UNKNOWN)   start; the FujiNet holds the request back
 *     fn_write(h, offset, data, len, &written)                 send the body, a chunk at a time
 *     fn_write(h, offset, NULL, 0, &written)                   a zero-length write ends the body
 *     fn_info() / fn_read()                                    the response, exactly as in httpget
 *
 * Why FN_OPEN_BODY_UNKNOWN: the FujiNet has to know when the body is finished before it sends the
 * request. If you know the length in advance a platform can declare it at open (on the BBC, see
 * fn_set_body_length); otherwise you open with this flag and *commit* the body by writing zero bytes.
 * Until the body is committed, fn_info() and fn_read() answer FN_ERR_NOT_READY, as the response cannot
 * exist yet.
 *
 * Things to notice:
 *
 *   - fn_write() takes the offset of the bytes within the body, like fn_read() does within the
 *     response: keep a running total and pass it each time. Offsets must be sequential.
 *   - fn_write() reports how many bytes it accepted. The library retries a busy adapter for you,
 *     but it can still accept fewer than you offered: loop until the whole buffer is sent.
 *   - The server here is httpbin's /post, which echoes back what it received as JSON, so you can see
 *     the data arrive.
 *
 * Run: httppost [base-url]    (default: the httpbin address in nio_example.h)
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

#define RETRY_PAUSE_MS 100
#define RETRY_LIMIT    200

static const char body[] = "{\"from\":\"httppost\",\"numbers\":[1,2,3]}";
static uint8_t chunk[CHUNK + 1];

/* Ask `step` again until it stops answering NOT_READY (with a pause between, and a limit). */
#define WAIT_WHILE_NOT_READY(result, call) \
    do { \
        uint16_t tries_; \
        for (tries_ = 0; tries_ < RETRY_LIMIT; ++tries_) { \
            (result) = (call); \
            if ((result) != FN_ERR_NOT_READY) break; \
            nio_sleep_ms(RETRY_PAUSE_MS); \
        } \
    } while (0)

int main(int argc, char **argv)
{
    char url[160];
    fn_handle_t handle;
    uint16_t status = 0;
    uint32_t length = 0;
    uint8_t info_flags = 0;
    uint32_t sent = 0;
    uint32_t offset = 0;
    uint16_t written;
    uint16_t got;
    uint8_t read_flags;
    uint8_t result;
    const uint16_t body_len = (uint16_t) strlen(body);

    strcpy(url, argc > 1 ? argv[1] : FN_DEFAULT_HTTPBIN_URL);
    strcat(url, "/post");

    result = fn_init();
    if (result != FN_OK) {
        printf("init: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    printf("POST %s\n", url);
    result = fn_open(&handle, FN_METHOD_POST, url, FN_OPEN_BODY_UNKNOWN);
    if (result != FN_OK) {
        printf("open: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    /* Send the body. */
    while (sent < body_len) {
        written = 0;
        result = fn_write(handle, sent, (const uint8_t *) body + sent, (uint16_t) (body_len - sent), &written);
        if (result != FN_OK) {
            printf("write: %u (%s)\n", (unsigned) result, fn_error_string(result));
            fn_close(handle);
            return 1;
        }
        sent += written;
    }

    /* Commit: a write of zero bytes says "that is the whole body". Only now is the request sent. */
    result = fn_write(handle, sent, NULL, 0, &written);
    if (result != FN_OK) {
        printf("commit: %u (%s)\n", (unsigned) result, fn_error_string(result));
        fn_close(handle);
        return 1;
    }
    printf("sent %lu body bytes\n", (unsigned long) sent);

    /* From here it is the same as httpget: wait for the status, then read the body. */
    WAIT_WHILE_NOT_READY(result, fn_info(handle, &status, &length, &info_flags));
    if (result != FN_OK) {
        printf("info: %u (%s)\n", (unsigned) result, fn_error_string(result));
        fn_close(handle);
        return 1;
    }
    printf("HTTP %u\n", (unsigned) status);

    for (;;) {
        got = 0;
        read_flags = 0;
        WAIT_WHILE_NOT_READY(result, fn_read(handle, offset, chunk, CHUNK, &got, &read_flags));
        if (result != FN_OK) {
            printf("read: %u (%s)\n", (unsigned) result, fn_error_string(result));
            break;
        }
        chunk[got] = '\0';
        fputs((const char *) chunk, stdout);
        offset += got;
        if (read_flags & FN_READ_EOF) {
            break;
        }
    }
    putchar('\n');

    fn_close(handle);
    nio_exit_pause();
    return 0;
}
