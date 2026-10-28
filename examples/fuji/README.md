# fuji - the adapter itself

The FujiDevice describes the FujiNet: firmware version, build profile. It involves no network, so it
answers at once on any board, which makes it the right first call to prove the link works.

| Program | Shows |
|---|---|
| `fujiinfo/` | `fn_init`, `fn_is_ready`, `fn_fuji_get_info`; checking every status; coping with `FN_ERR_UNSUPPORTED` from older firmware. |
