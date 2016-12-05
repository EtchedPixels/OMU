/*
 * Structure of an element of the file table.
 */

struct file {
	int             f_type;
	int             f_dev;
	struct inode    *f_inode;
	int             f_curblk;
	int             f_curchar;
	struct dev      *f_handler;
};

/*
 * Values of 'type' field.
 */

# define        STD     0
# define        B_SPECL 1
# define        C_SPECL 2
