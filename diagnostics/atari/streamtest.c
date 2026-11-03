/* Dedicated experimental stream test. Host: tools/stream_echo.py --host 0.0.0.0
 * Hardware verdict belongs to the user: DLI/VBI/sound are deliberately active.
 */
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fn_stream.h>
void stream_fx_start(void);
void stream_fx_stop(void);
unsigned char stream_dli=1, stream_sound=1;
volatile unsigned int stream_frames;
unsigned int stream_passes, stream_errors;
static fn_stream_io_t io;
static char input[122];
static unsigned char sent, got, size, state, status;
static unsigned char pattern(unsigned char n) { return (unsigned char)(stream_passes+n*17); }
static void banner(void) {
    clrscr();
    cprintf("Small NIO stream - experimental\r\n\r\n");
    cprintf("D: colour bands  S: sound  Q: stop\r\n");
    cprintf("VBI and DLI run during polled SIO.\r\n");
    cprintf("No U1MB tables or HSIO setting changed.\r\n\r\n");
}
int main(void) {
    unsigned char i,k;
    banner();
    cprintf("Echo server URL (tcp://host:9004):\r\n");
    cgets(input,FN_STREAM_MAX+1);
    status=fn_stream_init(&io);
    if(status) { cprintf("\r\nInit error %u - matching firmware?",status); cgetc();return 0; }
    io.length=strlen(input);memcpy(io.data,input,io.length);
    status=fn_stream_open(&io);
    if(status) { cprintf("\r\nOpen error %u",status);cgetc();return 0; }
    banner();stream_fx_start();
    for(;;) {
        if(kbhit()) {
            k=cgetc();
            if(k=='q'||k=='Q') break;
            if(k=='d'||k=='D') stream_dli^=1;
            if(k=='s'||k=='S') stream_sound^=1;
        }
        if(!state) {
            size=(unsigned char)(1+(stream_passes%120));sent=got=0;state=1;
        }
        if(state==1) {
            io.length=size-sent;
            for(i=0;i<io.length;++i) io.data[i]=pattern(sent+i);
            status=fn_stream_write(&io);
            if(!status) { sent+=io.actual;if(sent==size) state=2; }
        } else {
            io.length=size-got;status=fn_stream_read(&io);
            if(!status) {
                for(i=0;i<io.actual;++i) if(io.received[i]!=pattern(got+i)) ++stream_errors;
                got+=io.actual;
                if(got==size) { ++stream_passes;state=0; }
                else if(io.flags&1) status=6;
            }
        }
        gotoxy(0,7);cprintf("Echoes %5u  Errors %5u",stream_passes,stream_errors);
        gotoxy(0,9);cprintf("Frame %5u  Last status %3u",stream_frames,status);
        gotoxy(0,11);cprintf("Size %3u  Sent %3u  Read %3u",size,sent,got);
        gotoxy(0,13);cprintf("DLI %s  Sound %s",stream_dli?"on ":"off",stream_sound?"on ":"off");
        if(status && status!=3 && status!=4) {
            ++stream_errors;
            gotoxy(0,16);cprintf("Stopped: error %u. Press a key.",status);cgetc();break;
        }
    }
    stream_fx_stop();(void)fn_stream_close(&io);
    gotoxy(0,18);cprintf("Stopped. %u echoes, %u errors.\r\nReset to run again.",stream_passes,stream_errors);
    for(;;) cgetc();
}
