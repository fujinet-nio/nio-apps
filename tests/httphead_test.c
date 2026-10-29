#include <stdio.h>
#include <string.h>
#include "fujinet-nio.h"

int httphead_main(int argc, char **argv);
static unsigned opens, closes, writes, sleeps, shutdowns, reads;

uint8_t fn_init(void) { return FN_OK; }
void fn_shutdown(void) { ++shutdowns; }
void nio_sleep_ms(uint16_t ms) { if (ms) ++sleeps; }
void nio_exit_pause(void) {}

uint8_t fn_open_headers(fn_handle_t *h, uint8_t method, const char *url, uint8_t flags,
                        const fn_http_header_t *headers, uint16_t count, uint32_t length)
{
    ++opens;
    if (flags || count != 2 || strcmp(headers[0].name, "Authorization") ||
        strcmp(headers[0].value, "Bearer example-token") || strcmp(headers[1].name, "Content-Type") ||
        strcmp(headers[1].value, "application/json")) return FN_ERR_INVALID;
    if (opens == 1 && (method != FN_METHOD_GET || length || strcmp(url, "http://fixture/headers"))) return FN_ERR_INVALID;
    if (opens == 2 && (method != FN_METHOD_POST || length != strlen("{\"source\":\"httphead\"}") ||
                       strcmp(url, "http://fixture/post"))) return FN_ERR_INVALID;
    *h = (fn_handle_t)opens;
    return FN_OK;
}

uint8_t fn_write(fn_handle_t h, uint32_t offset, const uint8_t *data, uint16_t len, uint16_t *written)
{
    const char *expected = "{\"source\":\"httphead\"}";
    if (h != 2 || offset || len != strlen(expected) || memcmp(data, expected, len)) return FN_ERR_INVALID;
    ++writes; *written = len; return FN_OK;
}

uint8_t fn_info(fn_handle_t h, uint16_t *status, uint32_t *length, uint8_t *flags)
{
    if (h != opens) return FN_ERR_INVALID;
    *status = 200; *length = 1024; *flags = FN_INFO_HAS_STATUS | FN_INFO_HAS_LENGTH;
    return FN_OK;
}

uint8_t fn_read(fn_handle_t h, uint32_t offset, uint8_t *buf, uint16_t max_len, uint16_t *got, uint8_t *flags)
{
    ++reads;
    if (h != opens || max_len != 1024) return FN_ERR_INVALID;
    if (!offset && reads == 1) return FN_ERR_NOT_READY;
    if (!offset) { memset(buf, 'a', 1002); *got = 1002; *flags = 0; return FN_OK; }
    if (offset == 1002) { memset(buf, 'b', 22); *got = 22; *flags = FN_READ_EOF; return FN_OK; }
    return FN_ERR_INVALID;
}

uint8_t fn_close(fn_handle_t h)
{
    if (h != opens) return FN_ERR_INVALID;
    ++closes; return FN_OK;
}

int main(void)
{
    char *argv[] = {"httphead", "http://fixture", NULL};
    int rc = httphead_main(2, argv);
    if (rc || opens != 2 || closes != 2 || writes != 1 || shutdowns != 1 || sleeps != 1 || reads != 5) {
        fprintf(stderr, "httphead test failed: rc=%d opens=%u closes=%u writes=%u shutdowns=%u sleeps=%u reads=%u\n",
                rc, opens, closes, writes, shutdowns, sleeps, reads);
        return 1;
    }
    return 0;
}
