/*
 * Primitive shell-type routine.
 * Pattern matching added, 2.mar.87
 *
 * S.Hosgood, 23.feb.84
 */

# include	<setjmp.h>

# define	NCHRS		200
# define	DIRSIZ		16
# define	NEXPND		50

char		*expv[NEXPND+1], **eptr;
char		strbuffer[NCHRS], *bptr;
char		*mkname();
int		expc, compar();
jmp_buf		err_retn;

/* have to define my own 'dir' struct however */
struct dir {
	int	d_ino;
	char	d_name[DIRSIZ + 2];
};

/* list of built-in commands */
struct comm {
	char *c_name;
	int (*c_routine)();
};

/* mention built-ins */
int cat(), cd(), sync(), cp(), rm(), mv();

struct comm comm[] = {
	"cat",  cat,
	"cd",   cd,
	"cp",   cp,
	"mv",   mv,
	"rm",   rm,
	"sync", sync,
	0,      0
};

char stdname[] = "/bin/XXXXXXXXXXXXXXX";
# define        STDLEN          5

main()
{
	char input[100], *aptr, *cptr;
	int nbytes, in;
	struct comm *cm_ptr;

# ifdef omu
	chdir("/");
	if ((in = open("/dev/console", 2)) == -1)
		if ((in = open(":console", 2)) == -1)
			panic("No console");

	if (in != 0)
		panic("Std input non-zero");

	/* create std out and std err */
	dup(0);
	dup(0);

	/* welcome messages here */
	printf("6809 One-Man-Unix System.\n");
	printf("Steve Hosgood, 1984-87\n");

# ifdef DATESET
	/* set time */
	argv[0] = "date";
	argv[1] = "-s";
	argv[2] = 0;
	xeqt("/bin/date", 2, argv);
# endif
# else
	in = 0;
# endif

	while (1) {
		/* come here on error */
		setjmp(err_retn);

		expc = 0;
		eptr = expv;
		bptr = strbuffer;

# ifdef omu
		putchar('$');
# else
		putchar('%');
# endif
		putchar(' ');

		if ((nbytes = read(in, input, 100)) == 0)
			break;
		else if (nbytes == 1)
			continue;

		/* null terminate */
		input[nbytes-1] = '\0';

		/* scan line, breaking into args */
		cptr = input;

		while (1) {
			/* miss leading spaces */
			while (*cptr && *cptr == ' ')
				cptr++;

			if (*cptr) {
				/* arg there */
				aptr = cptr;

				/* scan to end of word */
				while (*cptr && *cptr != ' ')
					cptr++;

				/* null terminate arg */
				if (*cptr == ' ')
					*cptr++ = '\0';

				/* expand this arg */
				expand(aptr);
			}
			else
				break;
		}

		/* last arg is NULL */
		*eptr = ( char * ) 0;

		/* try in current directory */
		if (xeqt(expv[0], expc, expv) == -1) {
			/* failed - try in standard bin */
			stdname[STDLEN] = '\0';
			strcat(stdname, expv[0]);
			if (xeqt(stdname, expc, expv) == -1) {
				/* failed there too - try built-in */
				cm_ptr = comm;
				while (cm_ptr->c_name && strcmp(cm_ptr->c_name, expv[0]) != 0)
					cm_ptr++;

				/* try and execute */
				if (cm_ptr->c_name)
					(*cm_ptr->c_routine)(expc, expv);
				else
					/* failed all attempts */
					printf("%s: not found\n", expv[0]);
			}
		}
	}

	exit(0);
}

expand(pattern)
char *pattern;
{
	char		*dir;
	int		nwild;

	dir = pattern;
	nwild = 0;

	/* scan for end of string */
	while (*pattern) {
		if (*pattern == '*' || *pattern == '?')
			nwild++;

		pattern++;
	}

	/* step back to last '/' or start of string */
	while (pattern > dir && *pattern != '/')
		pattern--;

	/* split directory and filename parts */
	if (*pattern == '/')
		*pattern++ = '\0';
	else if (pattern == dir)
		/* null signifies search of '.' */
		dir = ( char * ) 0;

	/* only search dir if wildcards exist and dir exists */
	if (nwild)
		search(dir, pattern);
	else {
		inc_expc();
		*eptr++ = mkname(dir, pattern);
	}

	return;
}

search(dir, pattern)
char *dir, *pattern;
{
	char		**base;
	int		nmatch, dirfd;
	struct dir	buff;

	base = eptr;
	nmatch = 0;

	if (dirfd = open(dir? dir: ".", 0)) {
		while (read(dirfd, ( char * ) &buff, 16) == 16) {
			/* ignore empty directory slots */
			if (buff.d_ino == 0)
				continue;

			/* files starting '.' must be matched specifically */
			if (*(buff.d_name) == '.' && *pattern != '.')
				continue;

			/* ensure name is null-terminated */
			buff.d_name[16] = '\0';

			if (match(buff.d_name, pattern)) {
				inc_expc();
				*eptr++ = mkname(dir, buff.d_name);
				nmatch++;
			}
		}

		/* close directory */
		close(dirfd);

		/* sort into order */
		if (nmatch > 1)
			qsort(( char * ) base, nmatch, sizeof( char * ), compar);
	}

	if (nmatch == 0) {
		/* maybe no wildcards given or search failed */
		inc_expc();
		*eptr++ = mkname(dir, pattern);
	}

	return;
}

