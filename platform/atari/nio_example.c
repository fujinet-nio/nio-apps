/* Atari: the OS clock ("jiffies", 1/50 s on PAL, 1/60 s on NTSC) and the keyboard. */
#include <atari.h>
#include <conio.h>

#include "nio_example.h"

void nio_sleep_ms(uint16_t ms)
{
    const uint8_t rate = (get_tv() == AT_PAL) ? 50u : 60u;
    uint16_t jiffies = (uint16_t) (((uint32_t) ms * rate + 999u) / 1000u);
    uint8_t last = OS.rtclok[2];

    while (jiffies != 0) {
        const uint8_t now = OS.rtclok[2];
        if (now != last) {
            last = now;
            --jiffies;
        }
    }
}

void nio_exit_pause(void)
{
    cputs("press a key\r\n");
    (void) cgetc();
}

uint8_t nio_key_pressed(void)
{
    if (kbhit()) {
        (void) cgetc();
        return 1;
    }
    return 0;
}
