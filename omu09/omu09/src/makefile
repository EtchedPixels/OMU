CC=cc09
AS=as09
CONTS = mc.o bcache.o bio.o c.o cio.o fio.o icache.o inode.o messg.o physmap.o super.o sys.o uft.o
C_CONTS = bcache.c bio.c c.c cio.c fio.c icache.c inode.c messg.c physmap.c super.c sys.c uft.c

omu: $(CONTS)
	cd dev; make devlib
	ld09 -s -t 1D -o omu $(CONTS) dev/devlib -lc -lbsa -lbx -la
	hex09 omu -o /tmp/omu.s
	rm omu
	mv /tmp/omu.s .

lint:
	lint -nahbcu $(C_CONTS) dev/null/*.c dev/fdc/fdc.c dev/m6551/*.c > lint.out

mc.o: mc.09
	$(AS) -o mc.o mc.09

list:
	opr -l lint.out include/*.h makefile *.[c0]* dev/makefile \
		dev/fdc/makefile dev/fdc/*.c \
		dev/m6551/makefile dev/m6551/*.h dev/m6551/*.c \
		dev/null/makefile dev/null/*.c

bcache.o: bcache.c include/buf.h include/dev.h include/param.h
bio.o: bio.c include/buf.h include/inode.h include/file.h include/dev.h
c.o: c.c include/dev.h
cio.o: cio.c include/file.h include/dev.h
fio.o: fio.c include/buf.h include/inode.h include/file.h include/dev.h
icache.o: icache.c include/file.h include/ino.h include/buf.h include/dev.h include/inode.h include/param.h
inode.o: inode.c include/file.h include/inode.h include/dev.h include/types.h
physmap.o: physmap.c include/inode.h include/buf.h
super.o: super.c include/filsys.h include/fblk.h include/buf.h include/types.h include/inode.h include/dev.h include/ino.h
sys.o: sys.c include/a.out.h include/inode.h include/param.h include/procs.h
uft.o: uft.c include/file.h include/uft.h include/inode.h include/param.h include/procs.h
