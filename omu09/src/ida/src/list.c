/*
 * List handler for Clever Disassembler, S.Hosgood 22.nov.83
 *
 * History:
 * Version 1    Written 22.nov.83
 * Version 1.1  Altered to allow different target hardware, 14.feb.84
 */

# include	<stdio.h>
# include	"../ass.out.h"
# include       "list.h"
# include       "main.h"

# define        NPOOL           2000

LIST *lfree, *lstart;
LIST pool[NPOOL];

int poolindex;

/*
 * Initlist - set up empty free list, pointers etc.
 */
initlist()
{

	poolindex = 1;
	lfree = NULLIST;
	lstart = pool;
	lstart->type = UNREF;
	lstart->first = ( ADDR ) 0;
	lstart->last = 0;
	lstart->next = NULLIST;
	return;
}

/*
 * Entername - creates list element using 'enternew', fills in known
 *              name of identifier.
 */
LIST *
entername(type, addr, name)
ADDR addr;
char *name;
{
	LIST *lptr;

	lptr = enternew(type, addr, ( LIST * ) 0);
	lptr->name = name;
	return lptr;
}

/*
 * Enternew - create a list element, link in to list.
 *              'Cur' points to your current block, or is null.
 *                      if the former, routine returns current block ptr,
 *                      possibly modified; else it returns pointer to
 *                      list element created.
 */
LIST *
enternew(type, first, cur)
ADDR first;
LIST *cur;
{
	LIST *nptr, *lptr, *last;

	/* also check 'first' to see if it's in this code */
	if (first > dollr) {
		fprintf(stderr, "Address 0%o outside code\n", first);
		return cur;
	}

	/* scan flow list for place to put this entry */
	last = NULLIST;
	for (lptr = lstart; lptr && (lptr->first < first); lptr = lptr->next)
		last = lptr;

	if (!lptr || (lptr->first > first))
		lptr = last;

	if (lptr && lptr->first == first){
		/* entry already exists */
		if (lptr->type > type)
			/* redefine element if priority of new def is higher */
			lptr->type = type;

		/* make nptr point to this node.. */
		nptr = lptr;
	}
	else {
		/* brand new type of entry .. */
		/* first, get a free list element */
		if (lfree){
			nptr = lfree;
			lfree = lfree->next;
		}
		else {
			nptr = &pool[poolindex++];
			if (poolindex >= NPOOL){
				fprintf(stderr, "Fatal: Out of pool\n");
				exit(-1);
			}
		}

		/* fill in details */
		nptr->type = type;
		nptr->first = first;
		nptr->last = 0;

		/* link in after 'lptr' */
		if (lptr){
			nptr->next = lptr->next;
			lptr->next = nptr;

			/* if lptr is explored, see if we're cutting in */
			if (lptr->last && lptr->last >= first){
				nptr->last = lptr->last;
				lptr->last = first - 1;
			}
		}
		else {
			nptr->next = lstart;
			lstart = nptr;
		}

		/* check for branches backwards into current flow block */
		if (lptr == cur && first < cur_pc()){
			/* move into new block, fix old one */
			cur = nptr;
			lptr->last = nptr->first - 1;
		}
	}

	return cur? cur: nptr;
}

/*
 * Findsym - scan flow list for block which starts at addr given. Return
 *              null if no such block.
 */
LIST *
findsym(addr)
ADDR addr;
{
	LIST *ptr;

	ptr = lstart;
	while (ptr && ptr->first < addr)
		ptr = ptr->next;

	if (ptr && ptr->first > addr)
		ptr = NULLIST;

	return ptr;
}

/*
 * Poss_lab - if value could be a label, print it.
 */
poss_lab(addr)
ADDR addr;
{
	LIST *ptr;

	if (ptr = findsym(addr))
		/* sure enough! */
		pr_lab(ptr);
	else {
		putchar('$');
		praddr(addr);
	}

	return;
}

/*
 * Pr_lab - print label corresponding to given flow block ptr
 */
pr_lab(ptr)
LIST *ptr;
{

	switch (ptr->type){
	case PRG:
		printf("prg");
		break;

	case ENT:
		printf("ent");
		break;

	case SBR:
		printf("sbr");
		break;

	case DAT:
		printf("dat");
		break;

	case BTBL_1:
	case BTBL_2:
		printf("tbl");
		break;

	case GNAME:
	case LNAME:
		printf("%s", ptr->name);
	}

	if (ptr->type != GNAME && ptr->type != LNAME)
		printf("%d", ptr->lab);

	return;
}
