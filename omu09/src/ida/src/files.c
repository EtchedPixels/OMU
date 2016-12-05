/*
 * File handler for 6809 Standard Disassembler.
 *
 * History:
 * Version 1:	Written by S.Hosgood, 22.nov.84
 * Version 1.1:	Modified from PDP-11 to 6809, 23.nov.84
 */

# include	"ass.out.h"

/*
 * Pc = current 'program counter'.
 * First_pc = lowest addr. user may specify.
 * Last_pc = highest addr. user may specify.
 */
extern ADDR		first_pc, last_pc;

static char		file_nm[30];
static int		fileid;
static ADDR		pc;
static struct exec	hddr;

/*
 * A_open - open file for reading, step past magic header, sit at first byte.
 */
a_open(name)
char *name;
{

	if ((fileid = open(name, 0)) == -1)
		return -1;

	strncpy(file_nm, name, 30);

	if (read(fileid, &hddr, sizeof hddr) != sizeof hddr)
		return -1;

	pc = first_pc = hddr.a_toffset;
	last_pc = first_pc + hddr.a_text + hddr.a_data;

	printf("Text starts at ");
	praddr(first_pc);
	printf(", data ends at ");
	praddr(last_pc);
	putchar('\n');
	return 0;
}

/*
 * Set_pc - moves such that next get-byte returns byte at address given.
 */
set_pc(val)
ADDR val;
{

	if (val < first_pc) {
		praddr(val);
		printf(": address too low\n");
		val = first_pc;
	}
	else if (val >= last_pc) {
		praddr(val);
		printf(": address too high\n");
		val = last_pc;
	}

	pc = val;
	lseek(fileid, ( long )(pc - first_pc + sizeof hddr), 0);
	return;
}

/*
 * Get_byte - get 1 byte starting at 'pc', and make a byte.
 */
char
get_byte()
{
	char temp;

	if (pc >= last_pc) {
		printf("Off end\n");
		return 0;
	}

	if (read(fileid, &temp, 1) != 1) {
		printf("Error reading %s\n", file_nm);
		exit(-1);
	}

	pc += 1;
	return temp;
}

/*
 * Get_word - get 2 bytes starting at 'pc', and make a word.
 */
get_word()
{
	int temp;

	temp = get_byte();
	temp = (temp << 8) | (get_byte() & 0xFF);
	return temp;
}

/*
 * Cur-pc - returns current value of 'pc' as an int.
 */
ADDR
cur_pc()
{

	return pc;
}
