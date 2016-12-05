# include       "include/param.h"
# include       "include/buf.h"
# include       "include/dev.h"

struct buf bbuf[NBUF];
struct buf *b_first;
int b_index;

/*
 * Getbuf - searches cache for a given buffer, if none, reads one.
 */
struct buf *
getbuf(mdev, minor, block)
struct dev *mdev;
{
	int (*stratfnc)();
	struct buf *b_ptr, *b_prev, *b_prev2;

	stratfnc = mdev->stratfnc;
	b_ptr = b_first;
	b_prev = 0;
	b_prev2 = 0;

	while (b_ptr){
		if (b_ptr->b_bno == block){
			/* accept block if joke blk, or correct device */
			if (block == -1 || (b_ptr->b_dev == minor && b_ptr->b_strat == stratfnc)){
				/* found block! - link to start of list */
				if (b_prev){
					b_prev->b_next = b_ptr->b_next;
					b_ptr->b_next = b_first;
					b_first = b_ptr;
				}

				return b_ptr;
			}
		}

		/* try next one */
		b_prev2 = b_prev;
		b_prev = b_ptr;
		b_ptr = b_ptr->b_next;
	}

	/* not there - if free blocks exist, use one, else evict last block */
	if (b_index < NBUF)
		b_ptr = &bbuf[b_index++];
	else {
		/* evict - write dirty block */
		b_ptr = b_prev;
		b_prev = b_prev2;
		relbuf(b_ptr);
	}

	/* read in new data from disk */
	b_ptr->b_dev = minor;
	b_ptr->b_strat = stratfnc;
	b_ptr->b_flags = 0;
	if ((b_ptr->b_bno = block) != -1)
		/* not a joke block, do a real read */
		(*b_ptr->b_strat)(b_ptr);
	else
		clr_512(b_ptr->b_buf);

	/* link to start of list */
	if (b_prev)
		b_prev->b_next = 0;

	b_ptr->b_next = b_first;
	b_first = b_ptr;
	return b_ptr;
}

/*
 * Relbuf - frees a buffer in the cache. Writes buffer if dirty.
 */
relbuf(buf)
struct buf *buf;
{

	if (buf->b_flags & WRITE){
		if (buf->b_bno != -1)
			(*buf->b_strat)(buf);
		else
			/* joke block is caused by read on file with hole */
			panic("write of joke block?");
	}

	buf->b_flags = 0;
	return;
}

/*
 * Bflush - called by 'sync' to write changed buffers to disk.
 */
bflush()
{
	struct buf *b_ptr;

	b_ptr = b_first;
	while (b_ptr){
		relbuf(b_ptr);
		b_ptr = b_ptr->b_next;
	}

	return;
}

/*
 * Clr_512 - holes in files look like they're filled with NULs
 */
clr_512(cptr)
char *cptr;
{
	int i;

	for (i = 512; i; i--)
		*cptr++ = 0;

	return;
}
