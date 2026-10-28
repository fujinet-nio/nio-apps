/* The hardware hint on a machine that has none: waiting is just sleeping, and the caller then asks
 * the adapter. Used for every target except those with their own nio_hint.c. */
#include "nio_example.h"

uint8_t nio_hint_begin(void)
{
    return 0;
}

void nio_hint_wait(uint16_t max_ms)
{
    nio_sleep_ms(max_ms);
}

void nio_hint_end(void)
{
}
