# include       <sys/types.h>
# include       <sys/stat.h>
# include       "include/param.h"
# include       "include/procs.h"
# include       "include/file.h"
# include       "include/inode.h"
# include       "include/uft.h"

# define        OPEN            SEARCH
# define	VOID		char

int f_open(), f_creat();
long f_seek();
struct uft uft[NFILES], *check_fd();

/*
 * Open - makes an entry in User File Table, returns file descriptor.
 */
open(name, mode)
char *name;
{

	return gen_open(name, mode+1, OPEN, 0, f_open);
}

/*
 * Creat - makes an entry in User File Table, returns file descriptor.
 */
creat(name, mode)
char *name;
{

	return gen_open(name, 2, CREATE, mode, f_creat);
}

/*
 * Read - check supplied file descriptor, perform a read if mode ok.
 */
read(fd, buffer, nbytes)
char *buffer;
{
	struct uft *uft_ptr;

	uft_ptr = check_fd(fd);
	if (uft_ptr && (uft_ptr->uf_mode & READABLE))
		return f_read(&uft_ptr->uf_file, buffer, nbytes);

	/* file not opened for reading */
	return -1;
}

/*
 * Write - check supplied file descriptor, perform a write if mode ok.
 */
write(fd, buffer, nbytes)
char *buffer;
{
	struct uft *uft_ptr;

	uft_ptr = check_fd(fd);
	if (uft_ptr && (uft_ptr->uf_mode & WRITEABLE))
		return f_write(&uft_ptr->uf_file, buffer, nbytes);

	/* file not opened for writing */
	return -1;
}

/*
 * Lseek - check supplied file descriptor, perform a seek if fd ok.
 */
long
lseek(fd, off, mode)
long off;
{
	struct uft *uft_ptr;

	if (uft_ptr = check_fd(fd))
		return f_seek(&uft_ptr->uf_file, off, mode);

	/* file not open */
	return -1;
}

/*
 * Ioctl - check descriptor, perform io-control if it's valid.
 */
ioctl(fd, request, argp)
VOID *argp;
{
	struct uft *uft_ptr;

	if (uft_ptr = check_fd(fd))
		return f_ioctl(&uft_ptr->uf_file, request, argp);

	/* file not open */
	return -1;
}

/*
 * Fstat - return information concerning open file descriptor.
 */
fstat(fd, ptr)
struct stat *ptr;
{
	struct uft *uft_ptr;

	if (uft_ptr = check_fd(fd)) {
		istat(uft_ptr->uf_file.f_inode, ptr);
		return 0;
	}

	/* file not open */
	return -1;
}

/*
 * Stat - return information concerning named file.
 */
stat(name, ptr)
char *name;
struct stat *ptr;
{
	struct inode *iptr;

	/* attempt to get inode */
	if (iptr = namlock(name, SEARCH, NULLIPTR)) {
		istat(iptr, ptr);
		freeiptr(iptr);
		return 0;
	}

	/* couldn't read inode */
	return -1;
}

/*
 * Close - check supplied file descriptor, perform a close if fd ok.
 */
close(fd)
{
	struct uft *uft_ptr;

	if (uft_ptr = check_fd(fd)) {
		cur_proc->slots[fd] = 0;

		if (--uft_ptr->uf_ndup <= 0) {
			/* really close file on last duplicate close */
			uft_ptr->uf_mode = 0;
			return f_close(&uft_ptr->uf_file);
		}

		/* return OK on non-last closes */
		return 0;
	}

	/* file not open */
	return -1;
}

/*
 * Sync - writes changed inodes and blocks back to disk.
 */
sync()
{

	iflush();
	bflush();
	return;
}

/*
 * Gen_open - deal with open or creat calls..
 */
gen_open(name, mode, type, perm, fnc)
char *name;
int (*fnc)();
{
	int fd, slot;
	struct uft *uft_ptr, **slot_ptr;
	struct inode *iptr;

	slot_ptr = cur_proc->slots;
	for (slot = 0; slot < NFPERU; slot++) {
		if (! *slot_ptr) {
			/* user has slot free - has uft got space? */
			uft_ptr = uft;
			for (fd = 0; fd < NFILES; fd++) {
				if (uft_ptr->uf_mode == 0) {
					/* found an entry so use it */
					uft_ptr->uf_mode = mode;

					/* attempt to get inode */
					if (iptr = namlock(name, type, NULLIPTR)) {
						/* no write to directory */
						if ( !(isdir(iptr) && mode >= 2)) {
							if ((*fnc)(&uft_ptr->uf_file, iptr, perm) != -1) {
								*slot_ptr = uft_ptr;
								uft_ptr->uf_ndup = 1;
								return slot;
							}
						}

						freeiptr(iptr);
					}

					/* failed to open file - free slot again */
					uft_ptr->uf_mode = 0;
					return -1;
				}

				uft_ptr++;
			}

			/* no file */
			printf("no file\n");
			return -1;
		}

		slot_ptr++;
	}

	/* no file slot */
	printf("no slot\n");
	return -1;
}

/*
 * Check_fd - returns ptr to uft entry for file given by user's fd.
 */
struct uft *
check_fd(fd)
{

	if (fd < 0 || fd >= NFPERU)
		/* illegal fd */
		return 0;

	return cur_proc->slots[fd];
}

/*
 * Dup_all - makes dups of open files in parent, gives to child.
 */
dup_all(par, child)
struct u_regs *par, *child;
{
	int t;
	struct uft **p_ptr, **c_ptr;

	p_ptr = par->slots;
	c_ptr = child->slots;

	for (t = 0; t < NFPERU; t++)
		if (*p_ptr) {
			(*p_ptr)->uf_ndup++;
			*c_ptr++ = *p_ptr++;
		}

	return;
}

/*
 * Dup - duplicate file descriptor.
 */
dup(fd)
{
	int t;
	struct uft *old, **new;

	if (old = check_fd(fd)) {
		new = cur_proc->slots;
		for (t = 0; t < NFPERU; t++) {
			if (! *new) {
				*new = old;
				old->uf_ndup++;
				return t;
			}

			new++;
		}

		/* error - no slot */
		return -1;
	}

	/* error original wasn't open */
	return -1;
}
