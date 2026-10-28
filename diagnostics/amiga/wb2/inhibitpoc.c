/*
 * inhibitpoc 0|1 - proof of concept: Inhibit("DN0:") and report what AmigaDOS answered.
 *
 * Calls dos.library Inhibit() on DN0: with DOSTRUE (1) or DOSFALSE (0) and prints
 *   INHIBIT=<TRUE|FALSE> RETURN=<result> IOERR=<IoErr()>
 * The smallest possible probe of whether the handler accepts being inhibited. Workbench 2+ only.
 */

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    BOOL result;
    LONG error;

    if (argc != 2 || (argv[1][0] != '0' && argv[1][0] != '1')) {
        puts("usage: inhibitpoc 0|1");
        return 20;
    }

    result = Inhibit((STRPTR)"DN0:", argv[1][0] == '1' ? DOSTRUE : DOSFALSE);
    error = IoErr();
    printf("INHIBIT=%s RETURN=%ld IOERR=%ld\n",
           argv[1][0] == '1' ? "TRUE" : "FALSE",
           (long)result, (long)error);
    return result ? 0 : 1;
}
