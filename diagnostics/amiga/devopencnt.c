/*
 * devopencnt - how many programs have fujinet-disk.device open?
 *
 * Reads the device's lib_OpenCnt (under Forbid, since the list can change) and prints
 *   DEVICE name=fujinet-disk.device found=<0|1> opencnt=<n>
 * Exits 0 if the device is loaded, 10 if it is not. Used by the Amiberry tests to show that a sequence
 * of mounts and FUMOUNTs leaves the count where it started (the device is not leaking openers).
 */

#include <exec/devices.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <stdio.h>

extern struct ExecBase *SysBase;

int main(void)
{
    struct Device *device;
    BOOL found = FALSE;
    UWORD open_cnt = 0;

    Forbid();
    device = (struct Device *)FindName(&SysBase->DeviceList,
                                       (CONST_STRPTR)"fujinet-disk.device");
    if (device != NULL) {
        found = TRUE;
        open_cnt = device->dd_Library.lib_OpenCnt;
    }
    Permit();

    printf("DEVICE name=fujinet-disk.device found=%ld opencnt=%u\n",
           found ? 1L : 0L, (unsigned)open_cnt);
    return found ? 0 : 10;
}
