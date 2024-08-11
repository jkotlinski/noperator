.export _setup_irq
.export _start_playing
.export _stop_playing
.export _ticks

.import _tick_rotate_chars

_ticks:
    .byte 0

playing:
    .byte 0

_setup_irq:
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

	; Set raster pos for IRQ.
    lda #$ff
    sta $d012
	cli
    rts

_start_playing:
    inc playing
    rts

fail:
    inc $d020
    jmp fail

irq1:
    jsr _tick_rotate_chars

    lda playing
    beq ret

    inc _ticks
    beq fail

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

_stop_playing:
    lda #0
    sta playing
    sta $d404 ; turn off SID
    sta $d404 + 7
    sta $d404 + 14
    rts
