CC65BASE = /usr/local/lib/cc65

CRT0   	= $(CC65BASE)/lib/c64.o
CLIB	= $(CC65BASE)/lib/c64.lib
CC	= cc65 -Or -O --create-dep -Cl -tc64 -T -I $(CC65BASE)/include/ 
# CC	= cc65 --create-dep -Cl -tc64 -T -I $(CC65BASE)/include/ 
AS	= ca65 --cpu 6502x # -l
LD	= ld65 -C nop2.cfg -m nop2.map -Ln nop2.lbl 
C1541  	= ~/bin/c1541
DEPDIR = .dep


all:   	nop2

# --------------------------------------------------------------------------
# Generic rules

%.o : %.c

%.a : %.c
	@echo $<
	@$(CC) -o $(basename $<).a $(basename $<).c
	@mkdir -p $(DEPDIR)
	@mv $(basename $<).u $(DEPDIR)/

%.o : %.a
	@$(AS) $(basename $<).a

# Don't delete intermediate .a files.
.PRECIOUS : %.a 

%.o : %.s
	@echo $<
	@$(AS) $(basename $<).s

OBJS = main.o

-include $(OBJS:%.o=$(DEPDIR)/%.u)

# --------------------------------------------------------------------------
# Rules how to make each one of the binaries

EXELIST=nop2

nop2.d64:
	$(C1541) -format nop2,AA  d64 nop2.d64 > /dev/null

nop2: 		$(OBJS) $(CLIB) nop2.d64
	@$(LD) -o $@ $(OBJS) $(CLIB)
	@for exe in $(EXELIST); do\
	    $(C1541) -attach nop2.d64 -delete $$exe  > /dev/null;\
	    $(C1541) -attach nop2.d64 -write $$exe  > /dev/null;\
	done;

run: nop2
	x64 nop2.d64

# --------------------------------------------------------------------------
# Cleanup rules

.PHONY:	clean
clean:
	rm -f $(EXELIST) *.d64 *.map *.o *.lbl *.prg *.lst *.a *.u $(DEPDIR)/*
	
# ------------------

