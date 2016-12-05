/*
 * Start-up and Syscall Interface for 6809 Unix, S.Hosgood, 27.Mar.84
 * Note:
 *      There is a routine in the kernel for each of the system calls.
 *      These internal routines replace the library versions as far as
 *      the kernel code is concerned. The library calls invoke the
 *      syscall interface, which of course is not needed from within
 *      the kernel. There is one exception: the 'xeqt' call is called
 *      via the syscall even within the kernel, this is because the
 *      registers must be saved and stacked, the syscall is the easiest
 *      method. The internal routine for 'xeqt' is called 'xeq', and is
 *      declared static to avoid mistaken calls from within the kernel.
 */

# include               "include/a.out.h"
# include               "include/inode.h"
# include               "include/param.h"
# include               "include/procs.h"

# define        TSTART          0x4800
# define        ETEXT           0x5865
# define        EDATA           0x5a96
# define        EBSS            0x5a98

# define                HZ              20      /* clock rate */

struct u_regs proc_table[NPROC];
struct u_regs *cur_proc;
int proc_index, pid;

/* Union to split longs into two 16 bit ints */
union {
	long    All;
	struct {
		int     Hi;
		int     Lo;
	} S;
} Time, Tmp;

long    lseek();

# define        clock           (Time.All)
# define        clock_hi        (Time.S.Hi)
# define        clock_lo        (Time.S.Lo)
# define        ltmp            (Tmp.All)
# define        ltmp_hi         (Tmp.S.Hi)
# define        ltmp_lo         (Tmp.S.Lo)

main()
{
	char *argv[3];

	/* set process table at slot 0 */
	proc_index = 0;
	cur_proc = proc_table;

	/* mount disk 0 */
	if (fdopen(0) == -1)
		panic("No disk");

	/* set working directory */
	chdir("/");

	mask_irq();

# ifdef PIA
	/* set pia clock ready to go */
	reset_pia();
# endif

	set_time();

	/* call 'shell' */
	argv[0] = "sh";
	argv[1] = "-";
	argv[2] = 0;

	printf("Loading Shell..\n");
	if (xeqt("/bin/sh", 2, argv) == -1) {
		/* if that fails, try hot-wiring */
		printf("No Shell, Hot-wiring at $%x..\n", TSTART);
		xeqt("", 2, argv);
	}

	/* back to monitor if 'shell' returns */
	printf("Shell returned!\n");
	fdclose(0);
	return;
}

/*
 * Sys - called by machine code: current process info has been put into
 *              proc_table.
 */
sys(id)
{
	char name1[50], name2[50];
	int t;

	if (id != 5 && id != 9 && id != 10 && id != 17 && id != 22) {
		/* some use 'x' for other than name pointers */
		if (cur_proc->x)
			strncpy(name1, ( char * ) cur_proc->x, 50);
	}

	/* service each call seperately */
	switch (id) {
	case 0:
		/* Exit call */
		/* remove old process's current directory */
		freeiptr(user_curdir);

		/* close all his files */
		for (t = 0; t < NFPERU; t++)
			close(t);

		/* activate previous process */
		proc_index--;
		cur_proc--;
		cur_proc->d = 0;
		user_curdir = cur_proc->wd;
		break;

	case 1:
		/* Execute new process */
		cur_proc->d = xeq(name1, cur_proc->d, ( char ** ) cur_proc->y);
		break;

	case 2:
		/* Chmod call */
		cur_proc->d = chmod(name1, cur_proc->d);
		break;

	case 3:
		/* Link call */
		strncpy(name2, ( char * ) cur_proc->y, 50);
		cur_proc->d = link(name1, name2);
		break;

	case 4:
		/* Unlink call */
		cur_proc->d = unlink(name1);
		break;

	case 5:
		/* Ioctl call */
		cur_proc->d = ioctl(cur_proc->x, cur_proc->d, ( char * ) cur_proc->y);
		break;

	case 7:
		/* Mknod call */
		cur_proc->d = mknod(name1, cur_proc->d, cur_proc->y);
		break;

	case 8:
		/* Open call */
		cur_proc->d = open(name1, cur_proc->d);
		break;

	case 9:
		/* Read call */
		cur_proc->d = read(cur_proc->x, ( char * ) cur_proc->y, cur_proc->d);
		break;

	case 10:
		/* Write call */
		cur_proc->d = write(cur_proc->x, ( char * ) cur_proc->y, cur_proc->d);
		break;

	case 11:
		/* Close call */
		cur_proc->d = close(cur_proc->d);
		break;

	case 12:
		/* Dup call */
		cur_proc->d = dup(cur_proc->d);
		break;

	case 13:
		/* Stat call */
		cur_proc->d = stat(name1, ( struct stat * ) cur_proc->y);
		break;

	case 14:
		/* Fstat call */
		cur_proc->d = fstat(cur_proc->d, ( struct stat * ) cur_proc->y);
		break;

	case 15:
		/* Chdir call */
		cur_proc->d = chdir(name1);
		break;

	case 16:
		/* Sbrk call */
		cur_proc->d = sbrk(( unsigned ) cur_proc->d);
		break;

	case 17:
		/* Stime call */
		ltmp_hi = cur_proc->d;
		ltmp_lo = cur_proc->x;
		setime(ltmp);
		cur_proc->d = 0;
		break;

	case 18:
		/* Time call */
		mask_irq();
		cur_proc->d = clock_hi;
		cur_proc->x = clock_lo;
		let_irq();
		break;

	case 19:
		/* Creat call */
		cur_proc->d = creat(name1, cur_proc->d);
		break;

	case 20:
		/* Getpid call */
		cur_proc->d = cur_proc->pid;
		break;

	case 21:
		/* Sync call */
		sync();
		cur_proc->d = 0;
		break;

	case 22:
		/* Lseek call */
		ltmp_hi = cur_proc->y;
		ltmp_lo = cur_proc->u;
		ltmp = lseek(cur_proc->x, ltmp, cur_proc->d);
		cur_proc->d = ltmp_hi;
		cur_proc->x = ltmp_lo;
		break;

	default:
		/* illegal system call */
		cur_proc->d = -1;
	}

	return;
}

