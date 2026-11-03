; Diagnostic only: immediate VBI sound on channels 1/2 and six colour bands.
.export _stream_fx_start, _stream_fx_stop
.import _stream_dli, _stream_sound, _stream_frames
.bss
oldvbi: .res 2
olddli: .res 2
olddl: .res 2
row: .res 1
.data
dl:
.byte $70,$70,$70,$42
screen: .word 0
.repeat 23,i
.byte $02 | ((i .mod 4 = 2) * $80)
.endrepeat
.byte $41
.word dl
.code
_stream_fx_start:
    lda $58
    sta screen
    lda $59
    sta screen+1
    lda $230
    sta olddl
    lda $231
    sta olddl+1
    lda $200
    sta olddli
    lda $201
    sta olddli+1
    lda $222
    sta oldvbi
    lda $223
    sta oldvbi+1
    lda #<dli
    sta $200
    lda #>dli
    sta $201
    lda #<dl
    sta $230
    lda #>dl
    sta $231
    lda #6
    ldx #>vbi
    ldy #<vbi
    jsr $e45c
    lda #$c0
    sta $d40e
    rts
_stream_fx_stop:
    lda #$40
    sta $d40e
    lda #6
    ldx oldvbi+1
    ldy oldvbi
    jsr $e45c
    lda olddli
    sta $200
    lda olddli+1
    sta $201
    lda olddl
    sta $230
    lda olddl+1
    sta $231
    lda #0
    sta $d201
    sta $d203
    rts
vbi:
    pha
    inc _stream_frames
    bne :+
    inc _stream_frames+1
:
    lda #0
    sta row
    lda _stream_sound
    beq silent
    lda _stream_frames
    and #31
    clc
    adc #60
    sta $d200
    lda #80
    sta $d202
    lda #$a4
    bne audio
silent:
    lda #0
audio:
    sta $d201
    sta $d203
    pla
    jmp (oldvbi)
dli:
    pha
    lda _stream_dli
    beq plain
    lda row
    clc
    adc #$20
    sta row
    ora #2
    bne paint
plain:
    lda $2c6
paint:
    sta $d40a
    sta $d018
    pla
    rti