match(str, pattern)
char *str, *pattern;
{
	char pat_ch, *sptr;

	while (pat_ch = *pattern) {
		switch (pat_ch) {
		case '*':
			/* match a sub-string */
			if ((pat_ch = *++pattern) == '\0')
				/* simple case of '*' at end of pattern */
				return 1;

			/* else, scan to end of string */
			for (sptr = str; *sptr; sptr++);
			sptr--;

			/* step back, matching char after '*'. */
			while (sptr >= str) {
				if (*sptr == pat_ch && match(sptr+1, pattern+1))
					return 1;

				/* else, go back another one */
				sptr--;
			}

			/* no match */
			return 0;

		case '?':
			/* just check we don't match NULL */
			return *str? match(str+1, pattern+1): 0;

		default:
			if (*str != pat_ch)
				/* fail to match specific char */
				return 0;
		}

		/* prepare for next char */
		pattern++;
		str++;
	}

	/* strings match if pattern and string both exhausted */
	return (*str == '\0');
}

char *
mkname(dir, name)
char *dir, *name;
{
	char		*str;

	str = bptr;

	if (dir) {
		while (bptr < &strbuffer[NCHRS-3] && *dir)
			*bptr++ = *dir++;

		*bptr++ = '/';
	}

	while (bptr < &strbuffer[NCHRS-2] && *name)
		*bptr++ = *name++;

	if (bptr < &strbuffer[NCHRS-1])
		*bptr++ = '\0';
	else {
		printf("Too many chars\n");
		longjmp(err_retn, -2);
	}

	return str;
}

inc_expc()
{

	if (expc >= NEXPND) {
		printf("Too many args\n");
		longjmp(err_retn, -1);
	}

	expc++;
	return;
}

compar(a, b)
char **a, **b;
{

	return strcmp(*a, *b);
}

/*
 * Built-in 'cat' command.
 */
cat(argc, argv)
char *argv[];
{
	char **filename;

	filename = &argv[1];
	argc--;

	while (argc--)
		catfile(*filename++);

	return;
}

catfile(name)
char *name;
{
	char buffer[512];
	int nbytes, fd;

	if ((fd = open(name, 0)) == -1){
		printf("cat: cannot open %s\n", name);
		return;
	}

	while (nbytes = read(fd, buffer, 512)){
		write(1, buffer, nbytes);
	}

	close(fd);
}

/*
 * Cd - built-in change-directory command.
 */
cd(argc, argv)
char *argv[];
{

	if (argc == 1)
		argv[1] = "/";
	else if (argc != 2){
		printf("cd: too many args\n");
		return;
	}

	if (chdir(argv[1]))
		printf("cd: invalid directory\n");

	return;
}

/*
 * Built in version of 'cp' command.
 */
cp(argc, argv)
char *argv[];
{
	char buffer[512];
	int f_in, f_out, nbytes;

	if ((f_in = open(argv[1], 0)) == -1){
		printf("cp: cannot read %s\n", argv[1]);
		return;
	}

	if ((f_out = creat(argv[2], 0644)) == -1){
		printf("cp: cannot create %s\n", argv[2]);
		close(f_in);
		return;
	}

	while (nbytes = read(f_in, buffer, 512)){
		if (nbytes != write(f_out, buffer, nbytes)){
			printf("cp: write error\n");
			break;
		}
	}

	close(f_in);
	close(f_out);
	return;
}

/*
 * built-in version of 'rm'.
 */
rm(argc, argv)
char *argv[];
{

	if (argc != 2)
		printf("rm: arg count\n");
	else if (unlink(argv[1]) == -1)
		printf("rm: %s not removed\n", argv[1]);

	return;
}

/*
 * Built-in version of 'mv' - renames a file.
 */
mv(argc, argv)
char *argv[];
{

	if (argc != 3)
		printf("mv: arg count\n");
	else if (link(argv[1], argv[2]))
		printf("mv: cannot link to %s\n", argv[2]);
	else if (unlink(argv[1]))
		printf("mv: cannot unlink %s\n", argv[1]);

	return;
}

panic(mess)
char *mess;
{

	printf("%s\n", mess);
	exit(-1);
}

# ifndef omu
xeqt(name, argc, argv)
char *name, *argv[];
{
	int	ret;

	if (fork() == 0) {
		/* child process */
		execv(name, argv);
		exit(-1);
	}

	wait(&ret);
	return ret >> 8;
}
