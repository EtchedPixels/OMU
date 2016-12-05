/*
 * Version Seven SIRIUS - Steve's Inode Reader / Inode Updater System.
 * Converted to version seven, 4.july.82.
 * Steve Hosgood.
 */

/* Check the following agree with /usr/sys/h/param.h */
# define        NICINOD         100
# define        NICFREE         50

# include       <sys/types.h>
# include       <sys/dir.h>
# include       <sys/ino.h>
# include       <sys/filsys.h>
# include       <sys/stat.h>

# define        SUPERBLOCK      1       /* block number of super-block */

# define DIR    0040000
# define BLK    0020000
# define CHR    0010000
# define SUID   0004000
# define SGID   0002000
# define STXT   0001000
# define ROWN   0000400
# define WOWN   0000200
# define XOWN   0000100
# define RGRP   0000040
# define WGRP   0000020
# define XGRP   0000010
# define ROTH   0000004
# define WOTH   0000002
# define XOTH   0000001

union label {
	char dummy[512];
	struct {
		char Header[30];
		char L_id[30];
		char Mount[30];
		char Desc[30];
		char Perm[11];
		char Owner[30];
	} A;
};

# define        header  A.Header
# define        l_id    A.L_id
# define        mount   A.Mount
# define        desc    A.Desc
# define        perm    A.Perm
# define        owner   A.Owner

int fileid;

struct dinode inode;

char buffer[20];
long ino_posit(), get_daddr_t();
long lseek();

main(argc, argv)
char *argv[];
{
	int nbytes, writeable;

	if (argc != 2){
		printf("Usage: %s <blk-dev name>\n", argv[0]);
		exit(-1);
	}

	writeable = 1;
	if ((fileid = open(argv[1], 2)) == -1) {
		writeable = 0;
		if ((fileid = open(argv[1], 0)) == -1) {
			printf("%s:cannot open %s\n", argv[0], argv[1]);
			exit(-1);
		}
	}

	while (1) {
		printf(">");
		nbytes = read(0, buffer, 10);
		if (nbytes == 0)
			continue;

		switch (buffer[0]){
		case 'b':
			prb(&buffer[1]);
			break;

		case 'd':
			prd(&buffer[1], 0);
			break;

		case 'i':
			pri(&buffer[1], 0);
			break;

		case 'l':
			prl(0);
			break;

		case 'm':
			if (writeable) {
				switch (buffer[1]) {
				case 'i':
					prmi(&buffer[2]);
					break;

				default:
					printf("??\n");
				}
			}
			else
				printf("Can't write\n");

			break;

		case 'q':
			close(fileid);
			exit(0);

		case 's':
			prs(0);
			break;

		case 'u':
			if (writeable) {
				switch (buffer[1]) {
				case 'd':
					prd(&buffer[2], 1);
					break;

				case 'i':
					pri(&buffer[2], 1);
					break;

				case 'l':
					prl(1);
					break;

				case 's':
					prs(1);
					break;

				default:
					printf("??\n");
				}
			}
			else
				printf("Can't write\n");

			break;

		default:
			printf("??\n");
			break;
		}
	}
}

/*
 * Prb - print block just as characters.
 */
prb(string)
char *string;
{

	get_block(atoi(string));
	printf("\n");
	return;
}

/*
 * Prd - print a block as a directory.
 */
prd(string, up_flag)
int up_flag;
char *string;
{
	int count;
	struct direct dir_block;

	lseek(fileid, (( long ) atoi(string)) << 9, 0);
	for (count = 0; count != 512; count += sizeof dir_block) {
		read(fileid, ( char *) &dir_block, sizeof dir_block);
		if ((dir_block.d_ino == 0) && !up_flag)
			continue;

		printf("%s ", dir_block.d_name);
		get_int("i-number", ( int * ) &dir_block.d_ino, up_flag);

		if (up_flag){
			lseek(fileid, -( long )(sizeof dir_block), 1);
			write(fileid, ( char * ) &dir_block, sizeof dir_block);
		}
	}

	return;
}

/*
 * Pri - print an inode given its i-number.
 * Allows update of selected fields, uid, gid, links and number of 1st block.
 */
pri(string, up_flag)
int up_flag;
char *string;
{
	int count, uid, gid, ino, blk;
	long lblk;

	get_inode(ino = atoi(string));
	pmode(( int ) inode.di_mode);
	uid = inode.di_uid;
	gid = inode.di_gid;
	printf("%u %u ", uid, gid);
	printf(" %D", inode.di_size);
	printf("Blocks used :-\n");
	for (count = 0; count != 13; count ++) {
		l3tol(&lblk, &inode.di_addr[count*3], 1);
		if (count == 0)
			blk = lblk;

		printf(":%U", lblk);
	}

	printf("\n");
	count = inode.di_nlink;
	get_int("links", &count, up_flag);
	get_int("uid", &uid, up_flag);
	get_int("gid", &gid, up_flag);
	get_int("blk_0", &blk, up_flag);

	if (up_flag) {
		inode.di_uid = uid;
		inode.di_gid = gid;
		inode.di_nlink = count;

		lblk = blk;
		ltol3(inode.di_addr, &lblk, 1);
		put_inode(ino);
	}

	return;
}

char *label_hd  = "   Unix 7 Disk Label (UCS)";

/*
 * Prl - print label.
 */
