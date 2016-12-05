/*
 * Structure of a flow list element, etc
 */

struct list {
	int type;
	int lab;
	ADDR first;
	ADDR last;
	struct list *next;
	char *name;
};

typedef struct list LIST;

/*
 * Values that 'type' of a list may assume
 */

# define	GNAME	1		/* global known name */
# define	LNAME	2		/* local known name */
# define	ENT	3
# define	SBR	4
# define	PRG	5
# define	BTBL_1	6
# define	BTBL_2	7
# define	DAT	8
# define	UNREF	9		/* least priority symbol! */

/*
 * Global list constants etc.
 */

extern LIST *lstart, *lfree;
extern LIST pool[];

# define	NULLIST		(( LIST *) '\0')

/*
 * Some functions don't return integers
 */

extern LIST *enternew();
extern LIST *entername();
extern LIST *findsym();
