/**
 * @file fujiinfo.c
 * @brief The shortest FujiNet-NIO program: start the library, ask the adapter what it is.
 *
 * Every program that uses fujinet-nio-lib begins the same way, and this one does nothing else:
 *
 *   1. fn_init()      Start the library. Nothing else may be called before it. It opens the machine's
 *                     link to the FujiNet (a serial port, the Atari SIO bus, an Amiga device, ...), and
 *                     fails with an error code if there is no adapter to talk to.
 *   2. fn_is_ready()  Whether the adapter has finished starting (Wi-Fi up, services registered). A
 *                     program that needs the network should check it, or be ready for
 *                     FN_ERR_NOT_READY from the calls that follow.
 *   3. fn_fuji_get_info()   A call that involves no network, so it answers at once on any board.
 *
 * What to learn from the code:
 *
 *   - Every call returns a uint8_t status: FN_OK (0) or an FN_ERR_* code. fn_error_string() turns one
 *     into text. Check every call; a failure here usually means a loose cable.
 *   - A call can be FN_ERR_UNSUPPORTED because the adapter's firmware is older than the library.
 *     That is not a fault: show the value as unknown and carry on, as this program does. New
 *     library calls are added over time, and the firmware may not have caught up.
 *
 * Build: make TARGET=<atari|bbc|msdos|linux|amiga>   (see the README). No arguments, no server needed.
 */

#include "fujinet-nio.h"
#include "nio_example.h"

#include <stdio.h>

int main(void)
{
    fn_fuji_info_t info;
    uint8_t result;

    puts("FujiNet-NIO adapter info");

    /* 1. Start the library. */
    result = fn_init();
    if (result != FN_OK) {
        printf("init failed: %u (%s)\n", (unsigned) result, fn_error_string(result));
        nio_exit_pause();
        return 1;
    }

    /* 2. Is the adapter up? Not an error if not: just say so. */
    printf("ready: %s\n", fn_is_ready() ? "yes" : "not yet");

    /* 3. Firmware version and build profile. */
    result = fn_fuji_get_info(&info);
    if (result == FN_OK) {
        printf("firmware: %s\nprofile:  %s\n", info.firmware, info.profile);
    } else if (result == FN_ERR_UNSUPPORTED) {
        /* Older firmware: it simply does not know this command. */
        puts("firmware: unknown (older than this library)");
    } else {
        printf("info failed: %u (%s)\n", (unsigned) result, fn_error_string(result));
    }

    nio_exit_pause();
    return 0;
}
