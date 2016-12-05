/*
 * Super-Block processing (free list etc).
 * S.Hosgood 5.mar.84
 */

# define        NICFREE         50
# define        NICINOD         100

# include       "include/types.h"
# include       "include/inode.h"
# include       "include/filsys.h"
# include       "include/buf.h"
# include       "include/fblk.h"
# include       "include/dev.h"
# include       "include/ino.h"

# define        SUPERB          1

long time();

/*
 * Getfree - removes a free block from a disk, returns its number.
 *              Block is cleared.
 */
getfree(mdev, min_dev)
struct dev *mdev;
{
	int fb, count;
	struct buf *s_buf, *fb_buf;
	struct filsys *s_blk;
	struct fblk *fb_ptr;

	fb = 0;

	/* obtain superblock for this volume */
	s_buf = getbuf(mdev, min_dev, SUPERB);
	s_blk = ( struct filsys * ) s_buf->b_buf;

	/* check if any free list */
	if (s_blk->s_nfree){
		/* free blocks exist */
		fb = s_blk->s_free[--s_blk->s_nfree].lo;
		s_buf->b_flags |= WRITE;
	}

	/* check if we need secondary free block search */
	if (fb){
		if (s_blk->s_nfree == 0){
			/* read in next block in free chain */
			fb_buf = getbuf(mdev, min_dev, fb);
			fb_ptr = ( struct fblk * ) fb_buf->b_buf;
			s_blk->s_nfree = fb_ptr->df_nfree;

			for (count = 0; count < s_blk->s_nfree; count++)
				s_blk->s_free[count].lo = fb_ptr->df_free[count].lo;
		}
	}
	else
		printf("No space on disk %d\n", min_dev);

	/* read in and clear free block */
	if (fb){
		fb_buf = getbuf(mdev, min_dev, fb);
		clear_b(fb_buf->b_buf);
		fb_buf->b_flags |= WRITE;
	}

	return fb;
}

/*
 * Makefree - makes a block appear on the free-list.
 */
makefree(mdev, min_dev, bno)
struct dev *mdev;
{
	int count;
	struct buf *s_buf, *fb_buf;
	struct filsys *s_blk;
	struct fblk *fb_ptr;

	/* obtain superblock for this volume */
	s_buf = getbuf(mdev, min_dev, SUPERB);
	s_blk = ( struct filsys * ) s_buf->b_buf;

	/* check if we need to use a new chain block */
	if (s_blk->s_nfree >= NICFREE){
		/* get block to be freed, use as chain block */
		fb_buf = getbuf(mdev, min_dev, bno);
		fb_ptr = ( struct fblk * ) fb_buf->b_buf;
		fb_ptr->df_nfree = s_blk->s_nfree;

		for (count = 0; count < s_blk->s_nfree; count++){
			fb_ptr->df_free[count].lo = s_blk->s_free[count].lo;
			fb_ptr->df_free[count].hi = 0;
		}

		fb_buf->b_flags |= WRITE;
		s_blk->s_nfree = 0;
	}

	/* enter block to be freed into superblock */
	s_blk->s_free[s_blk->s_nfree++].lo = bno;
	s_buf->b_flags |= WRITE;
	return;
}

/*
 * Lockfree - returns pointer to a locked inode from the free inode list.
 */
struct inode *
lockfree(mdev, min_dev)
struct dev *mdev;
{
	int fi, count;
	long now;
	struct buf *s_buf;
	struct filsys *s_blk;
	struct inode *fi_ptr;

	s_buf = getbuf(mdev, min_dev, SUPERB);
	s_blk = ( struct filsys * ) s_buf->b_buf;
	fi_ptr = NULLIPTR;

	/* scan down free i-list. */
	while (s_blk->s_ninode){
		fi = s_blk->s_inode[--s_blk->s_ninode];
		s_buf->b_flags |= WRITE;
		if (fi_ptr = getiptr(mdev, min_dev, fi)){
			if (fi_ptr->i_mode == 0)
				/* found a clear inode */
				break;

			printf("inode %d was busy\n", fi);
			freeiptr(fi_ptr);
			fi_ptr = NULLIPTR;
		}
	}

	/* if list exhausted, scan for a free inode */
	if (! fi_ptr) {
		for (fi = (s_blk->s_isize - 2) * INOPB; fi; fi--) {
			if (fi_ptr = getiptr(mdev, min_dev, fi)) {
				if (fi_ptr->i_mode == 0)
					/* this is clear */
					break;

				freeiptr(fi_ptr);
				fi_ptr = NULLIPTR;
			}
		}
	}

	if (fi_ptr) {
		/* make mode non-zero to indicate capture */
		fi_ptr->i_mode = 1;
		fi_ptr->i_uid = fi_ptr->i_gid = 0;

		/* set create time etc */
		now = time(( long * ) 0);
		fi_ptr->i_atime = fi_ptr->i_mtime = fi_ptr->i_ctime = now;

		for (count = 0; count < 13; count++)
			fi_ptr->i_addr[count] = 0;

		fi_ptr->i_type |= I_WRITE;
		fi_ptr->i_size = 0;
	}
	else
		printf("no inodes on disk %d\n", min_dev);

	return fi_ptr;
}

/*
 * Makeifree - enters inode in free inode list.
 */
makeifree(mdev, min_dev, fi)
struct dev *mdev;
{
	struct buf *s_buf;
	struct filsys *s_blk;

	s_buf = getbuf(mdev, min_dev, SUPERB);
	s_blk = ( struct filsys * ) s_buf->b_buf;

	if (s_blk->s_ninode < NICINOD){
		s_blk->s_inode[s_blk->s_ninode++] = fi;
		s_buf->b_flags |= WRITE;
	}

	return;
}

/*
 * Clear_b - clears 512 byte buffer area to zeroes.
 */
clear_b(ptr)
char *ptr;
{
	int i;

	for (i = 0; i < 512; i++)
		*ptr++ = 0;

	return;
}

/*
 * Set_time - sets system clock from superblock time.
 */
set_time()
{
	struct buf *s_buf;
	struct filsys *s_blk;

	/* get root volume superblock */
	s_buf = getbuf(&bdevsw[0], 0, SUPERB);
	s_blk = ( struct filsys * ) s_buf->b_buf;

	setime(s_blk->s_time);
	return;
}
