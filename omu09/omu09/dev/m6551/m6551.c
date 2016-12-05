/*
 * 6551 handler
 * Features:
 *
 *      '^D' key:               eof (read returns zero).
 *      'DEL' key (0x7F):       rubs out last char.
 *      '^U' key:               rubs out whole line.
 *      '\n' or '\r':           terminates input line early.
 *
 */

# include       "m6551.h"
# include	<sgtty.h>
# define        CNTL(x)         (x & 0x1F)

/* addresses of ACIA bases */
struct acia *bases[] = {
	0xEA00,
	0xEC00
};

o_6551(dev)
{
	struct acia *acia;
	struct ttystruct *tt;

	if (dev < NTERM){
		if ((tt = &tty[dev])->nopens <= 0) {
			/* valid device, unopened */
			acia = bases[dev];

# ifdef SETUP
			acia->status = 0;       /* programmed reset */
			acia->command = NO_TX_INT | NO_RX_INT | LET_RX;
			acia->control = INT_CLK | SPEED_9600;
# endif

			tt->nopens = 1;
			tt->c_left = 0;
			tt->col = 0;
		}
		else
			tt->nopens++;

		return 0;
	}

	/* invalid minor device */
	return -1;
}

c_6551(dev)
{

	if (dev < NTERM && tty[dev].nopens > 0)
		tty[dev].nopens--;

	return 0;
}

r_6551(dev, buffer, nbytes)
char *buffer;
{
	int ret;
	struct ttystruct *tt;

	tt = &tty[dev];
	if (tt->c_left == 0)
		readahead(dev);

	if (nbytes > tt->c_left)
		nbytes = tt->c_left;

	for (ret = nbytes; ret; ret--) {
		tt->c_left--;
		*buffer++ = *(tt->c_ptr)++;
	}

	return nbytes;
}

/*
 * Readahead - fill tty buffer until newline or buffer full.
 */
readahead(dev)
{
	int count, nch, done;
	char ch, *buffer, *cptr, top;
	struct ttystruct *tt;

	buffer = (tt = &tty[dev])->buf;
	done = 0;
	top = count = 0;

	while (!done){
		top ^= 0x80;

		switch (ch = getch(dev)) {
		case 0x7F:
			/* rubout - not before start of line */
			if (count){
				nch = rubout(dev, buffer, count);
				count -= nch;
				buffer -= nch;
				tt->col -= nch;
			}

			break;

		case CNTL('U'):
			while (count){
				nch = rubout(dev, buffer, count);
				count -= nch;
				buffer -= nch;
				tt->col -= nch;
			}

			break;

		default:
			/* output a char N times into buffer */
			if (count < TTYBUFSIZE-10){
				count += (nch = putch(dev, ch));
				for ( ;nch; nch--)
					*buffer++ = ch | top;
			}

			break;

		case '\n':
		case '\r':
			putch(dev, *buffer++ = ('\n' | top));
			count++;
			/* fall thru' */

		case CNTL('D'):
			done = 1;
		}
	}

	tt->c_ptr = tt->buf;

	/* compress buffer's expanded characters - if anything typed */
	buffer = cptr = tt->buf;
	while (count){
		tt->c_left++;
		if ((*buffer++ = (ch = *cptr++) & 0x7f) == '\n')
			break;

		while ((*cptr & 0x80) == (ch & 0x80))
			cptr++;
	}

	return;
}

/*
 * Rubout - rubs out chars in buffer whose top bits are the same.
 *              will rub out no more than 'count' chars.
 *              Returns actual no. rubbed out.
 */
rubout(dev, buff, count)
char *buff;
{
	char ch;
	int cnt;

	ch = *--buff;
	cnt = 0;
	while (count-- && (*buff & 0x80) == (ch & 0x80)){
		_pchar(dev, '\b');
		_pchar(dev, ' ');
		_pchar(dev, '\b');
		buff--;
		cnt++;
	}

	return cnt;
}

w_6551(dev, buffer, nbytes)
char *buffer;
{
	int nput;

	nput = nbytes;
	while (nbytes--)
		putch(dev, *buffer++);

	return nput;
}

/*
 * Kernel routines need putchar + getchar to console.
 */
char
getchar()
{

	return getch(0);
}

char
putchar(ch)
char ch;
{

	return putch(0, ch);
}

putch(dev, ch)
char ch;
{
	int cnt, here, ret;

	ret = 0;

	switch (ch & 0x7f) {
	case '\t':
		/* skip to next col modulo 8 */
		here = tty[dev].col;
		ret = ((here+8) & 0xFFF8) - here;

		for (cnt = ret; cnt; cnt--)
			_pchar(dev, ' ');

		break;

	case '\n':
		/* implement CRMOD type of conversion */
		ret = 2;
		_pchar(dev, '\r');
		_pchar(dev, '\n');
		tty[dev].col = 0;
		return ret;

	case '\r':
		/* newline resets character position */
		ret = 1;
		_pchar(dev, '\r');
		tty[dev].col = 0;
		return ret;

	default:
		/* non-trapped control codes are made visible */
		if (ch < ' ') {
			/* show as ^ followed by a letter */
			_pchar(dev, '^');
			_pchar(dev, ch + 'A' - 1);
			ret = 2;
		}
		else {
			_pchar(dev, ch);
			ret = 1;
		}
	}

	tty[dev].col += ret;
	return ret;
}

/*
 * _pchar - fundamental print-a-char routine for 6551.
 */
_pchar(dev, ch)
char ch;
{
	struct acia *acia;

	/* first wait until acia is quiet */
	acia = bases[dev];
	while (! (acia->status & TX_QUIET));

	/* transmitter is clear - has ^S been seen though? */
	if (acia->status & RX_FULL){
		if (! (acia->status & ERROR_BITS)) {
			if ((acia->data & 0x7f) == CNTL('S'))
				/* must now wait for ^Q */
				while (getch(dev) != CNTL('Q'));
		}
		else
			/* just clear error character - assume not ^S */
			dev = acia->data;
	}

	/* OK to put the character now */
	acia->data = ch;
	return;
}

getch(dev)
{
	struct acia *acia;
	char ch;

	/* first wait until acia has data for reading */
	acia = bases[dev];
	while (1)
		if (acia->status & RX_FULL){
			if (! (acia->status & ERROR_BITS))
				return (acia->data & 0x7f);
			else
				ch = acia->data;
		}
}

/*
 * I_6551 - implement ioctl for this tty. (Fake for now).
 */
i_6551(dev, request, argp)
struct sgttyb *argp;
{

	switch (request) {
	case TIOCGETP:
		/* old gtty call in effect */
		argp->sg_ispeed = argp->sg_ospeed = B9600;
		argp->sg_erase = 0x7F;
		argp->sg_kill = CNTL('U');
		argp->sg_flags = SCOPE | XTABS | ECHO | CRMOD |EVENP | ODDP;
		break;

	default:
		/* illegal */
		return -1;
	}

	/* successful.. */
	return 0;
}
