# disk - disk images as drives

The DiskDevice mounts images from the FujiNet's SD card, a web server or a TNFS share into numbered slots
that the machine sees as its drives. (The FMOUNT utility in `nio-core-apps` is the usual way a user does it.)

| Program | Shows |
|---|---|
| `diskinfo/` | `fn_disk_mount` (and the geometry-required retry), `fn_disk_info`, `fn_disk_read_sector`, `fn_disk_unmount`, and reading `fn_disk_last_error`. Not built for the BBC. |
