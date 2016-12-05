/*
 * ADB - simplified interactive debug program.
 *	Uses standard dissassembler, works on many machines.
 *
 * History:
 * Version 1:	S.Hosgood 1982
 * Version 2:	Re-written for new std diss. format. 21.nov.84
 */

# include	"ass.out.h"
# define digit(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f'))

char	get_byte();
ADDR	cur_pc(), first_pc, last_pc;
ADDR	number(), a_open(), chk_limits();

main(argc,argv)
int argc;
char *argv[];
{
	char	buff[30], *ch, prev_inst;
	int	diss(), show_byte(), show_ch();
	ADDR	from_pc, to_pc, prev_pc;

	if (argc != 2) {
		printf("Usage is '%s <filename>'\n", argv[0]);
		exit(-1);
	}

	if (a_open(argv[1]) < 0) {
		printf("File doesn't exist.\n");
		exit(-1);
	}

	/* Now interpret user's instructions */
	prev_inst = '$';
	prev_pc = first_pc;

	while (1) {
		putchar('>');
		read(0, buff, 30);
		ch = buff;

		if (digit(*ch)) {
			/* read first addr */
			from_pc = to_pc = number(&ch);

			if (*ch == ',') {
				ch++;
				to_pc = number(&ch);
			}

			if (*ch == '\n')
				/* re-use last instr */
				*ch = prev_inst;

			/* note this as being a command address */
			prev_pc = cur_pc();
		}
		else if (*ch == '\n') {
			/* step on to next one.. */
			prev_pc = from_pc = to_pc = cur_pc();
			*ch = prev_inst;
		}
		else
			/* perform command on last pc seen */
			from_pc = to_pc = prev_pc;

		/* check limits of both pcs */
		from_pc = chk_limits(from_pc);
		to_pc = chk_limits(to_pc);

		/* interpret command */
		switch (prev_inst = *ch) {
		case 'q':
			exit(0);

		case '?':
			repeat(from_pc, to_pc, diss);
			break;

		case '"':
			repeat(from_pc, to_pc, show_ch);
			break;

		case '$':
			repeat(from_pc, to_pc, show_byte);
			break;

		default:
			prev_inst = '\0';
			printf("??\n");
		}

	}

	exit(0);
}

repeat(from_pc, to_pc, func)
ADDR from_pc, to_pc;
int (*func)();
{

	set_pc(from_pc);

	while (cur_pc() <= to_pc) {
		praddr(cur_pc());
		putchar(':');
		putchar('\t');
		(*func)(0);
	}

	return;
}

hex4(str)
char *str;
{
	int cnt, val;

	val = 0;
	for (cnt = 0; cnt != 4; cnt++){
		val <<= 4;
		val += hexval(str[cnt]);
	}

	return val;
}

hex2(str)
char *str;
{

	return (hexval(str[0])<<4) | hexval(str[1]);
}

hexval(ch)
char ch;
{

	if (ch >= 'a')
		ch &= 0x5f;

	ch -= '0';
	if (ch > 9)
		ch -= 7;

	return ch;
}

prhex4(val)
unsigned val;
{

	prhex(val>>12);
	prhex(val>> 8);
	prhex(val>> 4);
	prhex(val);
	return;
}

prhex2(val)
unsigned val;
{

	prhex(val>> 4);
	prhex(val);
	return;
}

prhex(val)
unsigned val;
{

	val &= 0xf;
	if (val < 0xA)
		putchar(( int )(val + '0'));
	else
		putchar(( int )(val-0xa+'A'));
	return;
}

/*
 * This routine prints a character, converts newline to '\n' etc, and
 * converts misc control codes to hex constants.
 */

show_ch()
{
	char ch;

	if ((ch = get_byte()) < 0x20){
		switch (ch){
		case '\n':
			putchar('\\');
			putchar('n');
			break;

		case '\r':
			putchar('\\');
			putchar('r');
			break;

		case '\t':
			putchar('\\');
			putchar('t');
			break;

		case 0:
			putchar('\\');
			putchar('0');
			break;

		case '\b':
			putchar('\\');
			putchar('b');
			break;

		default:
			putchar('<');
			putchar('$');
			prhex2(( unsigned ) ch);
			putchar('>');
		}
	}
	else
		putchar(ch);

	putchar('\n');
	return;
}

/*
 * This displays a byte in hex notation. Increments 'cur_pc'.
 */

show_byte()
{

	putchar('$');
	prhex2(( unsigned ) get_byte());
	putchar('\n');
	return;
}

/*
 * Diss: calls both halves of the std dissassembler.
 */
diss()
{

	decode(0);
	pr_bytes();
	pr_inst();
	return;
}

/*
 * Std disassembler requires 'mk_lab'. This is a dummy.
 */
mk_lab(addr, type)
char type;
{

	return 0;
}

poss_lab(val)
ADDR val;
{

	putchar('$');
	praddr(val);
	return;
}

/*
 * Number - converts a number, altering pointer as it does.
 *	Assumes numbers beginning 0 are octal, others hex.
 */
ADDR
number(ch)
char **ch;
{
	ADDR pc;

	pc = 0;
	if (**ch == '0') {
		/* assume octal */
		for ((*ch)++; **ch >= '0' && **ch <= '7'; (*ch)++) {
			pc <<= 3;
			pc |= **ch - '0';
		}
	}
	else {
		for (; digit(*(*ch)); (*ch)++){
			pc <<= 4;
			pc |= hexval(*(*ch));
		}
	}

	return pc;
}

ADDR
chk_limits(pc)
ADDR pc;
{

	if (pc < first_pc) {
		printf("Too low\n");
		pc = first_pc;
	}
	else if (pc > last_pc) {
		printf("Too high\n");
		pc = last_pc;
	}

	return pc;
}
