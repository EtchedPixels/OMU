/*
 * Misc routines for Clever Disassembler, S.Hosgood 21.11.83
 *
 * History:
 * Version 1    Written for original 6809 disassembler, 1982.
 * Version 2    Split off into seperate file, 21.11.83
 * Version 2.1  Altered to allow different target hardware, 14.feb.84
 */

# include	"../ass.out.h"
# include       "main.h"

/*
 * Diss - dissassemble and print instructions.
 */
diss(start, end)
ADDR start, end;
{
	int flowbreak;

	for (set_pc(start); cur_pc() <= end; ) {
		/* print address of instruction + bytes of instruction */
		praddr(cur_pc());
		printf(": ");

		/* decode */
		flowbreak = decode(0);

		pr_bytes();
		pr_inst();
	}

	return flowbreak;
}

hexval(ch)
char ch;
{
	if (ch >= 'a') ch &= 0x5f;
	ch -= '0';
	if (ch > 9) ch -= 7;
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
	if (val < 0xa) putchar(( char ) val+'0');
	else putchar(( char ) val-0xa+'A');
	return;
}

show_bytes(stopdot)
ADDR stopdot;
{
	int count;

	while (cur_pc() <= stopdot){
		praddr(cur_pc());
		printf(": ");
		pr_bytes();
		printf("\tfcb\t");
		count = 8;
		while (count-- && cur_pc() <= stopdot){
			putchar('$');
			prhex2(get_byte());
			if (count && cur_pc() <= stopdot)
				putchar(',');
		}

		putchar('\n');
	}
	return;
}

/*
 * Show_btbl - interpret data as being addresses of things, relative to
 *              base address given.
 */
show_btbl(start, end, base_addr)
ADDR start, end, base_addr;
{

	for (set_pc(start); cur_pc() <= end; ) {
		praddr(cur_pc());
		printf(": ");

		/* get word, allow 'pr_bytes' to print */
		getw();
		pr_bytes();

		/* print as <addr> - <base> */
		printf("\tfdb\t");
		poss_lab(get_word() + base_addr);
		putchar('-');
		poss_lab(base_addr);
		putchar('\n');
	}

	return;
}
