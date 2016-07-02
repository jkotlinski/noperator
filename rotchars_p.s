
.export _tick_rotate_chars

.import _rotchar_dirs
.import _rotchar_screencodes
.importzp ptr1, ptr2

_tick_rotate_chars:
    ldy #$f
loop:
    lda _rotchar_dirs, y
    beq continue
    tax

    lda #(($2800 / 8) >> 8)
    sta ptr1 + 1
    lda _rotchar_screencodes, y
    sta ptr1

    ; *= 8
    asl ptr1
    rol ptr1 + 1
    asl ptr1
    rol ptr1 + 1
    asl ptr1
    rol ptr1 + 1

    txa  ; a = direction
    cmp #1
    beq up
    cmp #2
    beq right
    cmp #3
    beq down

    ; left
    tya
    tax

    ldy #7
@loop:
    lda (ptr1),y
    asl a
    bcc :+
    ora #1
:   sta (ptr1),y
    dey
    bpl @loop

    txa
    tay

continue:
    dey
    bpl loop
    rts

right:
    tya
    tax

    ldy #7
@loop:
    lda (ptr1),y
    lsr a
    bcc :+
    ora #$80
:   sta (ptr1),y
    dey
    bpl @loop

    txa
    tay
    jmp continue

up:
    tya
    pha

    ldy ptr1
    iny
    sty ptr2
    ldy ptr1+1
    sty ptr2+1

    ; copy from ptr2 to ptr1
    ldy #0
    lda (ptr1),y
    tax
    lda (ptr2),y
    sta (ptr1),y  ; 1 => 0

    iny
    lda (ptr2),y
    sta (ptr1),y  ; 2 => 1
    iny
    lda (ptr2),y
    sta (ptr1),y  ; 3 => 2
    iny
    lda (ptr2),y
    sta (ptr1),y  ; 4 => 3
    iny
    lda (ptr2),y
    sta (ptr1),y  ; 5 => 4
    iny
    lda (ptr2),y
    sta (ptr1),y  ; 6 => 5
    iny
    lda (ptr2),y
    sta (ptr1),y  ; 7 => 6
    txa
    sta (ptr2),y  ; 0 => 7

    pla
    tay
    jmp continue

down:
    tya
    pha

    ldy ptr1
    iny
    sty ptr2
    ldy ptr1+1
    sty ptr2+1

    ; copy from ptr1 to ptr2
    ldy #6
    lda (ptr2),y
    tax
    lda (ptr1),y
    sta (ptr2),y  ; 6 => 7

    dey
    lda (ptr1),y
    sta (ptr2),y  ; 5 => 6
    dey
    lda (ptr1),y
    sta (ptr2),y  ; 4 => 5
    dey
    lda (ptr1),y
    sta (ptr2),y  ; 3 => 4
    dey
    lda (ptr1),y
    sta (ptr2),y  ; 2 => 3
    dey
    lda (ptr1),y
    sta (ptr2),y  ; 1 => 2
    dey
    lda (ptr1),y
    sta (ptr2),y  ; 0 => 1

    txa
    sta (ptr1),y  ; 7 => 0

    pla
    tay
    jmp continue
