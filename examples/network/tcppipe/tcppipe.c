/**
 * @file tcppipe.c
 * @brief A permanent connection used as a pipe: write a request, read the reply, keep the line open.
 *
 * Some programs talk to one server for as long as they run (a game client, a chat, a monitor). Opening
 * and closing a connection for every message would be slow, so they open it once, use it as a pipe,
 * and close it when they exit. This program does that against an echo server, and shows what is
 * specific to that way of working.
 *
 *     fn_open(&h, 0, "tcp://host:port", FN_OPEN_STREAM_NO_PROBE | FN_OPEN_KEEP)
 *
 *   method 0 / tcp://   a raw byte stream: no HTTP, no status, no length. (tls:// is the same, encrypted.)
 *   FN_OPEN_KEEP        "keep this session until I close it". The FujiNet drops a session that has seen
 *                       no request for a long time (20 minutes) so that a program which has gone away
 *                       does not hold a handle for ever. A pipe that may sit quiet for long periods says
 *                       so with this flag and is left alone. Older firmware ignores the flag.
 *   FN_OPEN_STREAM_NO_PROBE
 *                       "the stream frames itself": the host will not probe for more data after each
 *                       chunk. Use it when your own protocol says how much to expect.
 *
 * Then, for each message:
 *
 *     fn_write(h, sent_so_far, data, len, &written)    sequential offsets, as for a POST body
 *     fn_read(h, taken_so_far, buf, n, &got, &flags)   NOT_READY = nothing has arrived *yet*
 *
 * and these are the points worth taking from it:
 *
 *   - A stream has no "end of response". A read returns whatever has arrived; zero bytes with
 *     FN_ERR_NOT_READY means the server has not said anything yet. Only FN_READ_EOF means the server
 *     closed the connection.
 *   - A reply can arrive in pieces. The program here knows a reply is one line, so it keeps reading
 *     until it has the newline. Your own protocol's framing (a length prefix, a terminator) decides
 *     when you have a whole message. Never assume one read is one message.
 *   - fn_read_available() (valid when the read's flags include FN_READ_AVAILABLE) says how many more
 *     bytes the adapter is already holding for you. When it is non-zero the rest of the reply is
 *     already there and the next read will not have to wait.
 *
 * Run: tcppipe [tcp://host:port]    (default: the address in nio_example.h; start an echo server with
 *                                     fujinet-nio/scripts/start_test_services.sh tcp)
 */

#include "fujinet-nio.h"
#include "nio_example.h"

#include <stdio.h>
#include <string.h>

#define ROUNDS         5
#define LINE_MAX_BYTES 64
#define RETRY_PAUSE_MS 50
#define RETRY_LIMIT    200      /* about ten seconds */

static uint8_t reply[LINE_MAX_BYTES + 1];

int main(int argc, char **argv)
{
    char url[128];
    char message[32];
    fn_handle_t handle;
    uint32_t sent = 0;       /* bytes written so far: the next write's offset */
    uint32_t taken = 0;      /* bytes read so far: the next read's offset */
    uint16_t written;
    uint16_t got;
    uint8_t flags;
    uint16_t tries;
    uint16_t have;           /* bytes of the current reply collected */
    uint8_t round;
    uint8_t result;
    int failed = 0;

    strncpy(url, argc > 1 ? argv[1] : FN_DEFAULT_TCP_URL, sizeof url - 1);
    url[sizeof url - 1] = '\0';

    result = fn_init();
    if (result != FN_OK) {
        printf("init: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    printf("pipe to %s\n", url);
    result = fn_open(&handle, 0, url, FN_OPEN_STREAM_NO_PROBE | FN_OPEN_KEEP);
    if (result != FN_OK) {
        printf("open: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    for (round = 1; round <= ROUNDS && !failed; ++round) {
        /* Request: one line. */
        sprintf(message, "ping %u\n", (unsigned) round);
        written = 0;
        result = fn_write(handle, sent, (const uint8_t *) message, (uint16_t) strlen(message), &written);
        if (result != FN_OK) {
            printf("write: %u (%s)\n", (unsigned) result, fn_error_string(result));
            failed = 1;
            break;
        }
        sent += written;

        /* Reply: read until the newline that ends it. The connection stays open between rounds. */
        have = 0;
        tries = 0;
        while (have < LINE_MAX_BYTES) {
            got = 0;
            flags = 0;
            result = fn_read(handle, taken, reply + have, (uint16_t) (LINE_MAX_BYTES - have), &got, &flags);
            if (result == FN_ERR_NOT_READY) {
                /* Nothing yet: the server has not answered. Wait and ask again. */
                if (++tries >= RETRY_LIMIT) {
                    puts("no reply");
                    failed = 1;
                    break;
                }
                nio_sleep_ms(RETRY_PAUSE_MS);
                continue;
            }
            if (result != FN_OK) {
                printf("read: %u (%s)\n", (unsigned) result, fn_error_string(result));
                failed = 1;
                break;
            }
            tries = 0;
            taken += got;
            have += got;
            reply[have] = '\0';
            if (flags & FN_READ_EOF) {
                puts("the server closed the connection");
                failed = 1;
                break;
            }
            if (strchr((const char *) reply, '\n') != NULL) {
                break;      /* a whole reply */
            }
            /* Part of a reply. If the adapter says the rest is already here, the next read will not wait. */
        }
        if (!failed) {
            char *eol = strchr((char *) reply, '\n');
            if (eol != NULL) {
                *eol = '\0';           /* show the line without its terminator */
            }
            if (flags & FN_READ_AVAILABLE) {
                printf("round %u: %s (adapter holds %u more bytes)\n", (unsigned) round, (char *) reply,
                       (unsigned) fn_read_available());
            } else {
                printf("round %u: %s\n", (unsigned) round, (char *) reply);
            }
        }
    }

    /* Only the program decides when a pipe ends. */
    fn_close(handle);
    nio_exit_pause();
    return failed ? 1 : 0;
}
