# include       "include/buf.h"
# include       "include/inode.h"
# include       "include/file.h"
# include       "include/dev.h"

# define	VOID	char

/*
 * F_open - file pointed-to be 'file' is opened and uses locked inode 'iptr'.
 */
f_open(file, iptr)
struct inode *iptr;
struct file *file;
{
	int devcode, (*fnc)();

	if (iptr) {
		file->f_inode = iptr;
		file->f_curblk = 0;
		file->f_curchar = 0;

		switch (file->f_type = (iptr->i_type & 0xFF)) {
		case STD:
			file->f_handler = iptr->i_mdev;
			file->f_dev = iptr->i_minor;
			break;

		case B_SPECL:
			devcode = iptr->i_addr[0];
			file->f_handler = &bdevsw[devcode >> 8];
			file->f_dev = devcode & 0xFF;
			break;

		case C_SPECL:
			devcode = iptr->i_addr[0];
			file->f_handler = &cdevsw[devcode >> 8];
			file->f_dev = devcode & 0xFF;
			break;

		default:
			/* file type unknown */
			return -1;
		}

		/* File open - open device it's using */
		if (fnc = file->f_handler->openfnc)
			return (*fnc)(file->f_dev);

		/* if no open fnc, assume it's not needed */
		return 0;
	}

	/* no access */
	return -1;
}

/*
 * F_creat - create a standard type file ready for writing. Uses locked
 *              inode 'iptr'. If 'iptr' had existed, and it was a regular
 *              file, then it is truncated.
 */
f_creat(file, iptr, mode)
struct inode *iptr;
struct file *file;
{

	if (iptr) {
		/* check if file did exist */
		if (iptr->i_mode == 1) {
			/* new file */
			iptr->i_mode = S_IFREG | (mode & 07777);
		}
		else if ((iptr->i_mode & S_IFMT) == S_IFREG) {
			/* truncate old file */
			itrunc(iptr);
		}

		return f_open(file, iptr);
	}

	/* no access */
	return -1;
}

f_write(file, buffer, nbytes)
struct file *file;
char *buffer;
{

	switch (file->f_type) {
	case STD:
	case B_SPECL:
		return b_write(file, buffer, nbytes);

	case C_SPECL:
		return c_write(file, buffer, nbytes);
	}

	/* type unknown */
	return -1;
}

f_ioctl(file, request, argp)
struct file *file;
VOID *argp;
{

	switch (file->f_type) {
	case C_SPECL:
		return c_ioctl(file, request, argp);
	}

	/* invalid file type.. */
	return -1;
}

f_read(file, buffer, nbytes)
struct file *file;
char *buffer;
{

	switch (file->f_type) {
	case STD:
	case B_SPECL:
		return b_read(file, buffer, nbytes);

	case C_SPECL:
		return c_read(file, buffer, nbytes);
	}

	/* type unknown */
	return -1;
}

long
f_seek(file, pos, mode)
long pos;
struct file *file;
{
	long b_seek();

	switch (file->f_type) {
	case STD:
	case B_SPECL:
		return b_seek(file, pos, mode);

	case C_SPECL:
		return 0;
	}

	/* type unknown */
	return -1;
}

f_close(file)
struct file *file;
{
	int (*fnc)();

	freeiptr(file->f_inode);

	/* close device which file used */
	if (fnc = file->f_handler->closefnc)
		return (*fnc)(file->f_dev);

	/* if no close function, assume device needs no closing */
	return 0;
}
