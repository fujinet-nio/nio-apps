/* BBC Micro (cc65): the OS clock through clock() and the keyboard. cc65's time.h does not give the BBC a
 * CLOCKS_PER_SEC; the system clock it reads counts centiseconds. */
#include <conio.h>
#include <time.h>

#include "nio_example.h"

#define BBC_CLOCKS_PER_SEC 100UL

void nio_sleep_ms(uint16_t ms)
{
    const clock_t end = clock() + ((clock_t) ms * BBC_CLOCKS_PER_SEC + 999UL) / 1000UL;

    while (clock() < end) {
    }
}

void nio_exit_pause(void)
{
    /* the output stays on screen */
}

uint8_t nio_key_pressed(void)
{
    if (kbhit()) {
        (void) cgetc();
        return 1;
    }
    return 0;
}
