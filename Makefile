CRT0   	= $(CC65_HOME)/lib/c64.o
CLIB	= $(CC65_HOME)/lib/c64.lib
CC	= cc65 -Or -O -Cl -tc64 -T -I $(CC65_HOME)/include/
# CC	= cc65 --create-dep -Cl -tc64 -T -I $(CC65BASE)/include/
AS	= ca65 --cpu 6502x # -l
LD	= ld65 -C src/nop.cfg -m nop.map -Ln nop.lbl
PUCRUNCH = ~/bin/pucrunch
C1541  	= c1541
DEPDIR = build


all:   	nop

# --------------------------------------------------------------------------
# Generic rules

build/%.a : src/%.c
	@echo $<
	@mkdir -p build
	@$(CC) --create-dep $(DEPDIR)/$(notdir $(basename $<)).u \
		-o build/$(notdir $(basename $<)).a \
		$(basename $<).c

build/%.o : build/%.a
	@$(AS) -o build/$(notdir $(basename $<)).o $(basename $<).a

# Don't delete intermediate .a files.
.PRECIOUS : build/%.a

build/%.o : src/%.s
	@echo $<
	@$(AS) -o build/$(notdir $(basename $<)).o $<

SRCS := $(wildcard src/*.c src/*.s)
OBJS := $(addprefix build/,$(addsuffix .o,$(notdir $(basename ${SRCS}))))

-include $(OBJS:build/%.o=build/%.u)

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
	done;\
	$(C1541) -attach nop.d64 -delete lightforce  > /dev/null;\
	$(C1541) -attach nop.d64 -write res/lightforce  > /dev/null;

run: nop
	x64sc nop.d64

# --------------------------------------------------------------------------
# Cleanup rules

.PHONY:	clean
clean:
	rm -rf $(EXELIST) *.d64 *.map build *.lbl *.prg *.lst *.u $(DEPDIR)

# ------------------

