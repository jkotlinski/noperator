;-------------------------------------------------------------------------------
; COVERT BITOPS Autoconfiguring Loader/Depacker V2.25
; with 1541/1571/1581/CMD FD/CMD HD/IDE64/Fastdrive-emu autodetection & support
;
; EXOMIZER 2 depack by Magnus Lind & Krill
; PUCRUNCH depack by Pasi Ojala
; 1581/CMD FD/CMD HD information from Ninja & DocBacardi /The Dreams
; 2MHz 2-bit transfer delay code by MagerValp
; Rest by Lasse Öörni
;
; Thanks to K.M/TABOO for inspiration on badline detection and 1-bit transfer,
; and Marko Mäkelä for his original irqloader.s (huge inspiration)
;-------------------------------------------------------------------------------

.define processor 6502

.export initloader
.export getbyte
.export openfile
.export loadfile

.segment "CODE"

;-------------------------------------------------------------------------------
; Include your loader configuration file at this point!
;-------------------------------------------------------------------------------

.include "cfg_2bit.s"

;-------------------------------------------------------------------------------
; Defines derived from the compile options (need not be changed)
;-------------------------------------------------------------------------------

                .if ADDITIONAL_ZEROPAGE>0
loadtempreg     = zpbase2+0      ;Temp variables for the loader
bufferstatus    = zpbase2+1      ;Bytes in fastload buffer
fileopen        = zpbase2+2      ;File open indicator
fastloadstatus  = zpbase2+3      ;Fastloader active indicator
                .endif

destlo          = zpbase+0
desthi          = zpbase+1

;-------------------------------------------------------------------------------
; Other defines
;-------------------------------------------------------------------------------

MW_LENGTH       = 32            ;Bytes in one M-W command

status          = $90           ;Kernal zeropage variables
messages        = $9d
fa              = $ba

acsbf           = $01           ;Diskdrive variables: Buffer 1 command
trkbf           = $08           ;Buffer 1 track
sctbf           = $09           ;Buffer 1 sector
iddrv0          = $12           ;Disk drive ID
drvtemp         = $06           ;Temp variable
id              = $16           ;Disk ID
buf             = $0400         ;Sector data buffer
drvstart        = $0500         ;Start of drivecode
initialize      = $d005         ;Initialize routine in 1541 ROM

ciout           = $ffa8         ;Kernal routines
listen          = $ffb1
second          = $ff93
unlsn           = $ffae
talk            = $ffb4
tksa            = $ff96
untlk           = $ffab
acptr           = $ffa5
chkin           = $ffc6
chkout          = $ffc9
chrin           = $ffcf
chrout          = $ffd2
close           = $ffc3
open            = $ffc0
setmsg          = $ff90
setnam          = $ffbd
setlfs          = $ffba
clrchn          = $ffcc
getin           = $ffe4
load            = $ffd5
save            = $ffd8

