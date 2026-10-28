/*
 * Atari: the PROC line as the hardware hint.
 *
 * If the FujiNet advertises it (Q capability bit, `sio.tune proc 1` on its console), nio_hint_begin
 * installs the library's VPRCED handler (fn_atari_proc_enable) and nio_hint_wait returns as soon as an
 * interrupt has been counted, instead of after the full timeout. Without it this behaves like the
 * other machines: a plain sleep. Either way the caller still asks the adapter what is ready.
 *
 * The library module is linked only because this file calls it, so a program that never calls
 * nio_hint_begin() pays nothing for PROC.
 */
#include <atari.h>

#include "fujinet-nio.h"
#include "nio_example.h"

static uint8_t armed;

uint8_t nio_hint_begin(void)
{
    armed = 0;
    if (fn_atari_query_capabilities() == FN_OK && (fn_atari_capability_flags() & FN_ATARI_CAP_PROC) != 0) {
        (void) fn_atari_proc_enable();
        armed = 1;
    }
    return armed;
}

void nio_hint_wait(uint16_t max_ms)
{
    uint16_t waited;

    if (!armed) {
        nio_sleep_ms(max_ms);
        return;
    }
    /* Look every few milliseconds rather than sleeping the whole time. */
    for (waited = 0; waited < max_ms; waited += 10) {
        if (fn_atari_proc_pending()) {
            return;
        }
        nio_sleep_ms(10);
    }
}

void nio_hint_end(void)
{
    if (armed) {
        fn_atari_proc_disable();
        armed = 0;
    }
}
