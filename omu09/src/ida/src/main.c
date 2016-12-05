/*
 * Main driver for Clever Disassembler, S.Hosgood 21.11.83
 *
 * History
 * Version 1    Pinched from old 6809 disassembler, 21.Nov.83
 * Version 1.1  Altered to allow driving of different dissassemblers for
 *              other target kits, 14.feb.84
 */

# define digit(c) ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))

# include	<stdio.h>
# include	"../ass.out.h"
# include       "list.h"

ADDR dollr, lastaddr;
ADDR hex4();

int labels[6], notref;
int shownotref = 0;

main(argc, argv)
int argc;
char *argv[];
{
	char *filename, **ap;
	LIST *lptr;

	filename = ( char * ) 0;

	if (argc < 2) {
		fprintf(stderr, "Usage is %s {-n} <a.out file>\n", argv[0]);
		exit(-1);
	}

	ap = &argv[1];
	argc--;
	while (argc--) {
		if ((*ap)[0] == '-')
			switch ((*ap)[1]) {
			case 'n':
				shownotref++;
				break;

			default:
				fprintf(stderr, "%s: Unknown flag: %s\n", argv[0], *ap);
				exit(-1);
			}
		else
			if (!filename)
				filename = *ap;
			else {
				fprintf(stderr, "%s: Too many files: %s\n", argv[0], *ap);
				exit(-1);
			}

		ap++;
	}

	/* set up lists etc for clever disassembly */
	initlist();

	if ((dollr = a_open(filename)) == 0){
		fprintf(stderr, "File doesn't exist.\n");
		exit(-1);
	}

	/* maybe get clever info to aid disassembly */
	clever();

	/* now read text, finding flow blocks */
	scan();

	/* use counters to give labels to flow blocks */
	labels[ENT] = labels[PRG] = labels[SBR] = labels[DAT] = 1;
	labels[BTBL_1] = 1;

	lptr = lstart;
	while (lptr){
		lptr->lab = labels[(lptr->type == BTBL_2)? BTBL_1: lptr->type]++;
		lptr = lptr->next;
	}

	/* then print whole flow list in correct modes */
	lptr = lstart;
	lastaddr = 0;
	notref = 1;
	while (lptr) {
		if (lastaddr < lptr->first) {
			if (shownotref) {
				putchar('\t');
				pr_bytes();
				printf("notref%d:\n", notref++);
				set_pc(lastaddr);
				show_bytes(lptr->first - 1);
				putchar('\n');
			}
			else
				set_pc(lptr->first);
		}
		else if (lastaddr > lptr->first) {
			fprintf(stderr, "Warning - overlap at %x\n", lptr->first);
			lastaddr = lptr->first;
		}

		/* note global names */
		if (lptr->type == GNAME) {
			/* pad past addr space and bytes of instr */
			putchar('\t');
			pr_bytes();
			printf("\tglobl\t%s\n", lptr->name);
		}

		/* print label on line */
		printf("\t");
		pr_bytes();
		pr_lab(lptr);
		printf(":\n");

		switch (lptr->type) {
		case ENT:
		case PRG:
		case SBR:
		case GNAME:
		case LNAME:
			diss(lptr->first, lptr->last);
			break;

		case DAT:
			/* set up last addr of a data area */
			if (lptr->next)
				lptr->last = lptr->next->first - 1;
			else
				lptr->last = dollr;

			set_pc(lptr->first);
			show_bytes(lptr->last);
			break;

		case BTBL_1:
			show_btbl(lptr->first, lptr->last, 0);
			break;

		case BTBL_2:
			show_btbl(lptr->first, lptr->last, lptr->first);
		}

		putchar('\n');
		lastaddr = lptr->last + 1;
		lptr = lptr->next;
	}

	exit(0);
}
