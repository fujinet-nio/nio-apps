#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests"
mkdir -p "$OUT"
gcc -std=c99 -Wall -Wextra -Werror -I"$ROOT/platform/include" -I"$ROOT/../fujinet-nio-lib/include" \
    -Dmain=httphead_main -c "$ROOT/examples/network/httphead/httphead.c" -o "$OUT/httphead.o"
gcc -std=c99 -Wall -Wextra -Werror -I"$ROOT/../fujinet-nio-lib/include" \
    "$ROOT/tests/httphead_test.c" "$OUT/httphead.o" -o "$OUT/httphead_test"
"$OUT/httphead_test" >/dev/null
echo "httphead example test passed"
