/**
 * @file jsonq.c
 * @brief Let the FujiNet pick one value out of a JSON document, so an 8-bit machine does not have to.
 *
 * Parsing JSON takes more memory than many of the machines FujiNet serves have to spare, and a typical
 * web API returns far more than a program wants. So the FujiNet can do the parsing: ask it to
 * *translate* the response, and give it a JSON Pointer (RFC 6901) naming the value you want. What
 * comes back from fn_read() is just that value, as plain text.
 *
 *     fn_open_translated(&h, FN_METHOD_GET, url, flags, FN_TRANSLATE_JSON, 0, "/url")
 *
 * This asks httpbin's /get, which answers with a JSON object like {"args":{},"headers":{...},
 * "url":"http://.../get"}, and the pointer "/url" selects the "url" string. You can try others:
 * "/headers/Host", "/args". How the value is rendered (strings raw, numbers as text, objects as
 * key/value lines) is in fujinet-nio's docs/network_device_protocol.md, "JSON serialization".
 *
 * What differs from an ordinary request (see httpget):
 *
 *   - While the FujiNet is still downloading and parsing the body, *both* fn_info() and fn_read()
 *     answer FN_ERR_NOT_READY. There is nothing to read until the whole document has been fetched.
 *   - The offset counts translated bytes, not bytes of JSON.
 *   - A bad pointer or malformed document comes back as an error from the open or the first read.
 *
 * Not available on the BBC (its translation goes through a different, ROM-based interface; see
 * fn_json_query in fujinet-nio.h): the build leaves it out there.
 *
 * Run: jsonq [base-url [json-pointer]]    (default: the httpbin address in nio_example.h, "/url")
 */

#include "fujinet-nio.h"
#include "nio_example.h"

#include <stdio.h>
#include <string.h>

#define RETRY_PAUSE_MS 100
#define RETRY_LIMIT    200

static uint8_t chunk[129];

int main(int argc, char **argv)
{
    char url[160];
    const char *pointer = argc > 2 ? argv[2] : "/url";
    fn_handle_t handle;
    uint32_t offset = 0;
    uint16_t got;
    uint8_t flags;
    uint16_t tries;
    uint8_t result;

    strcpy(url, argc > 1 ? argv[1] : FN_DEFAULT_HTTPBIN_URL);
    strcat(url, "/get");

    result = fn_init();
    if (result != FN_OK) {
        printf("init: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    printf("GET %s, picking %s\n", url, pointer);
    result = fn_open_translated(&handle, FN_METHOD_GET, url, 0, FN_TRANSLATE_JSON, 0, pointer);
    if (result != FN_OK) {
        printf("open: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    tries = 0;
    for (;;) {
        got = 0;
        flags = 0;
        result = fn_read(handle, offset, chunk, 128, &got, &flags);
        if (result == FN_ERR_NOT_READY) {          /* still fetching and parsing */
            if (++tries >= RETRY_LIMIT) {
                puts("gave up waiting");
                break;
            }
            nio_sleep_ms(RETRY_PAUSE_MS);
            continue;
        }
        if (result != FN_OK) {
            printf("read: %u (%s)\n", (unsigned) result, fn_error_string(result));
            break;
        }
        chunk[got] = '\0';
        fputs((const char *) chunk, stdout);
        offset += got;
        if (flags & FN_READ_EOF) {
            break;
        }
    }
    putchar('\n');

    fn_close(handle);
    nio_exit_pause();
    return 0;
}
