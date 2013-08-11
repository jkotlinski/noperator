;-------------------------------------------------------------------------------
; Default configuration (everything included)
;-------------------------------------------------------------------------------

TWOBIT_PROTOCOL   = 0           ;Nonzero to use 2-bit protocol which may delay
                                ;interrupts and does not allow sprites, but is
                                ;faster. Zero to use 1-bit protocol which is
                                ;the opposite.
LONG_NAMES      = 1             ;Set to nonzero to use long names (pointer in
                                ;X,Y) or zero to use 2-letter names (letters
                                ;in X,Y)
BORDER_FLASHING = 0             ;Set to nonzero to enable border flashing
                                ;when fastloading :)
ADDITIONAL_ZEROPAGE = 1         ;Set to nonzero to use additional zeropage
                                ;variables to shorten loader code
LOAD_UNDER_IO   = 1             ;Set to nonzero to enable possibility to load
                                ;under I/O areas, and to load packed data
                                ;under the Kernal ROM.
LOADFILE_UNPACKED = 1           ;Set to nonzero to include unpacked loading
LOADFILE_EXOMIZER = 1           ;Set to nonzero to include EXOMIZER loading
LOADFILE_PUCRUNCH = 1           ;Set to nonzero to include PUCRUNCH loading

LITERAL_SEQUENCES_NOT_USED = 0  ;(EXOMIZER only): set to nonzero for shorter 
                                ;depacker, if you use -c switch to disable 
                                ;literal sequences in Exomizer 2, or if you 
                                ;use Exomizer 1.
FORWARD_DECRUNCHING = 0         ;(EXOMIZER only): set to nonzero if you use -f
                                ;switch in Exomizer 2, zero for Exomizer 1.

RETRIES         = 5             ;Retries when reading a sector

loadbuffer      = $0400         ;256 byte table used by fastloader

depackbuffer    = $0500         ;156 bytes for EXOMIZER tables, 31 for
                                ;PUCRUNCH.

zpbase          = $74           ;Zeropage base address. Loader needs 2
                                ;addresses with unpacked, 3 with PUCRUNCH
                                ;and 8 with EXOMIZER loading.

zpbase2         = $7c           ;Additional 4 zeropage addresses for shortening
                                ;the loader code (optional)
