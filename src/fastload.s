; C interface for fastloader

.export _loader_init
.export _loader_getc
.export _loader_open
.export _loader_load

.import initloader
.import getbyte
.import openfile
.import loadfile

.segment	"CODE"

_loader_init:
    jmp initloader

_loader_load:
    pha
    txa
    tay
    pla
    tax
    jsr loadfile

_loader_open:
    pha
    txa
    tay
    pla
    tax
    jsr openfile

_loader_getc:
    jsr getbyte
    bcs @error
    ldx #0
    rts ; success, read byte to a
@error:
	lda #$ff
    tax
	rts
