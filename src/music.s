.export _init_music
.export _ticks_per_step

_init_music:
    lda $1000
    cmp #$4c
    bne :+
    lda #0
    tay
    jmp $1000
:   rts

_ticks_per_step:
    .byte 7	; lightforce
