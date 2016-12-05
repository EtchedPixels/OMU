/*
 * Inode.c - Manipulations of incore inode copies.
 */

# include       "include/types.h"
# include       <sys/dir.h>
# include       <sys/stat.h>
# include       "include/file.h"
# include       "include/inode.h"
# include       "include/dev.h"
# ifdef MOUNT
# include	"include/mount.h"
# endif
# ifdef TERRY
# include	"include/param.h"
# include	"include/procs.h"
# else

# define        ROOTINO         2

/* user data */
struct inode *user_curdir;

# endif
mknod(name, mode, addr)
int mode,addr;
char *name;
{
	int status;
	struct inode *i_ptr;

	status = -1;
	if (i_ptr = namlock(name, CREATE, NULLIPTR)) {
		if (i_ptr->i_mode == 1) {
			/* set attributes */
			i_ptr->i_mode = mode;
			i_ptr->i_addr[0] = addr;
			status = 0;
		}
		else
			/* node already there */
			status = -1;

		freeiptr(i_ptr);
	}

	return status;
}

chdir(path)
char *path;
{
	struct inode *iptr;

	if (iptr = namlock(path, SEARCH, NULLIPTR)) {
		/* check this too is a directory */
		if (isdir(iptr)) {
			/* ok - allow change */
# ifdef TERRY
			if (cur_proc->wd)
				freeiptr(cur_proc->wd);
			cur_proc->wd = iptr;
# else
			if (user_curdir)
				freeiptr(user_curdir);
			user_curdir = iptr;
# endif
			return 0;
		}

		/* not directory - free locked inode */
		freeiptr(iptr);
	}

	return -1;
}

/*
 * Link - create a new link to a file.
 */
link(name1, name2)
char *name1, *name2;
{
	int status;
	struct inode *iptr;

	status = -1;
	if (iptr = namlock(name1, SEARCH, NULLIPTR)) {
		if (namlock(name2, MKLINK, iptr)) {
			/* sucessful link creation */
			iptr->i_nlink++;
			iptr->i_type |= I_WRITE;
			status = 0;
		}

		freeiptr(iptr);
	}

	return status;
}

/*
 * Unlink - remove directory entry.
 */
unlink(name)
char *name;
{
	struct inode *iptr;

	if (iptr = namlock(name, DELETE, NULLIPTR)) {
		/* decrement link count */
		iptr->i_nlink--;
		iptr->i_type |= I_WRITE;
		freeiptr(iptr);
		return 0;
	}

	return -1;
}

/*
 * Chmod - changes mode of file with given name.
 */
chmod(name, mode)
int mode;
char *name;
{
	struct inode *iptr;

	if (iptr = namlock(name, SEARCH, NULLIPTR)) {
		iptr->i_mode &= 0170000;
		iptr->i_mode |= mode & 07777;
		iptr->i_type |= I_WRITE;
		freeiptr(iptr);
		return 0;
	}

	return -1;
}

/*
 * Namlock, given a name, search directories and return a pointer to a
 *              locked inode (or NULL) corresponding to the name.
 *              Mode may be 'SEARCH', 'CREATE' or 'DELETE': the directory
 *              may therefore get modified.
 */
struct inode *
namlock(name, mode, iptr)
char *name;
int mode;
struct inode *iptr;
{
	char *cptr, ch;
	struct inode *cur_i;

	/* if name starts with '/' start at root */
	if (*name == '/') {
		cur_i = getiptr(&bdevsw[0], 0, ROOTINO);
		name++;
	}
	else if (*name == ':' ) {
		cur_i = 0;
		/* allow funnies to permit use with no /dev directory */
		if (!strcmp(name+1, "console")) {
			/* non standard alias for console dev [C 0 0] */
			cur_i = lockfree(&bdevsw[0], 0);
			cur_i->i_type = C_SPECL;
			cur_i->i_addr[0] = 0;
		}
		else if (!strcmp(name+1, "fd0")) {
			/* non standard alias for root dev [B 0 0] */
			cur_i = lockfree(&bdevsw[0], 0);
			cur_i->i_type = B_SPECL;
			cur_i->i_addr[0] = 0;
		}

		return cur_i;
	}
	else
		/* use current directory */
# ifdef TERRY
		cur_i = relock(cur_proc->wd);
# else
		cur_i = relock(user_curdir);
# endif

	/* loop while some of 'name' exists, reading directories */
	while (cur_i) {
		/* scrap spare '/' s */
		while (*name == '/')
			name++;

		/* check name still exists */
		if (*name == '\0')
			break;

		/* skip to next delimiter and make null */
		for (cptr = name; ((ch = *cptr) && ch != '/'); cptr++);

		/* cptr points to delimiter, 'ch' is delimiter */
		*cptr = '\0';
		cur_i = srchdir(name, cur_i, (ch == '\0')? mode: SEARCH, iptr);

		/* if delimiter was '/' carry on, else finish */
		if (ch == '/') {
			*cptr++ = '/';
			name = cptr;
		}
		else
			break;
	}

	return cur_i;
}

