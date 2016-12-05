/*
 * First thru' (N-1)th pass of Clever Dissassembler, S.Hosgood, 5.dec.83
 *
 * History:
 * Version 1    Written 5th Dec 83.
 * Version 1.1  This version to use 68000 dissassembler, 14.feb.84
 */

# include	<stdio.h>
# include	"../ass.out.h"
# include       "list.h"
# include       "main.h"

static LIST *lptr;
LIST *chk_refs();

/*
 * Scan - work thru flow graph, dissassembling all unexplored, reachable bits.
 */
scan()
{
	int start_seg, flowbreak;

	lptr = lstart;
	start_seg = 1;

	while (lptr){
		/* check we're not off end of text */
		if ( !start_seg){
			if (cur_pc() >= dollr){
				fprintf(stderr, "Warning code runs off end\n");
				start_seg = 1;
				lptr->last = cur_pc()-1;
			}
			else if (lptr->next && lptr->next->first <= cur_pc()){
				/* if we run into next block, finish present one */
				lptr->last = cur_pc()-1;
				start_seg = 1;
			}
		}

		if (start_seg){
			/* look for next unexplored, reachable text */
			lptr = lstart;
			while (lptr){
				if (lptr->type <= PRG && lptr->last == 0)
					break;
				lptr = lptr->next;
			}
		}

		if (lptr){
			if (start_seg) {
				start_seg = 0;
				set_pc(lptr->first);
			}

			/* dissassemble an instruction, loop to check flow graph */
			flowbreak = decode(0);

			/* check for breaks in flow */
			if (flowbreak) {
				lptr->last = cur_pc()-1;
				start_seg = 1;
			}
		}
	}

	return;
}

/*
 * Scan_btbl - scans branch table, creating entries for locations referenced
 */
scan_btbl(type, reftype, start, end)
ADDR start, end;
{
	ADDR base_addr;
	LIST *lptr;

	switch (type){
	case BTBL_1:
		base_addr = 0;
		break;

	case BTBL_2:
		base_addr = start;
	}

	/* create node to reference table itself */
	lptr = enternew(type, start, NULLIST);
	lptr->last = end;

	/* pick up entries in table, create nodes for them too */
	for (set_pc(start); cur_pc() <= end; )
		enternew(reftype, get_word()+base_addr, NULLIST);

	return;
}

/*
 * Mk_lab - makes a label, type given by single letter, at 'addr'.
 *	Letter codes: 'p' == PRG, 'd' == DAT, 's' == SBR, 'e' == ENT.
 *	Routine may split the 'current block', lptr.
 *	Routine may be called more than once in a call of 'decode', if the
 *	instruction has multiple operands.
 */
mk_lab(addr, type)
ADDR addr;
char type;
{

	switch (type) {
	case 'p':
		type = PRG;
		break;

	case 'd':
		type = DAT;
		break;

	case 's':
		type = SBR;
		break;

	case 'e':
		type = ENT;
		break;

	default:
		fprintf(stderr, "Fatal, code %c in mk_lab\n", type);
		exit(-1);
	}

	lptr = enternew(type, addr, lptr);
	return;
}
