/* MS-DOS (Open Watcom): delay() and the BIOS keyboard. */
#include <conio.h>
#include <dos.h>

#include "nio_example.h"

void nio_sleep_ms(uint16_t ms)
{
    delay(ms);
}

void nio_exit_pause(void)
{
    /* the output stays on screen */
}

uint8_t nio_key_pressed(void)
{
    if (kbhit()) {
        (void) getch();
        return 1;
    }
    return 0;
}
