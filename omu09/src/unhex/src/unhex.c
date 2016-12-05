/*
 * S-record binary file reader.
 * Mainly used to create 6809 OMU-runnable binaries from S-Record files
 *	sent from the PDP-11. Will make use of S0 record with text, data
 *	and bss sizes encoded in it.
 */

# include	<stdio.h>
# include	"ass.out.h"

# define	digit(c)	((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))

char		checksum;
int		gflag;

main(argc, argv)
int argc;
char *argv[];
{
	char		inchar;
	int		frame_cnt;
	ADDR		this_addr, last_addr;
	FILE		*file;
	struct exec	hddr;

	if (argc != 2) {
		fprintf(stderr, "Usage is '%s <filename>'.\n", argv[0]);
		exit(-1);
	}

	if ((file = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "File doesn't exist.\n");
		exit(-1);
	}

	inchar = getc(file);

	while (feof(file) == 0) {
		if (inchar != 'S') {
			fprintf(stderr, "File is not S-record format.\n");
			exit(-1);
		}

		/* set up ready for processing */
		inchar = getc(file);
		checksum = 0;
		frame_cnt = hex2(file);

		switch (inchar) {
		case '0':
			/* see if sizes are encoded */
			if (frame_cnt == 0x0B) {
				/* assume they are */
				/* ignore record's address */
				hex4(file);
				last_addr = hddr.a_toffset = hex4(file);
				hddr.a_text = hex4(file);
				hddr.a_doffset = hddr.a_toffset + hddr.a_text;
				hddr.a_data = hex4(file);
				hddr.a_boffset = hddr.a_doffset + hddr.a_data;
				hddr.a_bss = hex4(file);

				hddr.a_magic = SQUASH;
				hddr.a_syms = 0;

				fwrite( ( char * ) &hddr, sizeof hddr, 1, stdout);
			}
			else
				last_addr = 0;

			break;

		case '1':
			this_addr = hex4(file);

			if (gflag && (last_addr < this_addr)) {
				/* fill holes in file */
				while (last_addr < this_addr) {
					putchar('\0');
					last_addr++;
				}
			}

			if (last_addr != this_addr) {
				fprintf(stderr, "Data address error: %x\n", this_addr);
				exit(-1);
			}

			/* remove address and checksum from frame count */
			for (frame_cnt -= 3; frame_cnt > 0; frame_cnt--) {
				putchar(hex2(file));
				last_addr++;
			}

			break;

		case '9':
			/* scan along to the checksum */
			while (frame_cnt > 1) {
				hex2(file);
				frame_cnt--;
			}

			break;

		default:
			fprintf(stderr, "File format error.\n");
			exit(-1);
		}

		/* read in checksum */
		hex2(file);
		checksum = ~checksum;

		if (checksum != 0) {
			fprintf(stderr, "Checksum error: %d\n", checksum);
			exit(-1);
		}

		/* trailing linefeeds may occur */
		while ((inchar = getc(file)) == '\n' || inchar == '\r');
	}

	exit(0);
}

hex4(file)
FILE *file;
{
	unsigned	val;

	val = 0;
	val = hex2(file) << 8;
	val += hex2(file);
	return val;
}

hex2(file)
FILE *file;
{
	unsigned	val;

	val = hexval(file) << 4;
	val += hexval(file);
	checksum += val;
	return val;
}

hexval(file)
FILE *file;
{
	int		ch;

	if ((ch = getc(file)) == EOF) {
		fprintf(stderr, "Early EOF\n");
		exit(-1);
	}

	if (ch >= 'a') ch &= 0x5f;
	ch -= '0';
	if (ch > 9) ch -= 7;
	return ch;
}
