CRT0   	= $(CC65_HOME)/lib/c64.o
CLIB	= $(CC65_HOME)/lib/c64.lib
CC	= cc65 -Or -O -Cl -tc64 -T -I $(CC65_HOME)/include/
# CC	= cc65 --create-dep -Cl -tc64 -T -I $(CC65BASE)/include/
AS	= ca65 --cpu 6502x # -l
LD	= ld65 -C nop.cfg -m nop.map -Ln nop.lbl
PUCRUNCH = ~/bin/pucrunch
C1541  	= c1541
DEPDIR = .dep


all:   	nop

# --------------------------------------------------------------------------
# Generic rules

%.o : %.c

%.a : %.c
	@echo $<
	@mkdir -p $(DEPDIR)
	@$(CC) --create-dep $(DEPDIR)/$(basename $<).u -o $(basename $<).a $(basename $<).c

%.o : %.a
	@$(AS) $(basename $<).a

# Don't delete intermediate .a files.
.PRECIOUS : %.a

%.o : %.s
	@echo $<
	@$(AS) $(basename $<).s

OBJS = main.o keyframe.o anim.o irq.o loader.o fastload.o screen.o disk.o keybuf.o keyhandler.o rledec.o music.o \
	   movie.o opt.o font.o rotchars.o rotchars_p.o

-include $(OBJS:%.o=$(DEPDIR)/%.u)

# --------------------------------------------------------------------------
# Rules how to make each one of the binaries

EXELIST=nop

nop.d64:
	$(C1541) -format nop,AA  d64 nop.d64 > /dev/null

nop: 		$(OBJS) $(CLIB) nop.d64
	@$(LD) -o $@ $(OBJS) $(CLIB)
	# @$(PUCRUNCH) -ffast nop nop
	@for exe in $(EXELIST); do\
	    $(C1541) -attach nop.d64 -delete $$exe  > /dev/null;\
	    $(C1541) -attach nop.d64 -write $$exe  > /dev/null;\
	done;

run: nop
	x64sc nop.d64

# --------------------------------------------------------------------------
# Cleanup rules

.PHONY:	clean
clean:
	rm -f $(EXELIST) *.d64 *.map *.o *.lbl *.prg *.lst *.a *.u $(DEPDIR)/*

# ------------------

