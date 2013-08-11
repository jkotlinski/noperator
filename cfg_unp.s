;-------------------------------------------------------------------------------
; Default configuration (everything included)
;-------------------------------------------------------------------------------

.define LONG_NAMES       1             ;Set to nonzero to use long names (pointer in
                                ;X,Y) or zero to use 2-letter names (letters
                                ;in X,Y)
.define BORDER_FLASHING  0             ;Set to nonzero to enable border flashing
                                ;when fastloading :)
.define ADDITIONAL_ZEROPAGE  1         ;Set to nonzero to use additional zeropage
                                ;variables to shorten loader code
.define LOAD_UNDER_IO    1             ;Set to nonzero to enable possibility to load
                                ;under I/O areas, and to load packed data
                                ;under the Kernal ROM.
.define LOADFILE_UNPACKED  1           ;Set to nonzero to include unpacked loading
.define LOADFILE_EXOMIZER  0           ;Set to nonzero to include EXOMIZER loading
.define LOADFILE_PUCRUNCH  0           ;Set to nonzero to include PUCRUNCH loading

.define RETRIES          5             ;Retries when reading a sector

.define loadbuffer       $0f00         ;256 byte table used by fastloader & REU

.define depackbuffer     $0500         ;156 bytes for EXOMIZER tables, 31 for
                                ;PUCRUNCH.

.define zpbase           $74           ;Zeropage base address. Loader needs 2
                                ;addresses with unpacked, 3 with PUCRUNCH
                                ;and 8 with EXOMIZER loading.

.define zpbase2          $7c           ;Additional 4 zeropage addresses for shortening
                                ;the loader code (optional)
