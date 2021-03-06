;{{{ Copyright (c) 2013, Johan Kotlinski
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
