# clock - the FujiNet's time

The FujiNet keeps network-synchronised time and a time zone. A machine without a battery clock can ask it.

| Program | Shows |
|---|---|
| `clock/` | Reading the time in several formats (ISO UTC, ISO with zone, raw, simple and ProDOS binary), reading and setting the zone, setting the time. Note how `FN_TIME_T` differs between 8-bit and larger machines. |
