; {{{ Copyright (c) 2013, Johan Kotlinski
;
;Permission is hereby granted, free of charge, to any person obtaining a copy
;of this software and associated documentation files (the "Software"), to deal
;in the Software without restriction, including without limitation the rights
;to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;copies of the Software, and to permit persons to whom the Software is
;furnished to do so, subject to the following conditions:
;
;The above copyright notice and this permission notice shall be included in
;all copies or substantial portions of the Software.
;
;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
;THE SOFTWARE. }}}

.export _startirq
.export _stopirq
.export _ticks

_ticks:
    .byte 0

playing:
    .byte 1

_startirq:
    sei
    lda #>irq1
    sta $0315
    lda #<irq1
    sta $0314

	; Disable CIA interrupts.
    lda #$7f
    sta $dc0d

	; Clear MSbit of VIC raster register.
	and $d011
	sta $d011

	; Enable raster interrupt from VIC.
    lda #$01
    sta $d01a
    sta playing

	; Set raster pos for IRQ.
    lda #$ff
    sta $d012
	cli
    rts

irq1:
    lda playing
    beq ret

    inc _ticks

    ; music
    lda $1003
    cmp #$4c
    bne ret
    lda #0
    tay
    jsr $1003
	asl $d019 ; Acknowledge interrupt by clearing VIC interrupt flag.
    jmp $ea81 ; restore axy, end IRQ
ret:
	asl $d019 ; Acknowledge interrupt by clearing VIC interrupt flag.
	jmp $ea31 ; Jump to standard kernel IRQ service.

_stopirq:
    lda #0
    sta playing
    sta $d404 ; turn off SID
    sta $d404 + 7
    sta $d404 + 14
    rts
