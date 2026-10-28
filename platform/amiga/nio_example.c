/* AmigaOS (clib2): dos.library Delay() (1/50 s) and WaitForChar() on the CLI's input. The console is line
 * buffered, so the "key" is Return. */
#include <proto/dos.h>

#include "nio_example.h"

void nio_sleep_ms(uint16_t ms)
{
    ULONG ticks = ((ULONG) ms + 19UL) / 20UL;

    if (ticks == 0) {
        ticks = 1;
    }
    Delay(ticks);
}

void nio_exit_pause(void)
{
    /* the output stays on screen */
}

uint8_t nio_key_pressed(void)
{
    BPTR in = Input();
    char buffer[64];

    if (in != 0 && WaitForChar(in, 0)) {
        (void) Read(in, buffer, sizeof buffer);
        return 1;
    }
    return 0;
}
