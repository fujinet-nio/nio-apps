/**
 * @file diskinfo.c
 * @brief The DiskDevice: mount a disk image from a URL, look at it, read a sector.
 *
 * FujiNet can present a disk image from anywhere it can reach (a file on its SD card, a web server, a
 * TNFS share) to the machine as if it were a floppy. The DiskDevice calls are how a program does that
 * by hand; the usual way a user does it is the FMOUNT utility.
 *
 *     fn_disk_mount(slot, uri, readonly, type, sector_size_hint, &info)   attach an image to a slot
 *     fn_disk_info(slot, &info)                                           what is in the slot now
 *     fn_disk_read_sector(slot, lba, buf, capacity, &len)                 one sector, by number
 *     fn_disk_unmount(slot)                                               let it go
 *
 * What to take from it:
 *
 *   - Slots are numbered from 1. The machine's own drives (D1:, DN0:, ...) are served from them, so
 *     mounting in a slot that is in use changes what the machine sees. This program uses slot 8 by
 *     default, which is rarely anyone's boot disk; pass a slot number to change it.
 *   - `type` FN_DISK_TYPE_AUTO lets the FujiNet recognise the image (ATR, SSD, ...). Some raw images
 *     do not say what geometry they have: the mount then fails with FN_DISK_ERR_GEOMETRY_REQUIRED and
 *     you mount again giving the sector size this machine uses (the `sector_size_hint`). When a disk
 *     call fails, fn_disk_last_error() gives the exact reason, which the transport status cannot.
 *   - The info structure gives the geometry: sector size and count. Sector numbers (LBAs) start at 0.
 *   - fn_disk_read_sector() returns up to `capacity` bytes and says how many it got in `len`: a disk
 *     with 128-byte sectors returns fewer than a 512-byte buffer holds.
 *   - Mount read-only unless you intend to write.
 *
 * Not available on the BBC library build, which has no DiskDevice client.
 *
 * Run: diskinfo [uri [slot]]    e.g.  diskinfo http://192.168.1.86:8080/disk.atr 8
 *      (default: the boot image the firmware was configured with, restored into the slot)
 */

#include "fujinet-nio.h"
#include "nio_example.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SLOT_DEFAULT 8
#define SHOW_BYTES   32

static uint8_t sector[512];

static void hexdump(const uint8_t *data, uint16_t length)
{
    uint16_t i;

    for (i = 0; i < length; ++i) {
        printf("%02X%c", (unsigned) data[i], (i % 16 == 15 || i + 1 == length) ? '\n' : ' ');
    }
}

static void report(const char *what, uint8_t result)
{
    printf("%s: %u (%s)", what, (unsigned) result, fn_error_string(result));
    if (result != FN_OK) {
        printf(", disk error %u", (unsigned) fn_disk_last_error());
    }
    putchar('\n');
}

int main(int argc, char **argv)
{
    const char *uri = argc > 1 ? argv[1] : NULL;
    const uint8_t slot = (uint8_t) (argc > 2 ? atoi(argv[2]) : SLOT_DEFAULT);
    fn_disk_info_t info;
    uint16_t length = 0;
    uint8_t result;

    result = fn_init();
    if (result != FN_OK) {
        printf("init: %u (%s)\n", (unsigned) result, fn_error_string(result));
        return 1;
    }

    if (uri != NULL) {
        printf("mount %s in slot %u (read-only)\n", uri, (unsigned) slot);
        result = fn_disk_mount(slot, uri, 1, FN_DISK_TYPE_AUTO, 0, &info);
        if (result != FN_OK && fn_disk_last_error() == FN_DISK_ERR_GEOMETRY_REQUIRED) {
            /* A raw image that does not say what it is: say what this machine uses and try again. */
            puts("geometry required: retrying as a raw image with 256-byte sectors");
            result = fn_disk_mount(slot, uri, 1, FN_DISK_TYPE_RAW, 256, &info);
        }
    } else {
        printf("restore the configured boot image into slot %u\n", (unsigned) slot);
        result = fn_disk_restore_boot(slot, &info);
    }
    report("mount", result);
    if (result != FN_OK) {
        return 1;
    }

    /* What did we get? (Mount returned the same information; fn_disk_info() asks again at any time.) */
    result = fn_disk_info(slot, &info);
    report("info", result);
    if (result == FN_OK) {
        printf("type %u, %u-byte sectors, %lu sectors%s%s\n", (unsigned) info.type, (unsigned) info.sector_size,
               (unsigned long) info.sector_count, (info.flags & FN_DISK_FLAG_READONLY) ? ", read-only" : "",
               (info.flags & FN_DISK_FLAG_MOUNTED) ? ", mounted" : "");
    }

    /* Read the first sector (LBA 0). `length` says how many bytes came back. */
    result = fn_disk_read_sector(slot, 0, sector, sizeof sector, &length);
    report("read sector 0", result);
    if (result == FN_OK) {
        printf("%u bytes; first %u:\n", (unsigned) length, (unsigned) (length < SHOW_BYTES ? length : SHOW_BYTES));
        hexdump(sector, length < SHOW_BYTES ? length : SHOW_BYTES);
    }

    result = fn_disk_unmount(slot);
    report("unmount", result);
    nio_exit_pause();
    return 0;
}
