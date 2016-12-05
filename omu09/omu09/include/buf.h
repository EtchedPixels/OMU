/*
 * Block I/O buffer structure.
 */

struct buf {
	int		b_bno;
	int		b_dev;
	int		b_flags;
	char		b_buf[512];
	int		(*b_strat)();
	struct buf	*b_next;
};

/*
 * Values of 'flags' field
 */

# define	ABORT		0x01
# define	WRITE		0x02

/*
 * Some functions don't return integers
 */

struct buf *getbuf();
struct buf *getibuf();
