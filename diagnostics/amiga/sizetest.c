/*
 * sizetest - do the Amiga types have the sizes the wire format assumes?
 *
 * The library's packets are built from 8/16/32-bit fields. Different Amiga NDK and compiler
 * combinations spell the 32-bit type differently (ULONG, unsigned long, uint32_t), so the static
 * assertions below fail the build if any of them is not 32 bits wide, and the program prints the sizes
 * it sees for the record. What matters is the width, not which spelling the type has.
 */

#include <exec/types.h>

#include <stdio.h>
#include <stdint.h>

_Static_assert(sizeof(ULONG) == 4, "ULONG is not 32 bits");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t is not 32 bits");
_Static_assert(sizeof(unsigned long) == 4, "unsigned long is not 32 bits");

#include <stdint.h>

/* The exact underlying type varies between NDK/toolchain combinations. The
 * wire contract requires 32 bits, which is asserted above, not a particular
 * spelling of the C type. */
#define UINT32_IS_ULONG __builtin_types_compatible_p(uint32_t, unsigned long)

int main(void)
{
    printf("sizeof(ULONG) = %lu\n", (unsigned long)sizeof(ULONG));
    printf("sizeof(LONG)  = %lu\n", (unsigned long)sizeof(LONG));
    printf("sizeof(UWORD) = %lu\n", (unsigned long)sizeof(UWORD));
    printf("sizeof(APTR)  = %lu\n", (unsigned long)sizeof(APTR));
    printf("uint32_t is unsigned long = %s\n", UINT32_IS_ULONG ? "yes" : "no");

    return 0;
}
