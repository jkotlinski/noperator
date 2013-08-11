;-------------------------------------------------------------------------------
; COVERT BITOPS Autoconfiguring Loader/Depacker V2.2
; with REU+SuperRAM, and 1541/1571/1581/CMD FD/CMD HD support
;
; EXOMIZER compressor by Magnus Lind
; PUCRUNCH compressor by Pasi Ojala
; 1581/CMD FD/CMD HD information from Ninja & DocBacardi /The Dreams
; Rest by Lasse ™”rni
;
; Thanks to K.M/TABOO for inspiration on badline detection and 1-bit transfer,
; and Marko M„kel„ for his original irqloader.s (huge inspiration)
;-------------------------------------------------------------------------------

.define processor 6502

.export initloader
.export loadfile

.segment "CODE"

;-------------------------------------------------------------------------------
; Include your loader configuration file at this point!
;-------------------------------------------------------------------------------

.include "cfg_unp.s"

;-------------------------------------------------------------------------------
; Defines derived from the compile options (need not be changed)
;-------------------------------------------------------------------------------

.if ADDITIONAL_ZEROPAGE>0
.define loadtempreg     zpbase2+0      ;Temp variables for the loader
.define bufferstatus    zpbase2+1      ;Bytes in REU/fastload buffer
.define fileopen        zpbase2+2      ;File open indicator
.define fastloadstatus  zpbase2+3      ;Fastloader active indicator
.endif

.define destlo          zpbase+0
.define desthi          zpbase+1

;-------------------------------------------------------------------------------
; Other defines
;-------------------------------------------------------------------------------

.define MW_LENGTH       32            ;Bytes in one M-W command

.define status          $90           ;Kernal zeropage variables
.define messages        $9d
.define fa              $ba

.define acsbf           $01           ;Diskdrive variables: Buffer 1 command
.define trkbf           $08           ;Buffer 1 track
.define sctbf           $09           ;Buffer 1 sector
.define iddrv0          $12           ;Disk drive ID
.define drvtemp         $06           ;Temp variable
.define id              $16           ;Disk ID
.define buf             $0400         ;Sector data buffer
.define drvstart        $0500         ;Start of drivecode
.define initialize      $d005         ;Initialize routine in 1541 ROM

.define ciout           $ffa8         ;Kernal routines
.define listen          $ffb1
.define second          $ff93
.define unlsn           $ffae
.define talk            $ffb4
.define tksa            $ff96
.define untlk           $ffab
.define acptr           $ffa5
.define chkin           $ffc6
.define chkout          $ffc9
.define chrin           $ffcf
.define chrout          $ffd2
.define close           $ffc3
.define open            $ffc0
.define setmsg          $ff90
.define setnam          $ffbd
.define setlfs          $ffba
.define clrchn          $ffcc
.define getin           $ffe4
.define load            $ffd5
.define save            $ffd8

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
; Loads a file packed with EXOMIZER
;
; Parameters: X (low),Y (high): Address of null-terminated filename
; Returns: C=0 OK, C=1 error (A holds errorcode)
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

.define tabl_bi         depackbuffer
.define tabl_lo         depackbuffer+52
.define tabl_hi         depackbuffer+104

.define zp_len_lo       zpbase+0
.define zp_src_lo       zpbase+1
.define zp_src_hi       zpbase+2
.define zp_bits_lo      zpbase+3
.define zp_bits_hi      zpbase+4
.define zp_bitbuf       zpbase+5
.define zp_dest_lo      zpbase+6
.define zp_dest_hi      zpbase+7

loadfile_exomizer:
                jsr openfile
                tsx
                stx exomizer_stackptr+1
                jsr exomizer_depack
                clc
                rts

; -------------------------------------------------------------------
; EXOMIZER depackroutine by Magnus Lind
; Modified by Lasse ™”rni for interrupt disabling (loading under I/O)
; and to call getbyte directly without need for a "wrapper" routine
; that slows down decrunching/loading
; -------------------------------------------------------------------
;
; Copyright (c) 2002 Magnus Lind.
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
; exodecruncher.s, a part of the exomizer v1.0 release
; -------------------------------------------------------------------
; no code below this comment has to be modified in order to generate
; a working decruncher of this source file.
; However, you may want to relocate the tables last in the file to a
; more suitable adress.
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; jsr this label to decrunch, it will in turn init the tables and
; call the decruncher
; no constraints on register content, however the
; decimal flag has to be #0 (it almost always is, otherwise do a cld)
exomizer_depack:
; -------------------------------------------------------------------
; init zeropage (12 bytes)
;
        ldy #0
        ldx #3
