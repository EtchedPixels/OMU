/*
 * Read and Write routines for files that are basically Block type.
 */

# include       "include/buf.h"
# include       "include/inode.h"
# include       "include/file.h"
# include       "include/dev.h"

# define        B_READ          1
# define        B_WRITE         2

long curpos();

b_read(file, buffer, nbytes)
struct file *file;
char *buffer;
{
	char *cptr;
	int n, xfr_cnt;
	long charsleft;
	struct buf *b;

	xfr_cnt = 0;
	if (file->f_type == STD) {
		/* limit read to stop at end of file or after */
		if ((charsleft = isize(file->f_inode) - curpos(file)) < 0)
			charsleft = 0;

		nbytes = (nbytes > charsleft)? charsleft: nbytes;
	}

	while (nbytes) {
		n = file->f_curblk;
		if (file->f_type == STD) {
			if ( !(n = physblk(n, file->f_inode, B_READ)))
				/* unallocated blocks read as nulls */
				n = -1;
			else if (n < 59 || n > 1440)
				/* block outside valid filestore area? */
				return -1;
		}

		/* get into cache if not already there */
		b = getbuf(file->f_handler, file->f_dev, n);

		/* check for error */
		if (b->b_flags & ABORT)
			return -1;

		cptr = &b->b_buf[file->f_curchar];

		while (nbytes && (file->f_curchar++ < 512)){
			*buffer++ = *cptr++;
			nbytes--;
			xfr_cnt++;
		}

		if (file->f_curchar >= 512){
			file->f_curchar = 0;
			file->f_curblk++;
		}
	}

	return xfr_cnt;
}

b_write(file, buffer, nbytes)
struct file *file;
char *buffer;
{
	char *cptr;
	int n, xfr_cnt;
	struct buf *b;

	xfr_cnt = 0;
	while (nbytes){
		n = file->f_curblk;
		if (file->f_type == STD){
			if ( !(n = physblk(n, file->f_inode, B_WRITE)))
				/* early exit if block not availiable */
				return xfr_cnt;
			else if (n < 59 || n > 1440)
				/* error if outside valid filestore area */
				return -1;
		}

		/* get block */
		b = getbuf(file->f_handler, file->f_dev, n);

		/* check for i/o error */
		if (b->b_flags & ABORT)
			return -1;

		cptr = &b->b_buf[file->f_curchar];
		b->b_flags |= WRITE;

		while (nbytes && (file->f_curchar++ < 512)){
			*cptr++ = *buffer++;
			nbytes--;
			xfr_cnt++;
		}

		if (file->f_curchar >= 512){
			file->f_curchar = 0;
			file->f_curblk++;
		}

		if (file->f_type == STD)
			/* if file is bigger now, change inode */
			iexpand(file->f_inode, curpos(file));
	}

	return xfr_cnt;
}

/*
 * B_seek - perform a seek on a block-structured file.
 */
long
b_seek(file, pos, mode)
long pos;
struct file *file;
{
	long cur;

	cur = curpos(file);
	switch (mode){
	case 0:
		/* set to absolute position */
		cur = pos;
		break;

	case 1:
		/* set pointer to current + offset */
		cur += pos;
		break;

	case 2:
		/* set pointer to end + offset */
		cur = isize(file->f_inode) + pos;
	}

	if (cur >= 0){
		/* file is only physically expanded by a write, not here */
		file->f_curchar = cur & 0x1FF;
		file->f_curblk = cur >> 9;
	}
	else
		/* attempt to point before start of file */
		cur = -1;

	return cur;
}

/*
 * Curpos - returns current byte in a file.
 */
long
curpos(fptr)
struct file *fptr;
{

	return (( long )(fptr->f_curblk) << 9) | (fptr->f_curchar & 0x1FF);
}
