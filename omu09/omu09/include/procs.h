/*
 * Structure of a user process table entry.
 */

struct u_regs {
	char            cc;
	int             d;
	char            dp;
	int             x;
	int             y;
	int             u;
	int             pc;
	int             s;
	struct inode    *wd;
	unsigned        ebss;
	struct uft      *slots[NFPERU];
	int             pid;
};

extern struct u_regs *cur_proc;
extern struct u_regs proc_table[];

/*
 * Bits in condition code register
 */
# define        ENTIRE          0x80
