/**
 * @file waitany.c
 * @brief Several connections at once, and finding out which one has something for you.
 *
 * A program can hold more than one session open. The question this example answers is: how do I wait
 * for *whichever* one is ready without asking each in turn, and without doing nothing while I wait?
 * (The simple way, calling fn_read() on each in a loop, works but wastes the bus: every try that
 * answers FN_ERR_NOT_READY is a full exchange with the adapter.)
 *
 * The program opens two requests to httpbin that answer after different delays, so one finishes
 * before the other, and then waits for both using the library's "ready" call:
 *
 *     fn_ready(entries, max, &count)
 *
 *   One small call that lists *only the sessions that can be used now*, each with a flag saying what
 *   it can do (FN_READY_INFO: fn_info() will answer, for HTTP the status has arrived; FN_READY_READ:
 *   fn_read() will give data, or the end of the stream) and how many bytes the adapter holds for it.
 *   Your program can keep the screen up to date, wait a bit, ask again.
 *
 * And between asks, the hardware hint:
 *
 *     nio_hint_begin();  nio_hint_wait(ms);  nio_hint_end();
 *
 *   On an Atari the FujiNet can pulse the PROC line when something becomes ready, and the program is
 *   woken by that interrupt instead of sleeping out the whole timeout: it reacts at once, and until
 *   then it uses no bus time at all. (Switch it on with `sio.tune proc 1` on the FujiNet console.) On
 *   machines without such a signal nio_hint_wait() is just a sleep and the rest of the program is
 *   unchanged. A hint is only a nudge to *look*; it can come early, so the program always asks
 *   fn_ready() and treats the answer as the truth. See platform/atari/nio_hint.c for how little code
 *   the hardware side is.
 *
 * Things to notice:
 *
 *   - Handles come back in each entry: match them to your own bookkeeping.
 *   - A read that returns FN_READ_EOF is the end of that session: close it.
 *   - Old firmware answers fn_ready() with FN_ERR_UNSUPPORTED. The program then falls back to
 *     trying each open session, which is the fallback any client needs.
 *   - A client that keeps asking fn_ready() is not "idle" to the adapter, so its (non-keep) sessions
 *     are not dropped while it waits.
 *
 * Run: waitany [base-url]    (default: the httpbin address in nio_example.h)
 */

#include "fujinet-nio.h"
#include "nio_example.h"

#include <stdio.h>
#include <string.h>

#define SESSIONS     2
#define POLL_MS      250
#define WAIT_LIMIT   120          /* polls: about thirty seconds */
#define CHUNK        96

typedef struct {
    const char *name;
    const char *path;             /* what to ask httpbin for */
    fn_handle_t handle;
    uint8_t open;                 /* still a live session */
    uint8_t have_status;
    uint32_t offset;              /* bytes read so far */
} job_t;

static job_t jobs[SESSIONS] = {
    { "slow", "/delay/3", 0, 0, 0, 0 },
    { "fast", "/delay/1", 0, 0, 0, 0 }
};

static uint8_t chunk[CHUNK + 1];

static job_t *job_for(fn_handle_t handle)
{
    uint8_t i;
    for (i = 0; i < SESSIONS; ++i) {
        if (jobs[i].open && jobs[i].handle == handle) {
            return &jobs[i];
        }
    }
    return NULL;
}

/* Use what `job` has for us now: its status if known, then everything already readable. */
static void service(job_t *job, uint8_t what)
{
    uint16_t status;
    uint32_t length;
    uint8_t info_flags;
    uint16_t got;
    uint8_t flags;
    uint8_t result;

    if (!job->have_status && (what & FN_READY_INFO)) {
        result = fn_info(job->handle, &status, &length, &info_flags);
        if (result == FN_OK) {
            job->have_status = 1;
            printf("%s: HTTP %u\n", job->name, (unsigned) status);
        } else if (result != FN_ERR_NOT_READY) {
            printf("%s: info %u (%s)\n", job->name, (unsigned) result, fn_error_string(result));
            fn_close(job->handle);
            job->open = 0;
            return;
        }
    }

    if (what & FN_READY_READ) {
        for (;;) {
            got = 0;
            flags = 0;
            result = fn_read(job->handle, job->offset, chunk, CHUNK, &got, &flags);
            if (result == FN_ERR_NOT_READY) {
                break;                       /* nothing more for now: that is normal */
            }
            if (result != FN_OK) {
                printf("%s: read %u (%s)\n", job->name, (unsigned) result, fn_error_string(result));
                fn_close(job->handle);
                job->open = 0;
                return;
            }
            job->offset += got;
            if (flags & FN_READ_EOF) {
                printf("%s: finished, %lu bytes\n", job->name, (unsigned long) job->offset);
                fn_close(job->handle);
                job->open = 0;
                return;
            }
            if (got == 0) {
                break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    char url[160];
    fn_ready_entry_t ready[SESSIONS];
    uint8_t count;
    uint8_t i;
    uint8_t live;
    uint16_t waits;
    uint8_t hinted;
    uint8_t result;
    job_t *job;

    result = fn_init();
    if (result != FN_OK) {
        printf("init: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    /* Start both requests. Neither blocks: they are now in flight at the same time. */
    for (i = 0; i < SESSIONS; ++i) {
        strcpy(url, argc > 1 ? argv[1] : FN_DEFAULT_HTTPBIN_URL);
        strcat(url, jobs[i].path);
        result = fn_open(&jobs[i].handle, FN_METHOD_GET, url, 0);
        if (result != FN_OK) {
            printf("%s: open %u (%s)\n", jobs[i].name, (unsigned) result, fn_error_string(result));
            continue;
        }
        jobs[i].open = 1;
        printf("%s: %s\n", jobs[i].name, jobs[i].path);
    }

    hinted = nio_hint_begin();                    /* after fn_init() */
    printf("waiting%s\n", hinted ? " (woken by the hardware hint)" : "");

    for (waits = 0; waits < WAIT_LIMIT; ++waits) {
        live = 0;
        for (i = 0; i < SESSIONS; ++i) live += jobs[i].open;
        if (live == 0) {
            break;
        }

        nio_hint_wait(POLL_MS);                   /* a nap, or until the hardware says "look" */

        count = 0;
        result = fn_ready(ready, SESSIONS, &count);
        if (result == FN_OK) {
            for (i = 0; i < count; ++i) {
                job = job_for(ready[i].handle);
                if (job != NULL) {
                    service(job, ready[i].flags);
                }
            }
        } else if (result == FN_ERR_UNSUPPORTED) {
            /* Firmware without fn_ready(): try every open session, accepting NOT_READY. */
            for (i = 0; i < SESSIONS; ++i) {
                if (jobs[i].open) {
                    service(&jobs[i], FN_READY_INFO | FN_READY_READ);
                }
            }
        } else {
            printf("ready: %u (%s)\n", (unsigned) result, fn_error_string(result));
            break;
        }
        if (nio_key_pressed()) {
            puts("stopped");
            break;
        }
    }

    nio_hint_end();
    for (i = 0; i < SESSIONS; ++i) {
        if (jobs[i].open) {
            fn_close(jobs[i].handle);             /* never leave a session open */
        }
    }
    nio_exit_pause();
    return 0;
}
