# include	<stdio.h>
# include	<sys/types.h>
# include	<sys/stat.h>

char	*getenv();
short	nlinks;
int	aka();
ino_t	ino;

main(argc, argv)
char *argv[];
{
	struct stat	status;

	if (stat(argv[1], &status) == 0) {
		if ((nlinks = status.st_nlink) > 1) {
			ino = status.st_ino;
			dsearch(getenv("HOME"), aka);
		}
		else
			fprintf(stderr, "%s: %s is unique\n", argv[0], argv[1]);
	}
	else
		fprintf(stderr, "%s: Cannot stat %s\n", argv[0], argv[1]);

	exit(0);
}

aka(name, status)
char *name;
struct stat *status;
{

	if (status->st_ino == ino) {
		printf("%s\n", name);

		if (--nlinks == 0)
			exit(0);
	}

	return;
}
