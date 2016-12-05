/*
 * Perform mapping from logical file concepts to physical blocks.
 */

# include       "include/buf.h"
# include       "include/inode.h"

# define        B_READ          1
# define        B_WRITE         2

/*
 * Physblk - returns physical block given logical blk no. and inode ptr.
 *              if writing, and new block reqd, get one from free list.
 */
physblk(n, i_ptr, mode)
int n;
struct inode *i_ptr;
{
	int	rblk, d, i;

# ifdef MULTI
	int	ii, iii;

	ii = iii = -1;
# endif

	i = -1;

	/* find how much indirection will be reqd */
	if (n < 10)
		/* direct access */
		d = n;
	else if (n < 10+128){
		/* single indirect access */
		d = 10;
		i = n-10;
	}
	else {
		/* multiple indirect not allowed (yet) */
		printf("Block %d??\n", n);
		return 0;
	}

	if ((rblk = i_ptr->i_addr[d]) == 0 && mode == B_WRITE){
		i_ptr->i_addr[d] = rblk = getfree(i_ptr->i_mdev, i_ptr->i_minor);
		i_ptr->i_type |= I_WRITE;
	}

	/* check if indirection reqd */
	if (rblk && i >= 0){
		rblk = scan_indir(rblk, i, i_ptr, mode);

# ifdef MULTI
		if (rblk && ii >= 0){
			rblk = scan_indir(rblk, ii, i_ptr, mode);

			if (rblk && iii >= 0){
				rblk = scan_indir(rblk, iii, i_ptr, mode);
			}
		}
# endif
	}

	return rblk;
}

/*
 * Scan_indir - returns block number at position 'n' in indirect block 'blk'.
 */
scan_indir(blk, n, i_ptr, mode)
int blk, n;
struct inode *i_ptr;
{
	int rblk;
	struct buf *b_ptr;
	struct indir *ind_ptr;

	/* get indirect block */
	b_ptr = getbuf(i_ptr->i_mdev, i_ptr->i_minor, blk);
	ind_ptr = ( struct indir * ) b_ptr->b_buf;

	/* obtain element 'n' - create block if reqd */
	if ((rblk = ind_ptr[n].ind_addr) == 0 && mode == B_WRITE){
		rblk = ind_ptr[n].ind_addr = getfree(i_ptr->i_mdev, i_ptr->i_minor);
		b_ptr->b_flags |= WRITE;
	}

	return rblk;
}

/*
 * Itrunc - clears an inode, releasing all blocks.
 */
itrunc(iptr)
struct inode *iptr;
{
	int count, fb;

	for (count = 0; count < 10; count++){
		if (fb = iptr->i_addr[count]){
			makefree(iptr->i_mdev, iptr->i_minor, fb);
			iptr->i_addr[count] = 0;
		}
	}

	/* first indirect block.. */
	if (fb = iptr->i_addr[10]){
		clr_1_indir(iptr->i_mdev, iptr->i_minor, fb);
		iptr->i_addr[10] = 0;
	}

	iptr->i_size = 0;
	iptr->i_type |= I_WRITE;

	return;
}

/*
 * Clr_1_indir - clears a first level indirect block.
 */
clr_1_indir(mdev, minor, blk)
struct dev *mdev;
{
	int count, fb;
	struct indir *ind_ptr;
	struct buf *b_ptr;

	/* get the indirect block */
	b_ptr = getbuf(mdev, minor, blk);
	ind_ptr = ( struct indir * ) b_ptr->b_buf;

	/* now release all blocks pointed-to */
	for (count = 0; count < 128; count++){
		if (fb = ind_ptr->ind_addr)
			makefree(mdev, minor, fb);

		ind_ptr++;
	}

	/* and free indirect block itself */
	makefree(mdev, minor, blk);

	return;
}
