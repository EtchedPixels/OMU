/*
 * Read 'clever file' for 'Clever Dissassembler', s.Hosgood, 13.mar.84
 *
 * Version 1    Written 13.mar.84
 */

# include	"../ass.out.h"
# include       "list.h"
# include       <stdio.h>

# define        STRSTORE        1000

char strstore[STRSTORE];
char *strptr = strstore;

/*
 * Beclever - called after initialisation of symbol list: kicks off various
 *              routines to interpret 'clever file'.
 */
beclever(fileid)
{
	char buffer[80], *argv[10], *cptr;
	int argc;

	while (readln(fileid, buffer, 80)){
		cptr = argv[0] = buffer;
		argc = 1;

		while (*cptr){
			while (*cptr && *cptr != ':')
				cptr++;

			if (*cptr)
				*cptr++ = '\0';

			while (*cptr && *cptr == ' ')
				cptr++;

			if (*cptr)
				argv[argc++] = cptr;
		}

		/* now examine first arg to see what to kick off */
		if (strcmp(argv[0], "GNAME") == 0){
			/* global name def */
			namdef(GNAME, argc, argv);
		}
		else if (strcmp(argv[0], "LNAME") == 0){
			/* local name def */
			namdef(LNAME, argc, argv);
		}
	}

	return;
}

/*
 * Readln - gets a line from given file, removes trailing newline.
 */
readln(fileid, buffer, nbytes)
char *buffer;
{
	char ch;
	int nread;

	nread = 0;
	while (nbytes-- && read(fileid, &ch, 1)) {
		nread++;

		if (ch != '\n')
			*buffer++ = ch;
		else {
			*buffer = '\0';
			break;
		}
	}

	return nread;
}

/*
 * Namdef - enter GNAME or LNAME type symbol in table.
 */
namdef(type, argc, argv)
char *argv[];
{
	char *cptr;
	ADDR address;

	if (strcmp(argv[1], "N_TEXT") == 0 || strcmp(argv[1], "N_DATA") == 0){
		sscanf(argv[2], "%x", &address);
		entername(type, address, strptr);

		/* copy name of symbol to string store */
		cptr = argv[3];
		while (*strptr++ = *cptr++);

		if (strptr > &strstore[STRSTORE]){
			fprintf(stderr, "Fatal - no string space\n");
			exit(-1);
		}
	}

	return;
}
