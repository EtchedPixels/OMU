/*
 * Structure of an in-core copy of an inode.
 */

struct inode {
	struct dev      *i_mdev;
	int             i_minor;
	int             i_ino;
	int             i_mode;         /* encoded file mode */
	int             i_nlink;
	unsigned        i_size;
	int             i_type;         /* STD, B_SPECIAL etc */
	int             i_nlocks;
	unsigned        i_addr[13];
	int             i_uid;
	int             i_gid;
	long            i_atime;        /* last access */
	long            i_mtime;        /* last modification */
	long            i_ctime;        /* create date */
};

# define        NULLIPTR        (( struct inode * ) 0)

/*
 * Structure of an indirect block entry.
 */
struct indir {
	int             ind_fill;
	unsigned        ind_addr;
};

/*
 * Encoding of 'i_mode'.
 */
# ifndef	S_IFMT
# define        S_IFMT          0170000         /* type of file */
# define                S_IFDIR 0040000         /* .. dir */
# define                S_IFCHR 0020000         /* .. char special */
# define                S_IFBLK 0060000         /* .. block special */
# define                S_IFREG 0100000         /* .. regular file */
# endif

/*
 * Bits in 'i_type'.
 */

# define        I_WRITE         0x0100          /* inode was altered */

/*
 * Modes for 'namlock' and 'srchdir':
 */

# define        CREATE          1
# define        SEARCH          2
# define        DELETE          3
# define        MKLINK          4

/*
 * Global variables..
 */

extern struct inode *user_curdir;

/*
 * Some functions don't return integers.
 */

extern struct inode *getiptr();
extern struct inode *namlock();
extern struct inode *relock();
extern struct inode *changeiptr();
extern struct inode *srchdir();
extern struct inode *lockfree();
