/* Linux: usleep() and standard input. The terminal is line buffered, so the "key" is Return. */
#define _DEFAULT_SOURCE   /* usleep() under -std=c99 */
#include <poll.h>
#include <unistd.h>

#include "nio_example.h"

void nio_sleep_ms(uint16_t ms)
{
    usleep((useconds_t) ms * 1000u);
}

void nio_exit_pause(void)
{
    /* the output stays on screen */
}

uint8_t nio_key_pressed(void)
{
    struct pollfd in;
    char buffer[64];

    in.fd = STDIN_FILENO;
    in.events = POLLIN;
    in.revents = 0;
    if (poll(&in, 1, 0) > 0 && (in.revents & POLLIN) != 0) {
        /* Zero bytes is end of input (stdin closed or redirected), not a key. */
        return read(STDIN_FILENO, buffer, sizeof buffer) > 0 ? 1 : 0;
    }
    return 0;
}