prl(up_flag)
int up_flag;
{
	int nbytes;
	char *i, *j;
	union label disc_label;

	lseek(fileid, 0L, 0);
	read(fileid, ( char * ) &disc_label, sizeof disc_label);

	if (up_flag) {
		for (nbytes = 0; nbytes < 512; nbytes++)
			disc_label.dummy[nbytes] = '\0';

		i = disc_label.header;
		j = label_hd;
		while (*i++ = *j++);
	}

	get_str("name", disc_label.l_id, up_flag);
	get_str("mount name", disc_label.mount, up_flag);
	get_str("permissions", disc_label.perm, up_flag);
	get_str("owner", disc_label.owner, up_flag);

	if (up_flag) {
		lseek(fileid, 0L, 0);
		write(fileid, ( char * ) &disc_label, sizeof disc_label);
	}

	return;
}

/*
 * Prmi - move (copy) an inode to elsewhere.
 */
prmi(string)
char *string;
{
	char *ptr;

	for (ptr = string; *ptr; ptr++)
		if (*ptr == '-')
			break;

	if (! (*ptr))
		return;

	get_inode(atoi(string));
	put_inode(atoi(++ptr));
	return;
}

/*
 * Prs - print super-block contents.
 */
prs(up_flag)
int up_flag;
{
	struct filsys super;

	lseek(fileid, ( long )( SUPERBLOCK*512 ), 0);
	read(fileid, ( char * ) &super, sizeof super);

	if (up_flag) {
		printf("no inode blocks? >");
		read(0, buffer, 10);
		super.s_isize = atoi(buffer);
	}

	printf("%u inode blocks used\n", super.s_isize);
	get_daddr_t("size of volume", &super.s_fsize, up_flag);

	if (up_flag) {
		lseek(fileid, ( long )( SUPERBLOCK*512 ), 0);
		write(fileid, ( char * ) &super, sizeof super);
	}

	return;
}

int     m0[] = { 3, DIR, 'd', BLK, 'b', CHR, 'c', '-'};
int     m1[] = { 1, ROWN, 'r', '-' };
int     m2[] = { 1, WOWN, 'w', '-' };
int     m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int     m4[] = { 1, RGRP, 'r', '-' };
int     m5[] = { 1, WGRP, 'w', '-' };
int     m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int     m7[] = { 1, ROTH, 'r', '-' };
int     m8[] = { 1, WOTH, 'w', '-' };
int     m9[] = { 1, XOTH, 'x', '-' };
int     m10[] = { 1, STXT, 't', ' ' };

int     *m[] = { m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10};

/*
 * Pmode - prints mode field of an inode.
 */
pmode(aflag)
int aflag;
{
	register int **mp, decoded;

	if ( aflag == 0 )
		printf("not in use ");
	else {
		decoded = aflag & 07777;

		switch (aflag & S_IFMT) {
		case S_IFDIR:
			decoded |= DIR;
			break;

		case S_IFCHR:
		case S_IFMPC:
			decoded |= CHR;
			break;

		case S_IFBLK:
		case S_IFMPB:
			decoded |= BLK;
			break;
		}

		for (mp = &m[0]; mp < &m[11];)
			xselect(*mp++,decoded);
	}

	return;
}

xselect(pairp, aflag)
int *pairp, aflag;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;

	while (--n >= 0 && (aflag & *ap++) == 0)
		ap++;

	putchar(*ap);
	return;
}

get_int(string, addr, up_flag)
int *addr;
char *string;
{
	int nbytes;
	char buffer[10];

	printf("%s : %u", string, *addr);

	if (up_flag) {
		printf("? >");
		nbytes = read(0, buffer, 10);
		if (nbytes > 1)
			*addr = atoi(buffer);
	}
	else
		printf("\n");

	return;
}

get_str(string, addr, up_flag)
int up_flag;
char *string, *addr;
{
	int nbytes;
	char buffer[30];

	printf("%s : %s", string, addr);
	if (up_flag) {
		printf("? >");
		nbytes = read(0, buffer, 30);
		if (nbytes > 1) {
			buffer[nbytes-1] = '\0';

			for( ;nbytes >= 0; nbytes--)
				addr[nbytes] = buffer[nbytes];
		}
	}
	else
		printf("\n");

	return;
}

get_block(block_no)
{
	int nbytes;
	char buffer[520];

	lseek(fileid, (( long ) block_no) << 9, 0);
	nbytes = read(fileid, buffer, 512);
	write(1, buffer, nbytes);

	return;
}

get_inode(ino)
{

	lseek(fileid, ino_posit(ino), 0);
	read(fileid, ( char * ) &inode, sizeof inode);
	return;
}

put_inode(ino)
int ino;
{

	lseek(fileid, ino_posit(ino), 0);
	write(fileid, ( char *) &inode, sizeof inode);
	return;
}

long
ino_posit(ino)
{

	/* Return offset in bytes, to a given inode. */
	return ( long )((( ino-1 )*(sizeof inode)) + 1024L);
}

long
get_daddr_t(string, addr, up_flag)
char *string;
long *addr;
{
	int nbytes;
	char buffer[10];

	printf("%s : %D", string, *addr);
	if (up_flag) {
		printf("? >");
		nbytes = read(0, buffer, 10);

		if (nbytes > 1)
			*addr = atoi(buffer);
	}
	else
		printf("\n");

	return;
}