;-------------------------------------------------------------------------------
; Resident portion of loader (routines that you're going to use at runtime)
;-------------------------------------------------------------------------------

                .if LOADFILE_UNPACKED>0
;-------------------------------------------------------------------------------
; LOADFILE
;
; Loads an unpacked file
;
; Parameters: X (low),Y (high): Address of null-terminated filename
; Returns: C=0 OK, C=1 error (A holds errorcode)
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

loadfile:       jsr openfile
                jsr getbyte             ;Get startaddress lowbyte
                bcs loadfile_fail       ;If EOF at first byte, error
                sta destlo
                jsr getbyte             ;Get startaddress highbyte
                sta desthi
                ldy #$00
loadfile_loop:  jsr getbyte
                bcs loadfile_eof
                .if LOAD_UNDER_IO>0
                jsr disableio           ;Allow loading under I/O area
                .endif
                sta (destlo),y
                .if LOAD_UNDER_IO>0
                jsr enableio
                .endif
                iny
                bne loadfile_loop
                inc desthi
                jmp loadfile_loop
loadfile_eof:   cmp #$01                ;Returncode 0 = OK, others error
loadfile_fail:  rts
                .endif

                .if LOADFILE_EXOMIZER>0
;-------------------------------------------------------------------------------
; LOADFILE_EXOMIZER
;
; Loads a file packed with EXOMIZERx
;
; Parameters: X (low),Y (high): Address of null-terminated filename
; Returns: C=0 OK, C=1 error (A holds errorcode)
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

tabl_bi         = depackbuffer
tabl_lo         = depackbuffer+52
tabl_hi         = depackbuffer+104

zp_len_lo       = zpbase+0
zp_src_lo       = zpbase+1
zp_src_hi       = zpbase+2
zp_bits_lo      = zpbase+3
zp_bits_hi      = zpbase+4
zp_bitbuf       = zpbase+5
zp_dest_lo      = zpbase+6
zp_dest_hi      = zpbase+7

loadfile_exomizer:
                jsr openfile
                tsx
                stx exomizer_stackptr+1

; -------------------------------------------------------------------
; This source code is altered and is not the original version found on
; the Exomizer homepage.
; It contains modifications made by Krill/Plush to depack a packed file
; crunched forward and to work with his loader.
;
; Further modification (error handling, loading under IO area) &
; bugfixing of the forward decruncher by Lasse Öörni
; -------------------------------------------------------------------
;
; Copyright (c) 2002 - 2005 Magnus Lind.
;
; This software is provided 'as-is', without any express or implied warranty.
; In no event will the authors be held liable for any damages arising from
; the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
;   1. The origin of this software must not be misrepresented; you must not
;   claim that you wrote the original software. If you use this software in a
;   product, an acknowledgment in the product documentation would be
;   appreciated but is not required.
;
;   2. Altered source versions must be plainly marked as such, and must not
;   be misrepresented as being the original software.
;
;   3. This notice may not be removed or altered from any distribution.
;
;   4. The names of this software and/or it's copyright holders may not be
;   used to endorse or promote products derived from this software without
;   specific prior written permission.
;
; -------------------------------------------------------------------
; no code below this comment has to be modified in order to generate
; a working decruncher of this source file.
; However, you may want to relocate the tables last in the file to a
; more suitable address.
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; jsr this label to decrunch, it will in turn init the tables and
; call the decruncher
; no constraints on register content, however the
; decimal flag has to be #0 (it almost always is, otherwise do a cld)
exomizer:

  .if FORWARD_DECRUNCHING>0

; -------------------------------------------------------------------
; init zeropage, x and y regs.
;
  ldx #3
  ldy #0
init_zp:
  jsr getbyte
  bcs exomizer_error
  sta zp_bitbuf-1,x
  dex
  bne init_zp

; -------------------------------------------------------------------
; calculate tables (50 bytes)
; x and y must be #0 when entering
;
nextone:
  inx
  tya
  and #$0f
  beq shortcut    ; start with new sequence

  txa          ; this clears reg a
  lsr          ; and sets the carry flag
  ldx tabl_bi-1,y
rolle:
  rol
  rol zp_bits_hi
  dex
  bpl rolle    ; c = 0 after this (rol zp_bits_hi)

  adc tabl_lo-1,y
  tax

  lda zp_bits_hi
  adc tabl_hi-1,y
shortcut:
  sta tabl_hi,y
  txa
  sta tabl_lo,y

  ldx #4
  jsr get_bits    ; clears x-reg.
  sta tabl_bi,y
  iny
  cpy #52
  bne nextone
  beq begin

; -------------------------------------------------------------------
; get bits (29 bytes)
;
; args:
;   x = number of bits to get
; returns:
;   a = #bits_lo
;   x = #0
;   c = 0
;   z = 1
;   zp_bits_hi = #bits_hi
; notes:
;   y is untouched
; -------------------------------------------------------------------
get_bits:
  lda #$00
  sta zp_bits_hi
  cpx #$01
  bcc bits_done
bits_next:
  lsr zp_bitbuf
  bne bits_ok
  pha
  jsr getbyte
  bcs exomizer_error
  sec
  ror
  sta zp_bitbuf
  pla
bits_ok:
  rol
  rol zp_bits_hi
  dex
  bne bits_next
bits_done:
  rts

exomizer_ok:
  clc
exomizer_error:
exomizer_stackptr:
  ldx #$ff
  txs
  rts

; -------------------------------------------------------------------
; literal sequence handling
;
  .if LITERAL_SEQUENCES_NOT_USED=0
literal_start:
  ldx #$10    ; these 16 bits
  jsr get_bits; tell the length of the sequence
  ldx zp_bits_hi
  .endif
literal_start1: ; if literal byte, a = 1, zp_bits_hi = 0
  sta zp_len_lo

; -------------------------------------------------------------------
; main copy loop
; x = length hi
; y = length lo
;
copy_start:
  ldy #$00
copy_next:
  bcs copy_noliteral
  jsr getbyte
  bcs exomizer_error
copy_noliteral:
  .if LOAD_UNDER_IO>0
  jsr disableio
  .endif
  bcc copy_store
  lda (zp_src_lo),y
copy_store:
  sta (zp_dest_lo),y
  .if LOAD_UNDER_IO>0
  jsr enableio
  .endif
  iny
  bne copy_skiphi
  dex
  inc zp_dest_hi
  inc zp_src_hi
copy_skiphi:
  tya
  eor zp_len_lo
  bne copy_next
  txa
  bne copy_next
  tya
  clc
  adc zp_dest_lo
  sta zp_dest_lo
  bcc copy_skiphi2
  inc zp_dest_hi
copy_skiphi2:

; -------------------------------------------------------------------
; decruncher entry point, needs calculated tables (21(13) bytes)
; x and y must be #0 when entering
;
begin:
  inx
  jsr get_bits
  tay
  bne literal_start1; if bit set, get a literal byte
getgamma:
  inx
  jsr bits_next
  lsr
  iny
  bcc getgamma
  cpy #$11
  beq exomizer_ok   ; gamma = 17   : end of file
  .if LITERAL_SEQUENCES_NOT_USED=0
  bcs literal_start ; gamma = 18   : literal sequence
  .endif
                    ; gamma = 1..16: sequence

; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (11 bytes)
;
  ldx tabl_bi-1,y
  jsr get_bits
  adc tabl_lo-1,y  ; we have now calculated zp_len_lo
  sta zp_len_lo
; -------------------------------------------------------------------
; now do the hibyte of the sequence length calculation (6 bytes)
  lda zp_bits_hi
  adc tabl_hi-1,y  ; c = 0 after this.
  pha
; -------------------------------------------------------------------
; here we decide what offset table to use (20 bytes)
; x is 0 here
;
  bne nots123
  ldy zp_len_lo
  cpy #$04
  bcc size123
nots123:
  ldy #$03
size123:
  ldx tabl_bit-1,y
  jsr get_bits
  adc tabl_off-1,y  ; c = 0 after this.
  tay      ; 1 <= y <= 52 here

; -------------------------------------------------------------------
; calulate absolute offset (zp_src)
;
  ldx tabl_bi,y
  jsr get_bits
  adc tabl_lo,y
  bcc skipcarry
  inc zp_bits_hi
skipcarry:
  sec
  eor #$ff
  adc zp_dest_lo
  sta zp_src_lo
  lda zp_dest_hi
  sbc zp_bits_hi
  sbc tabl_hi,y
  sta zp_src_hi

; -------------------------------------------------------------------
; prepare for copy loop (8(6) bytes)
;
  pla
  tax
  sec
  jmp copy_start

  .else

; -------------------------------------------------------------------
; init zeropage, x and y regs. (12 bytes)
;
  ldy #0
  ldx #3
init_zp:
  jsr getbyte
  bcs exomizer_error
  sta zp_bitbuf-1,x
  dex
  bne init_zp

; -------------------------------------------------------------------
; calculate tables (50 bytes)
; x and y must be #0 when entering
;
nextone:
  inx
  tya
  and #$0f
  beq shortcut              ; starta på ny sekvens
  txa                       ; this clears reg a
  lsr                       ; and sets the carry flag
  ldx tabl_bi-1,y
rolle:          
  rol
  rol zp_bits_hi
  dex
  bpl rolle                 ; c = 0 after this (rol zp_bits_hi)
  adc tabl_lo-1,y
  tax
  lda zp_bits_hi
  adc tabl_hi-1,y
shortcut:       
  sta tabl_hi,y
  txa
  sta tabl_lo,y
  ldx #4
  jsr get_bits              ; clears x-reg.
  sta tabl_bi,y
  iny
  cpy #52
  bne nextone
  ldy #0
  beq begin

; -------------------------------------------------------------------
; get bits (29 bytes)
;
; args:
;   x = number of bits to get
; returns:
;   a = #bits_lo
;   x = #0
;   c = 0
;   z = 1
;   zp_bits_hi = #bits_hi
; notes:
;   y is untouched
; -------------------------------------------------------------------
get_bits:       
  lda #$00
  sta zp_bits_hi
  cpx #$01
  bcc bits_done
bits_next:      
  lsr zp_bitbuf
  bne bits_ok
  pha
literal_get_byte:
  php
  jsr getbyte
  bcs exomizer_error
  plp
  bcc literal_byte_gotten
  ror
  sta zp_bitbuf
  pla
bits_ok:
  rol
  rol zp_bits_hi
  dex
  bne bits_next
bits_done:
  rts

exomizer_ok:
  clc
exomizer_error:
exomizer_stackptr:
  ldx #$ff
  txs
  rts

; -------------------------------------------------------------------
; main copy loop (18(16) bytes)
;
copy_next_hi:
  dex
  dec zp_dest_hi
  dec zp_src_hi
copy_next:
  dey
  .if LITERAL_SEQUENCES_NOT_USED = 0
  bcc literal_get_byte
  .endif
literal_byte_gotten:
  .if LOAD_UNDER_IO>0
  jsr disableio
  .endif
  bcc copy_store
  lda (zp_src_lo),y
copy_store:
  sta (zp_dest_lo),y
  .if LOAD_UNDER_IO>0
  jsr enableio
  .endif
copy_start:
  tya
  bne copy_next
begin:
  txa
  bne copy_next_hi
; -------------------------------------------------------------------
; decruncher entry point, needs calculated tables (21(13) bytes)
; x and y must be #0 when entering
;
  .if LITERAL_SEQUENCES_NOT_USED = 0
  inx
  jsr get_bits
  tay
  bne literal_start1
  .else
  dey
  .endif
begin2:
  inx
  jsr bits_next
  lsr
  iny
  bcc begin2
  .if LITERAL_SEQUENCES_NOT_USED > 0
  beq literal_start
  .endif
  cpy #$11
  .if LITERAL_SEQUENCES_NOT_USED = 0
  bcc sequence_start
  beq exomizer_ok
; -------------------------------------------------------------------
; literal sequence handling (13(2) bytes)
;
  ldx #$10
  jsr get_bits
literal_start1: 
  sta zp_len_lo
  ldx zp_bits_hi
  ldy #0
  bcc literal_start
sequence_start:
  .else
  bcs exomizer_ok
  .endif
; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (11 bytes)
;
  ldx tabl_bi-1,y
  jsr get_bits
  adc tabl_lo-1,y         ; we have now calculated zp_len_lo
  sta zp_len_lo
; -------------------------------------------------------------------
; now do the hibyte of the sequence length calculation (6 bytes)
  lda zp_bits_hi
  adc tabl_hi-1,y         ; c = 0 after this.
  pha
; -------------------------------------------------------------------
; here we decide what offset table to use (20 bytes)
; x is 0 here
;
  bne nots123
  ldy zp_len_lo
  cpy #$04
  bcc size123
nots123:        
  ldy #$03
size123:        
  ldx tabl_bit-1,y
  jsr get_bits
  adc tabl_off-1,y          ; c = 0 after this.
  tay                       ; 1 <= y <= 52 here
; -------------------------------------------------------------------
; Here we do the dest_lo -= len_lo subtraction to prepare zp_dest
; but we do it backwards:                a - b == (b - a - 1) ^ ~0 (C-syntax)
; (16(16) bytes)
  lda zp_len_lo
literal_start:  
  sbc zp_dest_lo            ; literal enters here with y = 0, c = 1
  bcc noborrow
  dec zp_dest_hi
noborrow:
  eor #$ff
  sta zp_dest_lo
  cpy #$01                  ; y < 1 then literal
  .if LITERAL_SEQUENCES_NOT_USED = 0
  bcc pre_copy
  .else
  bcc literal_get_byte
  .endif
; -------------------------------------------------------------------
; calulate absolute offset (zp_src) (27 bytes)
;
  ldx tabl_bi,y
  jsr get_bits;
  adc tabl_lo,y
  bcc skipcarry
  inc zp_bits_hi
  clc
skipcarry:
  adc zp_dest_lo
  sta zp_src_lo
  lda zp_bits_hi
  adc tabl_hi,y
  adc zp_dest_hi
  sta zp_src_hi
; -------------------------------------------------------------------
; prepare for copy loop (8(6) bytes)
;
  pla
  tax
  sec
  .if LITERAL_SEQUENCES_NOT_USED = 0
pre_copy:
  ldy zp_len_lo
  jmp copy_start
  .else
  ldy zp_len_lo
  bcs copy_start
  .endif

  .endif

; -------------------------------------------------------------------
; two small static tables (6(6) bytes)
;
tabl_bit:
	.byte 2,4,4
tabl_off:
	.byte 48,32,16
; -------------------------------------------------------------------
; end of decruncher
; -------------------------------------------------------------------
                .endif

                .if LOADFILE_PUCRUNCH>0

;-------------------------------------------------------------------------------
; LOADFILE_PUCRUNCH
;
; Loads a file packed with PUCRUNCH
;
; Parameters: X (low),Y (high): Address of null-terminated filename
; Returns: C=0 OK, C=1 error (A holds errorcode)
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

lzpos           = zpbase
bitstr          = zpbase+2

table           = depackbuffer

loadfile_pucrunch:
                jsr openfile
                tsx
                stx pucrunch_stackptr+1
                jsr getbyte                     ;Throw away file startaddress
                jsr getbyte

;-------------------------------------------------------------------------------
; PUCRUNCH DECOMPRESSOR by Pasi Ojala
;
; SHORT+IRQLOAD         354 bytes
; no rle =~             -83 bytes -> 271
; fixed params =~       -48 bytes -> 306
;                       223 bytes
; Parameters: -
; Returns: -
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

        ldx #5
@222:   jsr getbyt      ; skip 'p', 'u', endAddr HI&LO, leave starting escape in A
        dex
        bne @222
        sta esc+1       ; starting escape
        jsr getbyt      ; read startAddr
        sta outpos+1
        jsr getbyt
        sta outpos+2
        jsr getbyt      ; read # of escape bits
        sta escB0+1
        sta escB1+1
        lda #8
        sec
        sbc escB1+1
        sta noesc+1     ; 8-escBits

        jsr getbyt
        sta mg+1        ; maxGamma + 1
        lda #9
        sec
        sbc mg+1        ; 8 - maxGamma == (8 + 1) - (maxGamma + 1)
        sta longrle+1
        jsr getbyt
        sta mg1+1       ; (1<<maxGamma)
        asl
        clc
        sbc #0
        sta mg21+1      ; (2<<maxGamma) - 1
        jsr getbyt
        sta elzpb+1

        ldx #$03
@2:     jsr getbyt      ; Get 3 bytes, 2 unused (exec address)
        dex             ; and rleUsed. X is 0 after this loop
        bne @2

        ;jsr getbyt     ; exec address
        ;sta lo+1       ; lo
        ;jsr getbyt
        ;sta hi+1       ; hi
        ;
        ;jsr getbyt     ; rleUsed
        ;ldx #0

        tay
        sty bitstr
@0:     beq @1          ; Y == 0 ?
        jsr getbyt
        sta table,x
        inx
        dey
        bne @0
@1:     ; setup bit store - $80 means empty
        lda #$80
        sta bitstr
        jmp main

getbyt  jsr getnew
        lda bitstr
        ror
        rts

newesc  ldy esc+1       ; remember the old code (top bits for escaped byte)
escB0   ldx #2          ; ** PARAMETER  0..8
        jsr getchkf     ; get & save the new escape code
        sta esc+1
        tya             ; pre-set the bits
        ; Fall through and get the rest of the bits.
noesc   ldx #6          ; ** PARAMETER  8..0
        jsr getchkf
        jsr putch       ; output the escaped/normal byte
        ; Fall through and check the escape bits again
main    ldy #0          ; Reset to a defined state
        tya             ; A = 0
escB1   ldx #2          ; ** PARAMETER  0..8
        jsr getchkf     ; X = 0
esc     cmp #0
        bne noesc
        ; Fall through to packed code

        jsr getval      ; X = 0
        sta lzpos       ; xstore - save the length for a later time
        lsr             ; cmp #1        ; LEN == 2 ? (A is never 0)
        bne lz77        ; LEN != 2      -> LZ77
        ;tya            ; A = 0
        jsr get1bit     ; X = 0
        lsr             ; bit -> C, A = 0
        bcc lz77_2      ; A=0 -> LZPOS+1
        ;***FALL THRU***

        ; e..e01
        jsr get1bit     ; X = 0
        lsr             ; bit -> C, A = 0
        bcc newesc      ; e..e010
        ;***FALL THRU***

        ; e..e011
srle    iny             ; Y is 1 bigger than MSB loops
        jsr getval      ; Y is 1, get len, X = 0
        sta lzpos       ; xstore - Save length LSB
mg1     cmp #64         ; ** PARAMETER 63-64 -> C clear, 64-64 -> C set..
        bcc chrcode     ; short RLE, get bytecode

longrle ldx #2          ; ** PARAMETER  111111xxxxxx
        jsr getbits     ; get 3/2/1 more bits to get a full byte, X = 0
        sta lzpos       ; xstore - Save length LSB

        jsr getval      ; length MSB, X = 0
        tay             ; Y is 1 bigger than MSB loops

chrcode jsr getval      ; Byte Code, X = 0
        tax             ; this is executed most of the time anyway
        lda table-1,x   ; Saves one jump if done here (loses one txa)

        cpx #32         ; 31-32 -> C clear, 32-32 -> C set..
        bcc @1          ; 1..31, we got the right byte from the table

        ; Ranks 32..64 (11111°xxxxx), get byte..
        txa             ; get back the value (5 valid bits)
        ldx #3
        jsr getbits     ; get 3 more bits to get a full byte, X = 0

@1:     ldx lzpos       ; xstore - get length LSB
        inx             ; adjust for cpx#$ff;bne -> bne
dorle   jsr putch
        dex
        bne dorle       ; xstore 0..255 -> 1..256
        dey
        bne dorle       ; Y was 1 bigger than wanted originally
mainbeq beq main        ; reverse condition -> jump always


lz77    jsr getval      ; X = 0
mg21    cmp #127        ; ** PARAMETER  Clears carry (is maximum value)
        bne noeof
eof:    clc             ; Loading ended OK
        rts

noeof   sbc #0          ; C is clear -> subtract 1  (1..126 -> 0..125)
elzpb   ldx #0          ; ** PARAMETER (more bits to get)
        jsr getchkf     ; clears Carry, X = 0

lz77_2  sta lzpos+1     ; offset MSB
        jsr getbyt2     ; clears Carry, X = 0
        ; Note: Already eor:ed in the compressor..
        ;eor #255       ; offset LSB 2's complement -1 (i.e. -X = ~X+1)
        adc outpos+1    ; -offset -1 + curpos (C is clear)
        ldx lzpos       ; xstore = LZLEN (read before it's overwritten)
        sta lzpos

        lda outpos+2
        sbc lzpos+1     ; takes C into account
        sta lzpos+1     ; copy X+1 number of chars from LZPOS to outpos+1
        ;ldy #0         ; Y was 0 originally, we don't change it

        inx             ; adjust for cpx#$ff;bne -> bne

lzslow  .if LOAD_UNDER_IO>0
        jsr disableio
        .endif
        lda (lzpos),y   ; using abs,y is 3 bytes longer, only 1 cycle/byte faster
        jsr outpos
        iny             ; Y does not wrap because X=0..255 and Y initially 0
        dex
        bne lzslow      ; X loops, (256,1..255)
        jmp main

putch   .if LOAD_UNDER_IO>0
        jsr disableio
        .endif
outpos  sta $aaaa       ; ** parameter
        inc outpos+1    ; ZP
        bne putchok
        inc outpos+2    ; ZP
putchok .if LOAD_UNDER_IO>0
        jmp enableio
        .else
        rts
        .endif

getnew  pha             ; 1 Byte/3 cycles
        jsr getbyte
        bcs pucrunch_fail
@0:     sec
        rol             ; Shift out the next bit and
                        ;  shift in C=1 (last bit marker)
        sta bitstr      ; bitstr initial value = $80 == empty
        pla             ; 1 Byte/4 cycles
        rts
        ; 25+12 = 37

; getval : Gets a 'static huffman coded' value
n; ** Scratches X, returns the value in A **
getval  inx             ; X <- 1
        txa             ; set the top bit (value is 1..255)
gv0     asl bitstr
        bne @1
        jsr getnew
@1:     bcc getchk      ; got 0-bit
        inx
mg      cpx #7          ; ** PARAMETER unary code maximum length + 1
        bne gv0
        beq getchk      ; inverse condition -> jump always
        ; getval: 18 bytes
        ; 15 + 17*n + 6+15*n+12 + 36*n/8 = 33 + 32*n + 36*n/8 cycles

; getbits: Gets X bits from the stream
; ** Scratches X, returns the value in A **
getbyt2 ldx #7
get1bit inx             ;2
getbits asl bitstr
        bne @1
        jsr getnew
@1:     rol             ;2
getchk  dex             ;2              more bits to get ?
getchkf bne getbits     ;2/3
        clc             ;2              return carry cleared
        rts             ;6+6

pucrunch_fail:                          ;A premature EOF is treated as an
pucrunch_stackptr:                      ;error; return directly to caller
        ldx #$00
        txs
        rts
                .endif

;-------------------------------------------------------------------------------
; OPENFILE
;
; Opens a file either with slow or fast loader. If a file is already open, does
; nothing!
;
; Parameters: X (low),Y (high): Address of null-terminated filename
; Returns: -
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

openfile:       lda fileopen            ;A file already open?
                beq open_ok
                rts
open_ok:        .if LONG_NAMES>0
                stx destlo
                sty desthi
                .else
                stx filename
                sty filename+1
                .endif
                inc fileopen            ;File opened
                lda usefastload
                bne fastopen

;-------------------------------------------------------------------------------
; SLOWOPEN
;
; Opens a file without fastloader.
;
; Parameters: A:0 (it always is at this point)
; Returns: -
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

slowopen:       .if LONG_NAMES>0
                tay
                .endif
                jsr kernalon
                .if LONG_NAMES>0
slowopen_nameloop:
                iny
                lda (destlo),y
                bne slowopen_nameloop
                tya
                ldx destlo
                ldy desthi
                .else
                lda #$03
                ldx #<filename
                ldy #>filename
                .endif
                jsr setnam
                lda #$02
                ldy #$00
                jsr setlfsdevice
                jsr open
                ldx #$02                ;File number
                jmp chkin

;-------------------------------------------------------------------------------
; FASTOPEN
;
; Opens a file with fastloader. Uses an asynchronous protocol inspired by
; Marko Mäkelä's work when sending the filename.
;
; Parameters: -
; Returns: -
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

fastopen:       jsr initfastload        ;If fastloader is not yet initted,
                                        ;init it now
                .if LONG_NAMES>0
                ldy #$00
fastload_sendouter:
                lda (destlo),y
                sta loadtempreg
                pha
                ldx #$08                ;Bit counter
                .else
                ldx #$01
fastload_sendouter:
                ldy #$08                ;Bit counter
                .endif
fastload_sendinner:
                bit $dd00               ;Wait for both DATA & CLK to go high
                bpl fastload_sendinner
                bvc fastload_sendinner
                .if LONG_NAMES=0
                lsr filename,x
                .else
                lsr loadtempreg
                .endif
                lda #$10
                ora $dd00
                bcc fastload_zerobit
                eor #$30
fastload_zerobit:
                sta $dd00
                lda #$c0                ;Wait for CLK & DATA low (answer from
fastload_sendack:                       ;the diskdrive)
                bit $dd00
                bne fastload_sendack
                lda #$ff-$30            ;Set DATA and CLK high
                and $dd00
                sta $dd00
                .if LONG_NAMES>0
                dex
                bne fastload_sendinner
                iny
                pla
                bne fastload_sendouter
                .else
                dey
                bne fastload_sendinner
                dex
                bpl fastload_sendouter
                .endif
                sta $d07a               ;SCPU to slow mode
fastload_predelay:
                dex                     ;Delay to make sure the 1541 has
                bne fastload_predelay   ;set DATA high before we continue

fastload_fillbuffer:
                sta $d07a               ;SCPU to slow mode
                
                .if TWOBIT_PROTOCOL>0

                ldx #$00
fastload_fbwait:
                bit $dd00               ;Wait for 1541 to signal data ready by
                bmi fastload_fbwait     ;setting DATA low
fastload_fbloop:
                sei
fastload_waitbadline:
                lda $d011               ;Check that a badline won't disturb
                clc                     ;the timing
                sbc $d012
                and #$07
                beq fastload_waitbadline
                lda $dd00
                ora #$10
                sta $dd00               ;Set CLK low
fastload_delay: bit $00                 ;Delay (NTSC version, will be modified for PAL)
                nop
                and #$03
                sta fastload_eor+1
                sta $dd00               ;Set CLK high
                lda $dd00
                lsr
                lsr
                eor $dd00
                lsr
                lsr
                eor $dd00
                lsr
                lsr
fastload_eor:   eor #$00
                eor $dd00
                cli
                sta loadbuffer,x
                inx
                bne fastload_fbloop

                .else

fastload_fbwait:bit $dd00                 ;Wait for 1541 to signal data ready by
                bvc fastload_fbwait       ;setting CLK high
                pha                       ;Some delay before beginning
                pla
                pha
                pla
                ldx #$00
fastload_fillbufferloop:                  ;1bit receive code
                nop
                nop
                nop
                ldy #$08                  ;Bit counter
fastload_bitloop:
                nop
                lda #$10
                eor $dd00                 ;Take databit
                sta $dd00                 ;Store reversed clockbit
                asl
                ror loadbuffer,x
                dey
                bne fastload_bitloop
                .if BORDER_FLASHING>0
                dec $d020
                inc $d020
                .endif
                inx
                bne fastload_fillbufferloop
                
                .endif

fillbuffer_common:
                stx bufferstatus                ;X is 0 here
                ldx #$fe
                lda loadbuffer                  ;Full 254 bytes?
                bne fastload_fullbuffer
                ldx loadbuffer+1                ;End of load?
                bne fastload_noloadend
                stx fileopen                    ;Clear fileopen indicator
                lda loadbuffer+2                ;Read the return/error code
                sta fileclosed+1
fastload_noloadend:
                dex
fastload_fullbuffer:
                stx fastload_endcomp+1
fileclosed:     lda #$00
                sec
                rts

;-------------------------------------------------------------------------------
; GETBYTE
;
; Gets a byte from an opened file.
;
; Parameters: -
; Returns: C=0 OK, A contains byte
;          C=1 File stream ended. A contains the error code:
;              $00 - OK, end of file
;              $01 - Sector read error (only with fastloading)
;              $02 - File not found
; Modifies: A
;-------------------------------------------------------------------------------

getbyte:        lda fileopen
                beq fileclosed
                stx getbyte_restx+1
                sty getbyte_resty+1
getbyte_fileopen:
                lda usefastload
                beq slowload_getbyte
fastload_getbyte:
                ldx bufferstatus
                lda loadbuffer+2,x
                inx
fastload_endcomp:cpx #$00                       ;Reach end of buffer?
                stx bufferstatus
                bcc getbyte_restx
                pha
                jsr fastload_fillbuffer
                pla
getbyte_done:   clc
getbyte_restx:  ldx #$00
getbyte_resty:  ldy #$00
                rts

slowload_getbyte:
                jsr chrin
                ldx status
                beq getbyte_done
                pha
                txa
                and #$03
                sta fileclosed+1        ;EOF - store return code
                dec fileopen
                jsr close_kernaloff
                pla
                ldx fileclosed+1        ;Check return code, if nonzero,
                cpx #$01                ;return with carry set and return
                bcc getbyte_restx       ;code in A
                txa
                bcs getbyte_restx

                .if LOAD_UNDER_IO>0
;-------------------------------------------------------------------------------
; DISABLEIO
;
; Stores $01 status, disables interrupts & IO area.
;
; Parameters: -
; Returns: -
; Modifies: -
;-------------------------------------------------------------------------------

disableio:      pha
                lda $01
                sta enableio_01+1
                lda #$34
                sei
                sta $01
                pla
                rts

;-------------------------------------------------------------------------------
; ENABLEIO
;
; Restores $01 status and enables interrupts
;
; Parameters: -
; Returns: -
; Modifies: -
;-------------------------------------------------------------------------------

enableio:       pha
enableio_01:    lda #$36
                sta $01
                cli
                pla
                rts
                .endif

;-------------------------------------------------------------------------------
; SETLFSDEVICE
;
; Gets the last used device number and performs a SETLFS.
;
; Parameters: -
; Returns: -
; Modifies: X
;-------------------------------------------------------------------------------

setlfsdevice:   ldx fa
                jmp setlfs

;-------------------------------------------------------------------------------
; KERNALON
;
; Switches KERNAL on to prepare for slow loading. Saves state of $01.
;
; Parameters: -
; Returns: -
; Modifies: A,X
;-------------------------------------------------------------------------------

kernalon:       lda $01
                sta kernaloff+1
                lda useserial
                sta slowirq
                lda #$36
                sta $01
                rts

;-------------------------------------------------------------------------------
; CLOSE_KERNALOFF
;
; Closes file 2 and then restores state of $01.
;
; Parameters: -
; Returns: -
; Modifies: A
;-------------------------------------------------------------------------------

close_kernaloff:lda #$02
                jsr close
                jsr clrchn

;-------------------------------------------------------------------------------
; KERNALOFF
;
; Restores state of $01.
;
; Parameters: -
; Returns: -
; Modifies: A
;-------------------------------------------------------------------------------

kernaloff:      lda #$36
                sta $01
                lda #$00
                sta slowirq
il_ok:          rts

;-------------------------------------------------------------------------------
; INITFASTLOAD
;
; Uploads the fastloader to disk drive memory and starts it.
;
; Parameters: -
; Returns: -
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

initfastload:   lda usefastload         ;If fastloader not needed, do nothing
                beq il_ok
                lda fastloadstatus      ;If fastloader already initted,
                bne il_ok               ;do nothing
                inc fastloadstatus
                lda #<drivecode
                ldx #>drivecode
                ldy #(drvend-drvstart+MW_LENGTH-1)/MW_LENGTH

ifl_begin:      sta ifl_senddata+1
                stx ifl_senddata+2
                sty loadtempreg         ;Number of "packets" to send
                jsr kernalon
                lda #>drvstart
                sta ifl_mwstring+1
                ldy #$00
                sty ifl_mwstring+2      ;Drivecode starts at lowbyte 0
                beq ifl_nextpacket
ifl_sendmw:     lda ifl_mwstring,x      ;Send M-W command (backwards)
                jsr ciout
                dex
                bpl ifl_sendmw
                ldx #MW_LENGTH
ifl_senddata:   lda drivecode,y         ;Send one byte of drivecode
                jsr ciout
                iny
                bne ifl_notover
                inc ifl_senddata+2
ifl_notover:    inc ifl_mwstring+2      ;Also, move the M-W pointer forward
                bne ifl_notover2
                inc ifl_mwstring+1
ifl_notover2:   dex
                bne ifl_senddata
                jsr unlsn               ;Unlisten to perform the command
ifl_nextpacket: lda fa                  ;Set drive to listen
                jsr listen
                lda status
                cmp #$c0
                beq ifl_error           ;Abort if serial error (IDE64!)
                lda #$6f
                jsr second
                ldx #$05
                dec loadtempreg         ;All "packets" sent?
                bpl ifl_sendmw
ifl_sendme:     lda ifl_mestring-1,x    ;Send M-E command (backwards)
                jsr ciout
                dex
                bne ifl_sendme
                jsr unlsn
ifl_error:      jmp kernaloff

;-------------------------------------------------------------------------------
; DRIVECODE - Code executed in the disk drive.
;-------------------------------------------------------------------------------

drivecode:                              ;Address in C64's memory
                .org drvstart           ;Address in diskdrive's memory

drvmain:        cli                     ;File loop: Get filename first
                lda #$00                ;Set DATA & CLK high
drv_1800ac0:    sta $1800
                .if LONG_NAMES>0
                ldx #$00
                .else
                ldx #$01
                .endif
drv_nameloop:   ldy #$08                ;Bit counter
drv_namebitloop:
drv_1800ac1:    lda $1800
                bpl drv_noquit          ;Quit if ATN is low
                jmp drv_quit
drv_noquit:     and #$05                ;Wait for CLK or DATA going low
                beq drv_namebitloop
                lsr                     ;Read the data bit
                lda #$02                ;Pull the other line low to acknowledge
                bcc drv_namezero ;the bit being received
                lda #$08
drv_namezero:   ror drv_filename,x      ;Store the data bit
drv_1800ac2:    sta $1800
drv_namewait:
drv_1800ac3:    lda $1800               ;Wait for either line going high
                and #$05
                cmp #$05
                beq drv_namewait
                lda #$00
drv_1800ac4:    sta $1800               ;Set both lines high
                dey
                bne drv_namebitloop     ;Loop until all bits have been received
                sei                     ;Disable interrupts after first byte
                .if LONG_NAMES>0
                inx
                lda drv_filename-1,x    ;End of filename?
                bne drv_nameloop
                .else
                dex
                bpl drv_nameloop
                .endif

                .if TWOBIT_PROTOCOL=0
                lda #$08                ;CLK low, data isn't available
drv_1800ac5:    sta $1800
                .endif

drv_dirtrk:     ldx $1000
drv_dirsct:     ldy $1000               ;Read disk directory
drv_dirloop:    jsr drv_readsector      ;Read sector
                bcs drv_loadend         ;If failed, return error code
                ldy #$02
drv_nextfile:   lda buf,y               ;File type must be PRG
                and #$83
                cmp #$82
                bne drv_notfound
                .if LONG_NAMES>0
                ldx #$03
                sty drv_namecmploop+1
                lda #$a0                ;Make an endmark at the 16th letter
                sta buf+19,y
drv_namecmploop:lda buf,x               ;Check against each letter of filename,
                cmp drv_filename-3,x    ;break on mismatch
                bne drv_namedone
                inx
                bne drv_namecmploop
drv_namedone:   cmp #$a0                ;If got endmark in both filenames, found
                bne drv_notfound
                lda drv_filename-3,x
                beq drv_found
                .else
                lda buf+3,y
                cmp drv_filename
                bne drv_notfound
                lda buf+4,y
                cmp drv_filename+1
                beq drv_found
                .endif
drv_notfound:   tya
                clc
                adc #$20
                tay
                bcc drv_nextfile
                ldy buf+1               ;Go to next directory block, go on until no
                ldx buf                 ;more directory blocks
                bne drv_dirloop
drv_filenotfound:
                ldx #$02                ;Return code $02 = File not found
drv_loadend:    stx buf+2
                lda #$00
                sta buf
                sta buf+1
                beq drv_sendblk

drv_quit:                               ;If ATN, exit to drive ROM code
drv_drivetype:  ldx #$00
                bne drv_not1541
                jmp initialize

drv_not1541:    rts

drv_found:      iny
drv_nextsect:   ldx buf,y       ;File found, get starting track & sector
                beq drv_loadend ;At file's end? (return code $00 = OK)
                lda buf+1,y
                tay
                jsr drv_readsector      ;Read the data sector
                bcs drv_loadend
                
                .if TWOBIT_PROTOCOL>0

drv_sendblk:    ldy #$00
                ldx #$02
drv_sendloop:   lda buf,y
                lsr
                lsr
                lsr
                lsr
drv_1800ac5:    stx $1800               ;Set DATA=low for first byte, high for
                tax                     ;subsequent bytes
                lda drv_sendtbl,x
                pha
                lda buf,y
                and #$0f
                tax
                lda #$04
drv_1800ac6:    bit $1800               ;Wait for CLK=low
                beq drv_1800ac6
                lda drv_sendtbl,x
drv_1800ac7:    sta $1800
drv_2mhzsend:   jsr drv_delay18
                nop
                asl
                and #$0f
drv_2mhz1800ac8:sta $1800
                cmp ($00,x)
                nop
                pla
drv_2mhz1800ac9:sta $1800
                cmp ($00,x)
                nop
                asl
                and #$0f
drv_2mhz1800ac10:
                sta $1800
                ldx #$00
                iny
                bne drv_sendloop
                jsr drv_delay12
drv_2mhz1800ac11:
                stx $1800               ;Finish send: DATA & CLK both high

                .else

drv_sendblk:    lda #$04                ;Bitpair counter/
                ldx #$00                ;compare-value for CLK-line
drv_1800ac6:    stx $1800               ;CLK & DATA high -> ready to go
drv_sendloop:   ldx buf
drv_zpac1:      stx drvtemp
                tay                     ;Bitpair counter
drv_sendloop_bitpair:
                ldx #$00
drv_zpac2:      lsr drvtemp
                bcs drv_sendloop_wait1
                ldx #$02
drv_sendloop_wait1:
drv_1800ac7:    bit $1800               ;Wait until CLK high
                bne drv_sendloop_wait1
drv_1800ac8:    stx $1800
                ldx #$00
drv_zpac3:      lsr drvtemp
                bcs drv_sendloop_wait2
                ldx #$02
drv_sendloop_wait2:
drv_1800ac9:    bit $1800
                beq drv_sendloop_wait2  ;Wait until CLK low
drv_1800ac10:   stx $1800
                dey
                bne drv_sendloop_bitpair
                inc drv_sendloop+1
                bne drv_sendloop
drv_sendloop_endwait:
drv_1800ac11:   bit $1800               ;Wait for CLK high
                bne drv_sendloop_endwait
                asl                     ;Set CLK low, DATA high
drv_1800ac12:   sta $1800               ;(more data yet not ready)

                .endif

drv_senddone:   lda buf                 ;First 2 bytes zero marks end of loading
                ora buf+1               ;(3rd byte is the return code)
                bne drv_nextsect
                jmp drvmain

drv_readsector: jsr drv_led
drv_readtrk:    stx $1000
drv_readsct:    sty $1000
                ldy #RETRIES            ;Retry counter
drv_retry:      lda #$80
                ldx #1
drv_execjsr:    jsr drv_1541exec        ;Exec buffer 1 job
                cmp #$02                ;Error?
                bcc drv_success
drv_skipid:     dey                     ;Decrease retry counter
                bne drv_retry
drv_failure:    ldx #$01                ;Return code $01 - Read error
drv_success:    sei                     ;Make sure interrupts now disabled
drv_led:        lda #$08                ;Flash the drive LED
drv_ledac1:     eor $1c00
drv_ledac2:     sta $1c00
                rts

drv_1541exec:   sta $01
                cli                     ;Allow interrupts & execute command
drv_1541execwait:
                lda $01
                bmi drv_1541execwait
                pha
                lda id                  ;Handle disk ID change
                sta iddrv0
                lda id+1
                sta iddrv0+1
                pla
                rts

drv_fdexec:     jsr $ff54               ;FD2000 fix by Ninja
                lda $03
                rts

                .if TWOBIT_PROTOCOL>0
drv_delay18:    cmp ($00,x)
drv_delay12:    rts

drv_sendtbl:    .byte $0f,$07,$0d,$05
                .byte $0b,$03,$09,$01
                .byte $0e,$06,$0c,$04
                .byte $0a,$02,$08,$00
                .endif

drv_1541dirtrk: .byte 18
drv_1541dirsct: .byte 1
drv_1581dirsct: .byte 3
drv_filename:

drvend:
                .reloc

;-------------------------------------------------------------------------------
; M-W and M-E command strings
;-------------------------------------------------------------------------------

ifl_mwstring:   .byte MW_LENGTH,$00,$00,"W-M"

ifl_mestring:   .byte >drvstart, <drvstart, "E-M"

;-------------------------------------------------------------------------------
; Loader variables, if not on zeropage
;-------------------------------------------------------------------------------

                .if ADDITIONAL_ZEROPAGE=0
loadtempreg:    .byte 0          ;Temp variable for the loader
bufferstatus:   .byte 0          ;Bytes in fastload buffer
fileopen:       .byte 0          ;File open indicator
fastloadstatus: .byte 0          ;Fastloader active indicator
                .endif

;-------------------------------------------------------------------------------
; Filename (in short name mode)
;-------------------------------------------------------------------------------

                .if LONG_NAMES=0
filename:       .byte "00*"
                .endif

;-------------------------------------------------------------------------------
; Loader configuration
;-------------------------------------------------------------------------------

usefastload:    .byte 0                          ;If nonzero, fastloading will
                                                ;be used (autoconfigured)
useserial:      .byte 1                          ;If nonzero, serial protocol
                                                ;is in use and IRQs can't be
                                                ;used reliably while Kernal
                                                ;file I/O is in progress
slowirq:        .byte 0                          ;Indicator of whether IRQs are
                                                ;currently delayed

;-------------------------------------------------------------------------------
; Disposable portion of loader (routines only needed when initializing)
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; INITLOADER
;
; Inits the loadersystem. Must only be called only once in the beginning.
;
; Parameters: -
; Returns: -
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

initloader:     sta $d07f                       ;Disable SCPU hardware regs
                lda #$00
                sta messages                    ;Disable KERNAL messages
                sta fastloadstatus              ;Initial fastload status = off
                sta fileopen                    ;No file initially open

                .if TWOBIT_PROTOCOL>0
                tay
                sei                             ;Detect PAL/NTSC by measuring
il_detectntsc1: ldx $d011                       ;greatest rasterline number
                bmi il_detectntsc1
il_detectntsc2: ldx $d011
                bpl il_detectntsc2
il_detectntsc3: cpy $d012
                bcs il_detectntsc4
                ldy $d012
il_detectntsc4: ldx $d011
                bmi il_detectntsc3
                cli
                cpy #$20
                bcc il_isntsc
                lda #$2c                        ;Adjust 2-bit fastload transfer
                sta fastload_delay              ;delay
il_isntsc:
                .endif


il_detectdrive: lda #$aa
                sta $a5
                lda #<il_drivecode
                ldx #>il_drivecode
                ldy #(il_driveend-il_drivecode+MW_LENGTH-1)/MW_LENGTH
                jsr ifl_begin                   ;Upload test-drivecode
                lda status                      ;If serial error here, not a
                cmp #$c0                        ;serial device
                beq il_noserial
                ldx #$00
                ldy #$00
il_delay:       inx                             ;Delay to make sure the test-
                bne il_delay                    ;drivecode executed to the end
                iny
                bpl il_delay
                lda fa                          ;Set drive to listen
                jsr listen
                lda #$6f
                jsr second
                ldx #$05
il_ddsendmr:    lda il_mrstring,x               ;Send M-R command (backwards)
                jsr ciout
                dex
                bpl il_ddsendmr
                jsr unlsn
                lda fa
                jsr talk
                lda #$6f
                jsr tksa
                lda #$00
                jsr acptr                       ;First byte: test value
                pha
                jsr acptr                       ;Second byte: drive type
                tax
                jsr untlk
                pla
                cmp #$aa                        ;Drive can execute code, so can
                beq il_fastloadok               ;use fastloader
                lda $a5                         ;If serial bus delay counter
                cmp #$aa                        ;untouched, not a serial device
                bne il_nofastload
il_noserial:    dec useserial                   ;Serial bus not used: switch to
il_nofastload:  rts                             ;"fake" IRQ-loading mode

il_fastloadok:  inc usefastload
                stx il_drivetype+1
                stx drv_drivetype+1-drvstart+drivecode
                lda il_1800lo,x                 ;Perform patching of drivecode
                sta il_patch1800lo+1
                lda il_1800hi,x
                sta il_patch1800hi+1
                .if TWOBIT_PROTOCOL>0
                txa                             ;For 1MHz drives, need to copy
                bne il_2mhzdrive                ;the 1MHz transfer code
                ldy #drv_1mhzsenddone-drv_1mhzsend-1
il_copy1mhzcode:lda il_drv1mhzsend,y
                sta drv_2mhzsend-drvstart+drivecode,y
                dey
                bpl il_copy1mhzcode
                ldy #3
il_copy1mhz1800offsets:
                lda il_1mhz1800ofs,y            ;Also copy the serial port
                sta il_2mhz1800ofs,y            ;access offsets
                dey
                bpl il_copy1mhz1800offsets
il_2mhzdrive:   ldy #11
                .else
                ldy #12
                .endif
il_patchloop:   ldx il_1800ofs,y
il_patch1800lo: lda #$00                        ;Patch all $1800 accesses
                sta drvmain+1-drvstart+drivecode,x
il_patch1800hi: lda #$00
                sta drvmain+2-drvstart+drivecode,x
                dey
                bpl il_patchloop
il_drivetype:   ldx #$00
                lda il_dirtrklo,x               ;Patch directory
                sta drv_dirtrk+1-drvstart+drivecode
                lda il_dirtrkhi,x
                sta drv_dirtrk+2-drvstart+drivecode
                lda il_dirsctlo,x
                sta drv_dirsct+1-drvstart+drivecode
                lda il_dirscthi,x
                sta drv_dirsct+2-drvstart+drivecode
                lda il_execlo,x                 ;Patch job exec address
                sta drv_execjsr+1-drvstart+drivecode
                lda il_exechi,x
                sta drv_execjsr+2-drvstart+drivecode
                lda il_jobtrklo,x               ;Patch job track/sector
                sta drv_readtrk+1-drvstart+drivecode
                clc
                adc #$01
                sta drv_readsct+1-drvstart+drivecode
                lda il_jobtrkhi,x
                sta drv_readtrk+2-drvstart+drivecode
                adc #$00
                sta drv_readsct+2-drvstart+drivecode    
                .if TWOBIT_PROTOCOL=0
                lda il_zp,x                     ;Patch zeropage temp usage
                sta drv_zpac1+1-drvstart+drivecode
                sta drv_zpac2+1-drvstart+drivecode
                sta drv_zpac3+1-drvstart+drivecode
                .endif
                lda il_ledenabled,x             ;Patch LED flashing
                sta drv_led-drvstart+drivecode
                lda il_ledbit,x
                sta drv_led+1-drvstart+drivecode
                lda il_ledadrhi,x
                sta drv_ledac1+2-drvstart+drivecode
                sta drv_ledac2+2-drvstart+drivecode
                rts

;-------------------------------------------------------------------------------
; IL_DRIVECODE - Drivecode used to detect drive type & test if drivecode
; execution works OK
;-------------------------------------------------------------------------------

il_drivecode:
                .org drvstart

                asl ild_return1         ;Modify first returnvalue to prove
                                        ;we've executed something :)
                lda $fea0               ;Recognize drive family
                ldx #3                  ;(from Dreamload)
ild_floop:      cmp ild_family-1,x
                beq ild_ffound
                dex                     ;If unrecognized, assume 1541
                bne ild_floop
                beq ild_idfound
ild_ffound:     lda ild_idloclo-1,x
                sta ild_idlda+1
                lda ild_idlochi-1,x
                sta ild_idlda+2
ild_idlda:      lda $fea4               ;Recognize drive type
                ldx #3                  ;3 = CMD HD
ild_idloop:     cmp ild_id-1,x          ;2 = CMD FD
                beq ild_idfound         ;1 = 1581
                dex                     ;0 = 1541
                bne ild_idloop
ild_idfound:    stx ild_return2
                rts

ild_family:     .byte $43,$0d,$ff
ild_idloclo:    .byte $a4,$c6,$e9
ild_idlochi:    .byte $fe,$e5,$a6
ild_id:         .byte "8","F","H"

ild_return1:    .byte $55
ild_return2:    .byte 0

                .reloc

il_driveend:

;-------------------------------------------------------------------------------
; IL_DRV1MHZSEND - 2-bit protocol send code for 1MHz drives
;-------------------------------------------------------------------------------

                .if TWOBIT_PROTOCOL>0
il_drv1mhzsend:
                .org drv_2mhzsend

drv_1mhzsend:   asl
                and #$0f
drv_1mhz1800ac8:sta $1800
                pla
drv_1mhz1800ac9:sta $1800
                asl
                and #$0f
drv_1mhz1800ac10:
                sta $1800
                ldx #$00
                iny
                bne drv_sendloop
                nop
drv_1mhz1800ac11:
                stx $1800               ;Finish send: DATA & CLK both high
                beq drv_senddone
drv_1mhzsenddone:

                .reloc
                .endif

il_mrstring:    .byte 2,>ild_return1,<ild_return1,"R-M"

                .if TWOBIT_PROTOCOL>0

il_1800ofs:     .byte drv_1800ac0-drvmain
                .byte drv_1800ac1-drvmain
                .byte drv_1800ac2-drvmain
                .byte drv_1800ac3-drvmain
                .byte drv_1800ac4-drvmain
                .byte drv_1800ac5-drvmain
                .byte drv_1800ac6-drvmain
                .byte drv_1800ac7-drvmain
il_2mhz1800ofs: .byte drv_2mhz1800ac8-drvmain
                .byte drv_2mhz1800ac9-drvmain
                .byte drv_2mhz1800ac10-drvmain
                .byte drv_2mhz1800ac11-drvmain

il_1mhz1800ofs: .byte drv_1mhz1800ac8-drvmain
                .byte drv_1mhz1800ac9-drvmain
                .byte drv_1mhz1800ac10-drvmain
                .byte drv_1mhz1800ac11-drvmain

                .else

il_1800ofs:     .byte drv_1800ac0-drvmain
                .byte drv_1800ac1-drvmain
                .byte drv_1800ac2-drvmain
                .byte drv_1800ac3-drvmain
                .byte drv_1800ac4-drvmain
                .byte drv_1800ac5-drvmain
                .byte drv_1800ac6-drvmain
                .byte drv_1800ac7-drvmain
                .byte drv_1800ac8-drvmain
                .byte drv_1800ac9-drvmain
                .byte drv_1800ac10-drvmain
                .byte drv_1800ac11-drvmain
                .byte drv_1800ac12-drvmain

                .endif

il_1800lo:      .byte <$1800,<$4001,<$4001,<$8000
il_1800hi:      .byte >$1800,>$4001,>$4001,>$8000

il_dirtrklo:    .byte <drv_1541dirtrk,<$022b,<$54,<$2ba7
il_dirtrkhi:    .byte >drv_1541dirtrk,>$022b,>$54,>$2ba7
il_dirsctlo:    .byte <drv_1541dirsct,<drv_1581dirsct,<$56,<$2ba9
il_dirscthi:    .byte >drv_1541dirsct,>drv_1581dirsct,>$56,>$2ba9

il_execlo:      .byte <drv_1541exec,<$ff54,<drv_fdexec,<$ff4e
il_exechi:      .byte >drv_1541exec,>$ff54,>drv_fdexec,>$ff4e

il_jobtrklo:    .byte <$0008,<$000d,<$000d,<$2802
il_jobtrkhi:    .byte >$0008,>$000d,>$000d,>$2802

                .if TWOBIT_PROTOCOL=0
il_zp:          .byte $06,$0b,$0b,$06
                .endif

il_ledenabled:  .byte $a9,$a9,$a9,$60
il_ledbit:      .byte $08,$40,$40,$00
il_ledadrhi:    .byte $1c,$40,$40,$40
