# diagnostics

Tools that are about one machine, driver or OS rather than about the library, so they are not examples:
each is built for its own target only (`diagnostics/<target>/`). Every file begins with what it is for. They
are run by hand, and several by the Amiberry integration suite (`../../integration-tests/amiberry/tests.toml`),
which finds them by program name.

| Folder | Contents |
|---|---|
| `amiga/` | Driver and checksum diagnostics: `checksumbench`, `devopencnt`, `diskinspect`, `sizetest`. |
| `amiga/wb2/` | DOS-list and handler experiments that need Workbench 2 or later (left out of the 1.3 profile): `doslistdiag`, `dynamicdosnode`, `lockdiag`, `inhibitpoc`, `fmount_inhibit_exp_a`, `fmount_inhibit_exp_b`. |
| `atari/` | `fsioraw`: one raw SIO command, with no library. |
| `msdos/` | Driver probes (`nioprobe`, `nioread`), the interrupt monitor `irqmon`, and the small probe library they use. |