init_zp:
        php
        jsr getbyte
        bcs exomizer_fail
        plp
        sta zp_bitbuf - 1,x
        dex
        bne init_zp
; -------------------------------------------------------------------
; calculate tables (53 bytes)
; x and y must be #0 when entering
;
nextone:
        inx
        tya
        and #$0f
        beq shortcut                ; starta på ny sekvens

        lda zp_bits_lo
        adc tabl_lo-1,y
        tax

        lda zp_bits_hi
        adc tabl_hi-1,y
shortcut:
        sta tabl_hi,y
        txa
        sta tabl_lo,y

        ldx #4
        jsr get_bits                ; clears x-reg.
        sta tabl_bi,y

        stx zp_bits_lo                ; zp_bits_hi is already 0 because
        tax                        ; we called get_bits with x = 4
        inx
        sec
rolle:
        rol zp_bits_lo
        rol zp_bits_hi
        dex
        bne rolle                ; c = 0 after this
skip:
        iny
        cpy #52
        bne nextone
        ldy #0
        beq begin
; -------------------------------------------------------------------
; get x + 1 bits (1 byte)
;
get_bit1:
        inx
; -------------------------------------------------------------------
; get bits (31 bytes)
;
; args:
;   x = number of bits to get
; returns:
;   a = #bits_lo
;   x = #0
;   c = 0
;   zp_bits_lo = #bits_lo
;   zp_bits_hi = #bits_hi
; notes:
;   y is untouched
;   other status bits are set to (a == #0)
; -------------------------------------------------------------------
get_bits:
        lda #$00
        sta zp_bits_lo
        sta zp_bits_hi
        cpx #$01
        bcc bits_done
        lda zp_bitbuf
bits_next:
        lsr
        bne bits_ok
        php
        jsr getbyte
        bcs exomizer_fail
        plp
        ror
bits_ok:
        rol zp_bits_lo
        rol zp_bits_hi
        dex
        bne bits_next
        sta zp_bitbuf
        lda zp_bits_lo
bits_done:
        rts
; -------------------------------------------------------------------
; Failure when reading a byte: restore stackptr and exit
;
exomizer_fail:                          ;A premature EOF is treated as an
exomizer_stackptr:                      ;error; return directly to caller
        ldx #$00
        txs
        rts
; -------------------------------------------------------------------
; main copy loop (16 bytes)
;
copy_next_hi:
        dex
        dec zp_dest_hi
        dec zp_src_hi
copy_next:
        dey
        .if LOAD_UNDER_IO>0
        jsr disableio
        .endif
        lda (zp_src_lo),y
literal_entry:
        sta (zp_dest_lo),y
        .if LOAD_UNDER_IO>0
        jsr enableio
        .endif
copy_start:
        tya
        bne copy_next
        txa
        bne copy_next_hi
; -------------------------------------------------------------------
; decruncher entry point, needs calculated tables (5 bytes)
; x and y must be #0 when entering
;
begin:
        jsr get_bit1
        beq sequence
; -------------------------------------------------------------------
; literal handling (13 bytes)
;
literal_start:
        lda zp_dest_lo
        bne avoid_hi
        dec zp_dest_hi
avoid_hi:
        dec zp_dest_lo
        php
        jsr getbyte
        bcs exomizer_fail
        plp
        .if LOAD_UNDER_IO>0
        bcs sequence
        jsr disableio
        .endif
        bcc literal_entry
; -------------------------------------------------------------------
; count zero bits + 1 to get length table index (10 bytes)
; y = x = 0 when entering
;
sequence:
next1:
        iny
        jsr get_bit1
        beq next1
        cpy #$11
        bcs bits_done
; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (17 bytes)
;
        ldx tabl_bi - 1,y
        jsr get_bits
        adc tabl_lo - 1,y
        sta zp_len_lo
        lda zp_bits_hi
        adc tabl_hi - 1,y
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
        ldx tabl_bit - 1,y
        jsr get_bits
        adc tabl_off - 1,y
        tay
; -------------------------------------------------------------------
; prepare zp_dest (11 bytes)
;
        sec
        lda zp_dest_lo
        sbc zp_len_lo
        sta zp_dest_lo
        bcs noborrow
        dec zp_dest_hi
noborrow:
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
; prepare for copy loop (6 bytes)
;
        ldy zp_len_lo
        pla
        tax
        bcc copy_start
; -------------------------------------------------------------------
; two small static tables (6 bytes)
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

.define lzpos           zpbase
.define bitstr          zpbase+2

.define table           depackbuffer

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

getbyt: jsr getnew
        lda bitstr
        ror
        rts

newesc: ldy esc+1       ; remember the old code (top bits for escaped byte)
escB0:  ldx #2          ; ** PARAMETER  0..8
        jsr getchkf     ; get & save the new escape code
        sta esc+1
        tya             ; pre-set the bits
        ; Fall through and get the rest of the bits.
noesc:   ldx #6          ; ** PARAMETER  8..0
        jsr getchkf
        jsr putch       ; output the escaped/normal byte
        ; Fall through and check the escape bits again
main:    ldy #0          ; Reset to a defined state
        tya             ; A = 0
escB1:   ldx #2          ; ** PARAMETER  0..8
        jsr getchkf     ; X = 0
esc:     cmp #0
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
srle:    iny             ; Y is 1 bigger than MSB loops
        jsr getval      ; Y is 1, get len, X = 0
        sta lzpos       ; xstore - Save length LSB
mg1:     cmp #64         ; ** PARAMETER 63-64 -> C clear, 64-64 -> C set..
        bcc chrcode     ; short RLE, get bytecode

longrle: ldx #2          ; ** PARAMETER  111111xxxxxx
        jsr getbits     ; get 3/2/1 more bits to get a full byte, X = 0
        sta lzpos       ; xstore - Save length LSB

        jsr getval      ; length MSB, X = 0
        tay             ; Y is 1 bigger than MSB loops

chrcode: jsr getval      ; Byte Code, X = 0
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
dorle:   jsr putch
        dex
        bne dorle       ; xstore 0..255 -> 1..256
        dey
        bne dorle       ; Y was 1 bigger than wanted originally
mainbeq: beq main        ; reverse condition -> jump always


lz77:   jsr getval      ; X = 0
mg21:   cmp #127        ; ** PARAMETER  Clears carry (is maximum value)
        bne noeof
eof:    clc             ; Loading ended OK
        rts

noeof:  sbc #0          ; C is clear -> subtract 1  (1..126 -> 0..125)
elzpb:  ldx #0          ; ** PARAMETER (more bits to get)
        jsr getchkf     ; clears Carry, X = 0

lz77_2: sta lzpos+1     ; offset MSB
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

lzslow: .if LOAD_UNDER_IO>0
        jsr disableio
        .endif
        lda (lzpos),y   ; using abs,y is 3 bytes longer, only 1 cycle/byte faster
        jsr outpos
        iny             ; Y does not wrap because X=0..255 and Y initially 0
        dex
        bne lzslow      ; X loops, (256,1..255)
        jmp main

putch:  .if LOAD_UNDER_IO>0
        jsr disableio
        .endif
outpos: sta $aaaa       ; ** parameter
        inc outpos+1    ; ZP
        bne putchok
        inc outpos+2    ; ZP
putchok:.if LOAD_UNDER_IO>0
        jmp enableio
        .else
        rts
        .endif

getnew: pha             ; 1 Byte/3 cycles
        jsr getbyte
        bcs pucrunch_fail
        sec
        rol             ; Shift out the next bit and
                        ;  shift in C=1 (last bit marker)
        sta bitstr      ; bitstr initial value = $80 == empty
        pla             ; 1 Byte/4 cycles
        rts
        ; 25+12 = 37

; getval : Gets a 'static huffman coded' value
; ** Scratches X, returns the value in A **
getval: inx             ; X <- 1
        txa             ; set the top bit (value is 1..255)
gv0:    asl bitstr
        bne @1
        jsr getnew
@1:     bcc getchk      ; got 0-bit
        inx
mg:     cpx #7          ; ** PARAMETER unary code maximum length + 1
        bne gv0
        beq getchk      ; inverse condition -> jump always
        ; getval: 18 bytes
        ; 15 + 17*n + 6+15*n+12 + 36*n/8 = 33 + 32*n + 36*n/8 cycles

; getbits: Gets X bits from the stream
; ** Scratches X, returns the value in A **
getbyt2:ldx #7
get1bit:inx             ;2
getbits:asl bitstr
        bne @1
        jsr getnew
@1:     rol             ;2
getchk: dex             ;2              more bits to get ?
getchkf:bne getbits     ;2/3
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
; Marko M„kel„'s work when sending the filename.
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
                sta $d07a                 ;SCPU to slow mode
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
                sei
                lda #$34
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
; INITFASTLOADER
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
                and #$ff-$40
                bne ifl_error           ;Abort if serial error (IDE64!)
                lda #$6f
                jsr second
                ldx #$05
                dec loadtempreg         ;All "packets" sent?
                bpl ifl_sendmw
ifl_sendme:     lda ifl_mestring-1,x      ;Send M-E command (backwards)
                jsr ciout
                dex
                bne ifl_sendme
                jsr unlsn
ifl_error:      jmp kernaloff

;-------------------------------------------------------------------------------
; DRIVECODE - Code executed in the disk drive.
;-------------------------------------------------------------------------------

drivecode:                      ;Address in C64's memory
                .org drvstart   ;Address in diskdrive's memory

drvmain:        cli             ;File loop: Get filename first
                .if LONG_NAMES>0
                ldx #$00
                .else
                ldx #$01
                .endif
drv_nameloop:   ldy #$08        ;Bit counter
drv_namebitloop:
drv_1800ac1:    lda $1800
                bmi drv_quit    ;Quit if ATN is low
                and #$05        ;Wait for CLK or DATA going low
                beq drv_namebitloop
                lsr                ;Read the data bit
                lda #$02        ;Pull the other line low to acknowledge
                bcc drv_namezero ;the bit being received
                lda #$08
drv_namezero:   ror drv_filename,x ;Store the data bit
drv_1800ac2:    sta $1800
drv_namewait:
drv_1800ac3:    lda $1800        ;Wait for either line going high
                and #$05
                cmp #$05
                beq drv_namewait
                lda #$00
drv_1800ac4:    sta $1800        ;Set both lines high
                dey
                bne drv_namebitloop ;Loop until all bits have been received
                sei             ;Disable interrupts after first byte
                .if LONG_NAMES>0
                inx
                lda drv_filename-1,x ;End of filename?
                bne drv_nameloop
                .else
                dex
                bpl drv_nameloop
                .endif
                lda #$08            ;CLK low, data isn't available
drv_1800ac5:    sta $1800
drv_dirtrk:     ldx $1000
drv_dirsct:     ldy $1000           ;Read disk directory
drv_dirloop:    jsr drv_readsector  ;Read sector
                bcs drv_loadend     ;If failed, return error code
                ldy #$02
drv_nextfile:   lda buf,y       ;File type must be PRG
                and #$83
                cmp #$82
                bne drv_notfound
                .if LONG_NAMES>0
                ldx #$03
                sty drv_namecmploop+1
                lda #$a0                ;Make an endmark at the 16th letter
                sta buf+19,y
drv_namecmploop:lda buf,x
                cmp drv_filename-3,x    ;Check against each letter of filename
                bne drv_namedone        ;until at the endzero
                inx
                bne drv_namecmploop
drv_namedone:   cmp #$a0                ;If got to a $a0, name correct
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
                ldy buf+1       ;Go to next directory block, go on until no
                ldx buf                ;more directory blocks
                bne drv_dirloop
drv_filenotfound:
                ldx #$02        ;Return code $02 = File not found
drv_loadend:    stx buf+2
                lda #$00
                sta buf
                sta buf+1
                beq drv_sendblk

drv_quit:                       ;If ATN, exit to drive ROM code
drv_drivetype:  ldx #$00
                bne drv_not1541
                jmp initialize

drv_not1541:    rts

drv_found:      iny
drv_nextsect:   ldx buf,y       ;File found, get starting track & sector
                beq drv_loadend ;At file's end? (return code $00 = OK)
                lda buf+1,y
                tay
                jsr drv_readsector ;Read the data sector
                bcs drv_loadend
drv_sendblk:    lda #$04                  ;Bitpair counter/
                ldx #$00                  ;compare-value for CLK-line
drv_1800ac6:    stx $1800                 ;CLK & DATA high -> ready to go
drv_sendloop:   ldx buf
drv_zpac1:      stx drvtemp
                tay                       ;Bitpair counter
drv_sendloop_bitpair:
                ldx #$00
drv_zpac2:      lsr drvtemp
                bcs drv_sendloop_wait1
                ldx #$02
drv_sendloop_wait1:
drv_1800ac7:    bit $1800                 ;Wait until CLK high
                bne drv_sendloop_wait1
drv_1800ac8:    stx $1800
                ldx #$00
drv_zpac3:      lsr drvtemp
                bcs drv_sendloop_wait2
                ldx #$02
drv_sendloop_wait2:
drv_1800ac9:    bit $1800
                beq drv_sendloop_wait2    ;Wait until CLK low
drv_1800ac10:   stx $1800
                dey
                bne drv_sendloop_bitpair
                inc drv_sendloop+1
                bne drv_sendloop
drv_sendloop_endwait:
drv_1800ac11:   bit $1800                 ;Wait for CLK high
                bne drv_sendloop_endwait
                asl                       ;Set CLK low, DATA high
drv_1800ac12:   sta $1800                 ;(more data yet not ready)

drv_senddone:   lda buf                   ;First 2 bytes zero marks end of loading
                ora buf+1                 ;(3rd byte is the return code)
                bne drv_nextsect
drv_1800ac13:   lda $1800                 ;Set CLK high, preserve DATA
                and #$02
drv_1800ac14:   sta $1800
                lda #$04
drv_endwaitclk:
drv_1800ac15:   bit $1800       ;Wait for CLK to go high
                bne drv_endwaitclk
                lda #$00        ;Set DATA high
drv_1800ac16:   sta $1800
                jmp drvmain

drv_readsector:
drv_readtrk:    stx $1000
drv_readsct:    sty $1000
                ldy #RETRIES              ;Retry counter
drv_retry:      lda #$80
                ldx #1
drv_execjsr:    jsr drv_1541exec          ;Exec buffer 1 job
                cmp #$02                  ;Error?
                bcc drv_success
drv_skipid:     dey                       ;Decrease retry counter
                bne drv_retry
drv_failure:    ldx #$01                  ;Return code $01 - Read error
drv_success:    sei                       ;Make sure interrupts now disabled
                rts

drv_1541exec:   sta $01
                lda $1c00               ;Led on
                ora #$08
                sta $1c00
                cli                     ;Allow interrupts & execute command
drv_1541execwait:
                lda $01
                bmi drv_1541execwait
                pha
                lda id                  ;Handle disk ID change
                sta iddrv0
                lda id+1
                sta iddrv0+1
                lda $1c00               ;Led off
                and #$f7
                sta $1c00
                pla
                rts

drv_fdexec:     jsr $ff54               ;FD2000 fix by Ninja
                lda $03
                rts

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

usefastload:    .byte 1                          ;If nonzero, fastloading will
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
; Inits the loadersystem. Must only be called only *once* in the beginning.
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

il_detectdrive: lda #<il_drivecode
                ldx #>il_drivecode
                ldy #(il_driveend-il_drivecode+MW_LENGTH-1)/MW_LENGTH
                jsr ifl_begin             ;Upload test-drivecode
                lda status                ;If serial error here, assume fast
                and #$ff-$40              ;drive emulation
                bne il_noserial
                lda $dd00                 ;Set CLK/DATA low
                ora #$30
                sta $dd00
                lda #<ild_return1
                ldx #>ild_return1
                jsr il_getdrivebyte       ;Read test variable
                cpx #$aa                  ;Drive can execute code, so can
                beq il_fastloadok         ;use fastloader
                cpx #$55                  ;See if original byte can be read
                bne il_noserial           ;back
                lda $dd00                 ;If CLK/DATA stayed low, it's
                and #$30                  ;fast drive emulation
                cmp #$30
                bne il_serial
il_noserial:    dec useserial
il_serial:      rts

il_fastloadok:  inc usefastload
                lda #<ild_return2
                ldx #>ild_return2
                jsr il_getdrivebyte       ;Read drivetype
                stx il_drivetype+1
                stx drv_drivetype+1-drvstart+drivecode
                lda il_1800lo,x           ;Perform patching of drivecode
                sta il_patch1800lo+1
                lda il_1800hi,x
                sta il_patch1800hi+1
                ldy #15
il_patchloop:   ldx il_1800ofs,y
il_patch1800lo: lda #$00                  ;Patch all $1800 accesses
                sta drvmain+1-drvstart+drivecode,x
il_patch1800hi: lda #$00
                sta drvmain+2-drvstart+drivecode,x
                dey
                bpl il_patchloop
il_drivetype:   ldx #$00
                lda il_dirtrklo,x        ;Patch directory
                sta drv_dirtrk+1-drvstart+drivecode
                lda il_dirtrkhi,x
                sta drv_dirtrk+2-drvstart+drivecode
                lda il_dirsctlo,x
                sta drv_dirsct+1-drvstart+drivecode
                lda il_dirscthi,x
                sta drv_dirsct+2-drvstart+drivecode
                lda il_execlo,x          ;Patch job exec address
                sta drv_execjsr+1-drvstart+drivecode
                lda il_exechi,x
                sta drv_execjsr+2-drvstart+drivecode
                lda il_jobtrklo,x        ;Patch job track/sector
                sta drv_readtrk+1-drvstart+drivecode
                clc
                adc #$01
                sta drv_readsct+1-drvstart+drivecode
                lda il_jobtrkhi,x
                sta drv_readtrk+2-drvstart+drivecode
                adc #$00
                sta drv_readsct+2-drvstart+drivecode
                lda il_zp,x              ;Patch zeropage temp usage
                sta drv_zpac1+1-drvstart+drivecode
                sta drv_zpac2+1-drvstart+drivecode
                sta drv_zpac3+1-drvstart+drivecode
                rts

;-------------------------------------------------------------------------------
; IL_GETDRIVEBYTE
;
; Reads a byte from diskdrive memory using M-R
;
; Parameters: A,X:Address
; Returns: X:Byte
; Modifies: A,X,Y
;-------------------------------------------------------------------------------

il_getdrivebyte:sta il_mrstring+2
                stx il_mrstring+1
                lda fa                    ;Set drive to listen
                jsr listen
                lda #$6f
                jsr second
                ldx #$05
il_gdbsendmr:   lda il_mrstring,x         ;Send M-R command (backwards)
                jsr ciout
                dex
                bpl il_gdbsendmr
                jsr unlsn
                lda fa
                jsr talk
                lda #$6f
                jsr tksa
                lda #$00
                jsr acptr
                tax
                jmp untlk

;-------------------------------------------------------------------------------
; IL_DRIVECODE - Drivecode used to detect drive type & test if drivecode
; execution works OK
;-------------------------------------------------------------------------------

il_drivecode:
                .org drvstart

                asl ild_return1           ;Modify first returnvalue to prove
                                          ;we've executed something :)
                lda $fea0                 ;Recognize drive family
                ldx #3                    ;(from Dreamload)
ild_floop:      cmp ild_family-1,x
                beq ild_ffound
                dex                       ;If unrecognized, assume 1541
                bne ild_floop
                beq ild_idfound
ild_ffound:     lda ild_idloclo-1,x
                sta ild_idlda+1
                lda ild_idlochi-1,x
                sta ild_idlda+2
ild_idlda:      lda $fea4                 ;Recognize drive type
                ldx #3                    ;3 = CMD HD
ild_idloop:     cmp ild_id-1,x            ;2 = CMD FD
                beq ild_idfound           ;1 = 1581
                dex                       ;0 = 1541
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

il_mrstring:    .byte 1,$00,$00,"R-M"

il_1800ofs:     .byte drv_1800ac1-drvmain
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
                .byte drv_1800ac13-drvmain
                .byte drv_1800ac14-drvmain
                .byte drv_1800ac15-drvmain
                .byte drv_1800ac16-drvmain

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

il_zp:          .byte $06,$0b,$0b,$06
















