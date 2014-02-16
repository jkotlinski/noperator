.export _screen_left_opt

_screen_left_opt:
    ldx #0
:   lda $401, x
    sta $400, x
    lda $d801, x
    sta $d800, x
    inx
    bne :-
:   lda $501, x
    sta $500, x
    lda $d901, x
    sta $d900, x
    inx
    bne :-
:   lda $601, x
    sta $600, x
    lda $da01, x
    sta $da00, x
    inx
    bne :-
:   lda $701, x
    sta $700, x
    lda $db01, x
    sta $db00, x
    inx
    bne :-
    rts