/*
 * Xeq - attempt to load and execute a command.
 *              returns 0 on success, -1 on fail.
 *              Do not call this routine without using the syscall interface.
 */
static
xeq(name, argc, argv)
char *name, *argv[];
{
	int fd, ret;
	unsigned length, top;
	struct exec filhdr;

	if (*name) {
		/* name given - find it and run it */
		if ((fd = open(name, 0)) == -1)
			/* file not found */
			return -1;

		if (read(fd, ( char * ) &filhdr, sizeof filhdr) != sizeof filhdr) {
			/* read error */
			close(fd);
			return -1;
		}

		if (filhdr.a_magic != SQUASH) {
			/* format wrong */
			close(fd);
			return -1;
		}

		length = filhdr.a_text + filhdr.a_data;
		if (read(fd, ( char * ) filhdr.a_toffset, ( int ) length) != length) {
			/* read error */
			close(fd);
			return -1;
		}

		close(fd);

		top = clr_bss(filhdr.a_boffset, filhdr.a_bss);
		ret = new_proc(argc, argv, ( int ) filhdr.a_toffset, top);
	}
	else {
		/* hot-wire shell type process */
		top = clr_bss(EDATA, EBSS - EDATA);
		ret = new_proc(argc, argv, TSTART, top);
	}

	return ret;
}

clr_bss(start, length)
{
	char *cptr;

	/* set BSS to zeroes */
	for (cptr = start; length; length--)
		*cptr++ = 0;

	return ( int ) cptr;
}

new_proc(argc, argv, entry, top)
char *argv[];
unsigned top;
{
	int *sp;
	struct u_regs *parent;

	/* note that 'top' is the addr of the first free byte after BSS */
	sync();

	/* attempt to make process slot for new process */
	if (++proc_index < NPROC){
		/* hopefully 40 bytes between this stack and new stack is enough */
		sp = cur_proc->s - 20;
		*--sp = argv;
		*--sp = argc;

		/* keep note of parent's working directory */
		cur_proc->wd = relock(user_curdir);

		/* creat a process table entry for this new process */
		parent = cur_proc++;

		cur_proc->pid = ++pid;
		cur_proc->s = sp;
		cur_proc->pc = entry;
		cur_proc->cc = parent->cc | ENTIRE;
		cur_proc->ebss = top;

		/* make 'dups' of parent's open files */
		dup_all(parent, cur_proc);
		return 0;
	}

	/* failed - no slot */
	return -1;
}

/*
 * Sbrk - reset process 'break' location.
 */
sbrk(incr)
unsigned incr;
{
	unsigned oldbrk;

	if ((cur_proc->s - cur_proc->ebss) <= (incr + 40))
		/* gap is less than (space reqd + 40) so error */
		oldbrk = 0xFFFF;
	else {
		oldbrk = cur_proc->ebss;
		cur_proc->ebss += incr;
	}

	return oldbrk;
}

/*
 * Time - returns current time.
 */
long
time(ptr)
long *ptr;
{
	long time;

	mask_irq();
	time = clock;
	let_irq();

	if (ptr)
		*ptr = time;

	return time;
}

/*
 * Setime - sets time (also called from set_time in super.c).
 */
setime(tim)
long tim;
{

	mask_irq();
	clock = tim;
	let_irq();
	return;
}
