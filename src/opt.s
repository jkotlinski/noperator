.export _screen_left_opt
.export _screen_right_opt

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

_screen_right_opt:
    ldx #$e6
:   lda $700, x
    sta $701, x
    lda $db00, x
    sta $db01, x
    dex
    bne :-

    lda $700
    sta $701
    lda $db00
    sta $db01
    dex

:   lda $600, x
    sta $601, x
    lda $da00, x
    sta $da01, x
    dex
    bne :-

    lda $600
    sta $601
    lda $da00
    sta $da01
    dex

:   lda $500, x
    sta $501, x
    lda $d900, x
    sta $d901, x
    dex
    bne :-

    lda $500
    sta $501
    lda $d900
    sta $d901
    dex

:   lda $400, x
    sta $401, x
    lda $d800, x
    sta $d801, x
    dex
    bne :-

    lda $400
    sta $401
    lda $d800
    sta $d801

    rts
