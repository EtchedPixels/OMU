# include	<stdio.h>
# include	<sys/types.h>
# include	<sys/stat.h>
# include	<sys/dir.h>

/*
 * C Library: Dsearch(name, fnc) applies 'fnc' recursively to all files
 *		below 'name'.
 */
dsearch(name, fnc)
char *name;
int (*fnc)();
{
	char		direntry[100], *eptr;
	struct stat	status;
	struct direct	dir;
	FILE		*dirid;

	if (stat(name, &status) == 0) {
		/* directory ? */
		if ((status.st_mode & S_IFMT) == S_IFDIR &&
		    (dirid = fopen(name, "r")) != NULL) {
			/* directory opened: apply dsearch recursively */
			strcpy(direntry, name);
			for (eptr = direntry; *eptr; eptr++);
			*eptr++ = '/';

			/* first 2 entries are special */
			if (fread(&dir, sizeof dir, 1, dirid) == 1) {
				strncpy(eptr, dir.d_name, DIRSIZ);
				apply(direntry, fnc);
			}

			if (fread(&dir, sizeof dir, 1, dirid) == 1) {
				strncpy(eptr, dir.d_name, DIRSIZ);
				apply(direntry, fnc);
			}

			while (fread(&dir, sizeof dir, 1, dirid) == 1) {
				if (dir.d_ino) {
					strncpy(eptr, dir.d_name, DIRSIZ);
					dsearch(direntry, fnc);
				}
			}

			/* end of directory */
			fclose(dirid);
		}

		/* now apply fnc to 'name' */
		(*fnc)(name, &status);
	}
	/* else, couldn't 'stat' the given name */

	return;
}

apply(name, fnc)
char *name;
int (*fnc)();
{
	struct stat	status;

	if (stat(name, &status) == 0)
		(*fnc)(name, &status);

	return;
}
