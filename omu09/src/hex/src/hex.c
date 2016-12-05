# include	<stdio.h>
	/* read magic header to check file format */
# include	<signal.h>
# include	<sys/types.h>
# include	<sys/stat.h>
# include	<sgtty.h>
# include	"ass.out.h"

int		length, mode;
int		tidy_exit(), fdes;
struct stat	ttymode;
struct sgttyb	ttystate;
char		*progname, *outname;
struct exec	magic;

/*
 * Struct of 4 numbers we send in an S0 record for documentation.
 */
struct {
	short	tstart;
	short	text_size;
	short	data_size;
	short	bss_size;
}	s0_info;

ADDR		codeaddr;
unsigned	checksum;

main(argc, argv)
char *argv[];
{
	char **ap, *infile;
	int nbytes, o_flag;
	ADDR	 codesize;
	char buffer[100];
	FILE *in;

	/* make S-records from ass.out format files */
	o_flag = 0;
	progname = argv[0];
	infile = ( char * ) 0;
	ap = &argv[1];

	while (--argc > 0) {
		if ((*ap)[0] == '-') switch ((*ap)[1]) {
		case 'o':
			/* specify output file or pipe */
			o_flag++;

			if (--argc) {
				/* if name occurs, open it */
				if (freopen(*++ap, "w", stdout) == NULL)
					error("Can't create %s", *ap);
			}

			break;

		default:
			error("Illegal flag %s", *ap);
		}
		else {
			if (!infile) {
				if ((in = fopen(infile = *ap, "r")) == NULL)
					error("Can't read %s", *ap);
			}
			else
				error("Too many input files");
		}

		/* next arg? */
		ap++;
	}

	/* check we've got an input file */
	if (!infile)
		error("No input file");

	fread(&magic, sizeof magic, 1, in);

	if (magic.a_magic != SQUASH && magic.a_magic != ROMABLE)
		error("%s is not downloadable format", infile);

# ifndef omu
	/* insist on tty output unless -o flag over-rode */
	if (o_flag == 0) {
		if (isatty(fdes = fileno(stdout))) {
			/* it's a tty, get current state */
			ioctl(fdes, TIOCGETA, &ttystate);
			length = ttystate.sg_length;
			ttystate.sg_length = 0;
			fstat(fdes, &ttymode);
			mode = ttymode.st_mode;

			/* now catch signals */
			signal(SIGQUIT, tidy_exit);
			signal(SIGINT, tidy_exit);

			/* and set no page length */
			ioctl(fdes, TIOCSETA, &ttystate);

			/* set mode of tty to prevent interruptions */
			chmod(outname = ttyname(fdes), mode & 0700);
		}
		else
			error("Output must be tty");
	}
# endif

	/* buffer output file and make S0 record */
	setbuf(stdout, malloc(BUFSIZ));

	/* 4 numbers are passed in s0 record */
	s0_info.tstart = byteswop(magic.a_toffset);
	s0_info.text_size = byteswop(magic.a_text);
	s0_info.data_size = byteswop(magic.a_data);
	s0_info.bss_size = byteswop(magic.a_bss);

	mksrec(0, &s0_info, sizeof s0_info);

	codesize = magic.a_text + magic.a_data;
	codeaddr = magic.a_toffset;

	while (codesize > 0) {
		nbytes = fread(buffer, sizeof (char), codesize > 16? 16: codesize, in);
		mksrec(1, buffer, nbytes);
		codesize -= nbytes;
	}

	/* finish with blank S9 record */
	fprintf(stdout, "S9030000FC\n\r");

	/* careful exit if we've set user's tty */
	if (!o_flag)
		tidy_exit();

	exit(0);
}

mksrec(type, buffer, size)
char *buffer;
{

	/* outputs up to 16 bytes from 'buffer' in s-record format */
	fprintf(stdout, "S%d", type);
	checksum = 0;
	putbyte(size+3);
	putword(codeaddr);
	codeaddr += size;
	while (size--){
		putbyte(*buffer++);
	}
	putbyte(~checksum);
	fprintf(stdout, "\n\r");
	return;
}

putword(word)
unsigned word;
{

	putbyte(word >> 8);
	putbyte(word);
	return;
}

putbyte(byte)
unsigned byte;
{
	byte &= 0xFF;
	putnybble(byte >> 4);
	putnybble(byte);
	checksum += byte;
	return;
}

putnybble(nybble)
unsigned nybble;
{
	nybble &= 0x0F;
	if (nybble > 9) putc(nybble-0x0A+'A', stdout);
	else
			putc(nybble+'0', stdout);

	/* check for error */
	if (ferror(stdout)) {
		error("Write error");
		exit(-1);
	}

	return;
}

/*VARARGS1*/
error(s1, s2)
char *s1, *s2;
{

	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, s1, s2);
	putc('\n', stderr);
	exit(-1);
}

/*
 * Exit this way if the user's terminal was got-at.
 */
tidy_exit()
{

# ifndef omu
	ttystate.sg_length = length;
	ioctl(fdes, TIOCSETA, &ttystate);

	chmod(outname, mode);
# endif
	exit(0);
}

/*
 * Byteswop - we wouldn't need this in a SENSIBLE bloody computer would we?
 *	The PDP-11 insists on storing ints as lobyte-hibyte, so we have to do
 *	a switcheroo on them.
 */
byteswop(val)
unsigned val;
{
# ifdef pdp
	unsigned ret;

	ret = val >> 8;
	val <<= 8;

	return val | ret;
# endif
# ifdef omu
	return val;
# endif
}
