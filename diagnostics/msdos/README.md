# MS-DOS diagnostics

MS-DOS-only tools built with `make TARGET=msdos`:

- `irqmon`: a foreground interrupt monitor (hooks the IRQ vectors, counts activity, shows vector owners and
  PIC masks). It does not use fujinet-nio-lib.
- `nioprobe` and `nioread`: ask the driver about a disk slot and read a sector, through `lib/nio.c`, a small
  probe library that talks to the driver's IOCTL interface directly. Retired from the shipped disk image;
  kept as driver diagnostics.

The user-facing MS-DOS `F*` utilities are in `../../../nio-core-apps` and `config-nio` in `../../../nio-config`.
`tools/create_msdos_img.py` (at the repository root) builds the diagnostic disk image
(`make TARGET=msdos disk`).