/*
 * Srchdir - a pointer to a directory inode is reqd. Mode may be:-
 *      CREATE - look for name, if there, truncate file, else make entry and
 *              get blank inode to match.
 *      DELETE - expect name to exist, lock inode, remove entry in dir.
 *      SEARCH - just search directory, lock inode if present, else return 0.
 *      MKLINK - look for name, error if found, else create name pointing
 *              to locked inode 'l_iptr'.
 *
 *      In all cases, 0 return is a failure.
 *	In all sucessful cases, a pointer to a locked inode is returned.
 *      In all cases the directory inode pointer is unlocked.
 */
struct inode *
srchdir(name, iptr, mode, l_iptr)
char *name;
struct inode *iptr, *l_iptr;
int mode;
{
# ifdef MOUNT
	int freeslot, nami, cur_slot, update, mounted;
# else
	int freeslot, nami, cur_slot, update;
# endif
	struct direct dbuf;
	struct file file;
	struct inode *c_iptr;

# ifdef MOUNT
	mounted = update = nami = 0;
# else
	update = nami = 0;
# endif
	freeslot = -1;
	cur_slot = 0;

# ifdef ORIG
	c_iptr = NULLIPTR;
# endif

	if (isdir(iptr)) {
# ifdef MOUNT
		if (strcmp(name, "..") == 0) {
			/* '..' may be on a different device */
			while (iptr->i_ino == ROOTINO && (c_iptr = m_volparent(iptr)))
				/* it was, so look for ".." on parent dir */
				iptr = c_iptr;
		}

		/* Search directory - possibly 'under' mounted volume */
# endif
		if (f_open(&file, relock(iptr)) != -1) {
			while (1) {
				/* stop if directory exhausted */
				/* marginally quicker to use b_read not f_read etc */
				if (b_read(&file, ( char * ) &dbuf, sizeof dbuf) != sizeof dbuf) {

					/* if possible, position at empty slot */
					if (freeslot != -1)
						b_seek(&file, ( long ) freeslot, 0);
					break;
				}

				/* see if slot occupied */
				if (dbuf.d_ino) {
					/* stop if name matches */
					if (strncmp(dbuf.d_name, name, DIRSIZ) == 0) {
						nami = dbuf.d_ino;
						b_seek(&file, ( long ) cur_slot, 0);
						break;
					}
				}
				else {
					/* remember location of clear slot */
					if (freeslot == -1)
						freeslot = cur_slot;
				}

				/* note new position in directory and try again */
				cur_slot += sizeof dbuf;
			}

			/* may be positioned by wanted slot or a clear one */
			if (nami) {
# ifdef MOUNT
				/* get inode for file 'nami' - maybe mounted */
				c_iptr = m_getiptr(iptr, nami, &mounted);
# else
				/* at wanted slot */
				c_iptr = getiptr(iptr->i_mdev, iptr->i_minor, nami);
# endif

				switch (mode) {
				case DELETE:
# ifdef MOUNT
					if (mounted) {
						freeiptr(c_iptr);
						c_iptr = NULLIPTR;
					}
					else {
						dbuf.d_ino = 0;
						update = 1;
					}
# else
					dbuf.d_ino = 0;
					update = 1;
# endif
					break;

				case MKLINK:
					/* illegal if name exists */
					freeiptr(c_iptr);
					c_iptr = NULLIPTR;
					/* fall thru' */

				case CREATE:
				case SEARCH:
					break;
				}
			}
			else {
				/* positioned by free slot */
				switch (mode) {
				case DELETE:
				case SEARCH:
					/* error if name doesn't exist */
					c_iptr = NULLIPTR;
					break;

				case MKLINK:
					/* must ban cross-dev links (later) */
					dbuf.d_ino = l_iptr->i_ino;
					c_iptr = l_iptr;
					update = 1;
					break;

				case CREATE:
					if (c_iptr = lockfree(iptr->i_mdev, iptr->i_minor)) {
						/* created ok */
						dbuf.d_ino = c_iptr->i_ino;
						c_iptr->i_nlink++;
						update = 1;
					}
				}

				/* if updating, copy new name */
				if (update)
					strncpy(dbuf.d_name, name, DIRSIZ);
			}

			/* update directory if reqd */
			if (update)
				b_write(&file, ( char * ) &dbuf, sizeof dbuf);

			f_close(&file);
		}
	}
	else
		c_iptr = NULLIPTR;

	/* always free given directory inode */
	freeiptr(iptr);
	return c_iptr;
}

/*
 * Istat - returns contents of inode structure coded as per 'stat' and 'fstat'
 */
istat(iptr, statptr)
struct inode *iptr;
struct stat *statptr;
{

# ifdef MOUNT
	statptr->st_dev = makedev(iptr->i_mdev - &bdevsw[0], iptr->i_minor);
# else
	statptr->st_dev = makedev(0, iptr->i_minor);
# endif
	statptr->st_ino = iptr->i_ino;
	statptr->st_mode = iptr->i_mode;
	statptr->st_nlink = iptr->i_nlink;
	statptr->st_uid = iptr->i_uid;
	statptr->st_gid = iptr->i_gid;
	statptr->st_rdev = iptr->i_addr[0];
# ifdef TERRY
	statptr->st_size = iptr->i_size;
# else
	statptr->st_size.hi = 0;
	statptr->st_size.lo = iptr->i_size;
# endif
	statptr->st_atime = iptr->i_atime;
	statptr->st_mtime = iptr->i_mtime;
	statptr->st_ctime = iptr->i_ctime;
	return;
}
