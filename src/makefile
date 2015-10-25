# generic makefile for standard procedural databases
# Author:  Eric Haines

CC=cc -O
SUFOBJ=.o
SUFEXE=
INC=def.h lib.h
LIBOBJ=drv_null$(SUFOBJ) libini$(SUFOBJ) libinf$(SUFOBJ) libpr1$(SUFOBJ) \
	libpr2$(SUFOBJ) libpr3$(SUFOBJ) libply$(SUFOBJ) libdmp$(SUFOBJ) \
	libvec$(SUFOBJ) libtx$(SUFOBJ)
BASELIB=-lm

all:		balls gears mount rings teapot tetra tree \
		readdxf readnff readobj \
		sample lattice shells jacks sombrero nurbtst

drv_null$(SUFOBJ):	$(INC) drv_null.c drv.h
		$(CC) -c drv_null.c

libini$(SUFOBJ):	$(INC) libini.c
		$(CC) -c libini.c

libinf$(SUFOBJ):	$(INC) libinf.c
		$(CC) -c libinf.c

libpr1$(SUFOBJ):	$(INC) libpr1.c
		$(CC) -c libpr1.c

libpr2$(SUFOBJ):	$(INC) libpr2.c
		$(CC) -c libpr2.c

libpr3$(SUFOBJ):	$(INC) libpr3.c
		$(CC) -c libpr3.c

libply$(SUFOBJ):	$(INC) libply.c
		$(CC) -c libply.c

libdmp$(SUFOBJ):	$(INC) libdmp.c
		$(CC) -c libdmp.c

libvec$(SUFOBJ):	$(INC) libvec.c
		$(CC) -c libvec.c

libtx$(SUFOBJ):		$(INC) libtx.c
		$(CC) -c libtx.c

balls$(SUFEXE):		$(LIBOBJ) balls.c
		$(CC) -o balls$(SUFEXE) balls.c $(LIBOBJ) $(BASELIB)

gears$(SUFEXE):		$(LIBOBJ) gears.c
		$(CC) -o gears$(SUFEXE) gears.c $(LIBOBJ) $(BASELIB)

mount$(SUFEXE):		$(LIBOBJ) mount.c
		$(CC) -o mount$(SUFEXE) mount.c $(LIBOBJ) $(BASELIB)

rings$(SUFEXE):		$(LIBOBJ) rings.c
		$(CC) -o rings$(SUFEXE) rings.c $(LIBOBJ) $(BASELIB)

teapot$(SUFEXE):		$(LIBOBJ) teapot.c
		$(CC) -o teapot$(SUFEXE) teapot.c $(LIBOBJ) $(BASELIB)

tetra$(SUFEXE):		$(LIBOBJ) tetra.c
		$(CC) -o tetra$(SUFEXE) tetra.c $(LIBOBJ) $(BASELIB)

tree$(SUFEXE):		$(LIBOBJ) tree.c
		$(CC) -o tree$(SUFEXE) tree.c $(LIBOBJ) $(BASELIB)

readdxf$(SUFEXE):		$(LIBOBJ) readdxf.c
		$(CC) -o readdxf$(SUFEXE) readdxf.c $(LIBOBJ) $(BASELIB)

readnff$(SUFEXE):		$(LIBOBJ) readnff.c
		$(CC) -o readnff$(SUFEXE) readnff.c $(LIBOBJ) $(BASELIB)

readobj$(SUFEXE):		$(LIBOBJ) readobj.c
		$(CC) -o readobj$(SUFEXE) readobj.c $(LIBOBJ) $(BASELIB)

sample$(SUFEXE):		$(LIBOBJ) sample.c
		$(CC) -o sample$(SUFEXE) sample.c $(LIBOBJ) $(BASELIB)

lattice$(SUFEXE):		$(LIBOBJ) lattice.c
		$(CC) -o lattice$(SUFEXE) lattice.c $(LIBOBJ) $(BASELIB)

shells$(SUFEXE):		$(LIBOBJ) shells.c
		$(CC) -o shells$(SUFEXE) shells.c $(LIBOBJ) $(BASELIB)

jacks$(SUFEXE):			$(LIBOBJ) jacks.c
		$(CC) -o jacks$(SUFEXE) jacks.c $(LIBOBJ) $(BASELIB)

sombrero$(SUFEXE):		$(LIBOBJ) sombrero.c
		$(CC) -o sombrero$(SUFEXE) sombrero.c $(LIBOBJ) $(BASELIB)

nurbtst$(SUFEXE):		$(LIBOBJ) nurbtst.c
		$(CC) -o nurbtst$(SUFEXE) nurbtst.c $(LIBOBJ) $(BASELIB)

clean:
	rm -f balls gears mount rings teapot tetra tree \
		readdxf readnff readobj \
		sample lattice shells jacks sombrero nurbtst
	rm -f $(LIBOBJ)
