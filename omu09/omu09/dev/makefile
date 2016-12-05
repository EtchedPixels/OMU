create:
	cd m6551; make m6551.o
	cd null; make null.o
	cd fdc; make fdc.o
	rm -f devlib
	ar rc devlib fdc/fdc.o null/null.o m6551/m6551.o

devlib: fdc/fdc.o null/null.o m6551/m6551.o

fdc/fdc.o:
	cd fdc; make fdc.o
	ar rv devlib fdc/fdc.o

null/null.o:
	cd null; make null.o
	ar rv devlib null/null.o

m6551/m6551.o:
	cd m6551; make m6551.o
	ar rv devlib m6551/m6551.o
