CFLAGS = -O2 -Wall -W -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 
STRIP = strip -s -R .comment 
SSTRIP = strip -s -R .comment -R .eh_frame $@
STATIC = -static-libgcc

# CFLAGS = -g -Wall -W -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 
# STRIP = :
# SSTRIP = :

VER=0.2

PROGRAMS = raidextract parityinfo xor
PROGRAMS += raidextract.static parityinfo.static xor.static

RAIDEXTRACT_OBJS = raidextract.o util.o

PARITYINFO_OBJS = parityinfo.o util.o

XOR_OBJS = xor.o util.o

all: $(PROGRAMS)

clean:
	$(RM) ./*~ ./*.o $(PROGRAMS)

distclean: 
	$(RM) $(PROGRAMS) ./*~ ./*.o raidextract-*

raidextract: $(RAIDEXTRACT_OBJS)
	$(LINK.c) -o $@ $(RAIDEXTRACT_OBJS) && $(STRIP) $@

raidextract.static: $(RAIDEXTRACT_OBJS)
	$(LINK.c) -o $@ $(RAIDEXTRACT_OBJS) $(STATIC) && $(SSTRIP) $@

parityinfo: $(PARITYINFO_OBJS)
	$(LINK.c) -o $@ $(PARITYINFO_OBJS) && $(STRIP) $@

parityinfo.static: $(PARITYINFO_OBJS)
	$(LINK.c) -o $@ $(PARITYINFO_OBJS) $(STATIC) && $(SSTRIP) $@

xor: $(XOR_OBJS)
	$(LINK.c) -o $@ $(XOR_OBJS) && $(STRIP) $@

xor.static: $(XOR_OBJS)
	$(LINK.c) -o $@ $(XOR_OBJS) $(STATIC) && $(SSTRIP) $@

*.o: util.h
# *.static.o: lib.h

dist: distclean
	ln -s . raidextract-$(VER)
	tar --exclude raidextract-$(VER)/raidextract-$(VER) --exclude raidextract-$(VER)/raidextract-$(VER).tar -cvf raidextract-$(VER).tar raidextract-$(VER)/*
	gzip -9 raidextract-$(VER).tar
	rm raidextract-$(VER)

.PHONY: all clean distclean dist
