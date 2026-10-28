# appstore - data that outlives your program

A key/value store on the FujiNet, one namespace per application. Write, read at offsets (so values larger
than your buffer work), stat, list and delete.

| Program | Shows |
|---|---|
| `astest/` | Every call, as a self-checking test (PASS/FAIL), with the caller-owned scratch buffer each call needs. |
