/*
 * httphead: HTTP request headers, a known-length POST and larger reads.
 *
 * Runs GET /headers then POST /post against httpbin. Both send a demonstration
 * Authorization header; POST also sends Content-Type and declares its body
 * length in fn_open_headers. Replace the example token in a real application.
 * Never print real tokens or embed them in source code.
 *
 * fn_read's buffer is application memory, not a packet size. On Amiga, Linux
 * and MS-DOS the 1024-byte buffer may be filled by multiple exchanges. A read
 * may return fewer bytes: keep the offset and retry NOT_READY/BUSY. On Atari
 * the smaller buffer fits the machine. BBC's ROM-backed open cannot supply
 * arbitrary headers and is excluded from this example.
 *
 * Run: httphead [httpbin-base-url]; defaults to FN_DEFAULT_HTTPBIN_URL.
 */
#include "fujinet-nio.h"
#include "nio_example.h"
#include <stdio.h>
#include <string.h>

#ifdef NIO_SMALL_MEMORY
#define CHUNK 128
#else
#define CHUNK 1024
#endif
#define RETRIES 200
#define PAUSE_MS 100

static uint8_t chunk[CHUNK];
static const char body[] = "{\"source\":\"httphead\"}";
static const fn_http_header_t headers[] = {
    {"Authorization", "Bearer example-token"},
    {"Content-Type", "application/json"}
};

static uint8_t temporary(uint8_t rc)
{
    return rc == FN_ERR_NOT_READY || rc == FN_ERR_BUSY;
}

static int request(const char *url, uint8_t method)
{
    fn_handle_t handle;
    uint16_t n, status, attempts;
    uint32_t offset, length;
    uint8_t rc, flags;
    uint16_t body_len = method == FN_METHOD_POST ? (uint16_t)strlen(body) : 0;

    /* A nonzero body length tells the firmware when to send the POST. */
    rc = fn_open_headers(&handle, method, url, 0, headers,
                         (uint16_t)(sizeof headers / sizeof headers[0]), body_len);
    if (rc != FN_OK) { printf("open: %u\n", (unsigned)rc); return 1; }
    offset = 0;
    while (offset < body_len) {
        n = 0;
        rc = fn_write(handle, offset, (const uint8_t *)body + offset,
                      (uint16_t)(body_len - offset), &n);
        if (rc != FN_OK || n == 0) { puts("write failed"); fn_close(handle); return 1; }
        offset += n;
    }
    /* No zero-length commit is needed: the known body length completes it. */
    rc = FN_ERR_NOT_READY;
    for (attempts = 0; attempts < RETRIES && temporary(rc); ++attempts) {
        rc = fn_info(handle, &status, &length, &flags);
        if (temporary(rc)) nio_sleep_ms(PAUSE_MS);
    }
    if (rc != FN_OK || !(flags & FN_INFO_HAS_STATUS)) {
        puts("HTTP status unavailable"); fn_close(handle); return 1;
    }
    printf("HTTP %u\n", (unsigned)status);
    if (status < 200 || status >= 300) { fn_close(handle); return 1; }

    offset = 0;
    attempts = 0;
    for (;;) {
        n = 0; flags = 0;
        rc = fn_read(handle, offset, chunk, CHUNK, &n, &flags);
        if (temporary(rc) || (rc == FN_OK && !n && !(flags & FN_READ_EOF))) {
            if (++attempts >= RETRIES) { puts("read timed out"); fn_close(handle); return 1; }
            nio_sleep_ms(PAUSE_MS);
            continue;
        }
        if (rc != FN_OK) { printf("read: %u\n", (unsigned)rc); fn_close(handle); return 1; }
        attempts = 0;
        /* Count bytes rather than printing httpbin's echo of the token. */
        offset += n;
        if (flags & FN_READ_EOF) break;
    }
    printf("\n%lu bytes\n", (unsigned long)offset);
    fn_close(handle);
    return 0;
}

int main(int argc, char **argv)
{
    char url[160];
    const char *base = argc > 1 ? argv[1] : FN_DEFAULT_HTTPBIN_URL;
    int result;
    if (strlen(base) + sizeof "/headers" > sizeof url) { puts("URL too long"); return 1; }
    if (fn_init() != FN_OK) { puts("init failed"); return 1; }
    strcpy(url, base); strcat(url, "/headers");
    puts("GET with Authorization");
    result = request(url, FN_METHOD_GET);
    if (!result) {
        strcpy(url, base); strcat(url, "/post");
        puts("POST with Authorization and JSON body");
        result = request(url, FN_METHOD_POST);
    }
    fn_shutdown();
    nio_exit_pause();
    return result;
}
