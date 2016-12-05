/*
 * TTY Handler: S.Hosgood Mar.87
 *
 * Version 1:	This file created to allow several different devices to
 *		have tty-ability. Code was specific to 6551 ACIA. 25.mar.87
 */

# define	QLEN		20

struct charq {
	char	cq_buf[QLEN];
	char	*cq_rptr;
	char	*cq_wptr;
};

/*
 * Clr_q: Clears given queue.
 */
clr_q(queue)
struct charq *queue;
{

	queue->cq_rptr = queue->cq_wptr = queue->cq_buf;
	return;
}

/*
 * Push_q: Pushes character onto given queue. Returns no chars left in that
 *		queue. Returns -1 if char not stored due to no space.
 */
push_q(ch, queue)
char ch;
struct charq *queue;
{
	register char	*cptr;
	int		nleft;

	cptr = queue->cq_wptr;
	*cptr++ = ch;

	/* see if off end - loop back if so */
	if (cptr >= queue->cq_buf + QLEN)
		cptr = queue->cq_buf;

	/* only update 'wptr' if there's space */
	if ((nleft = queue->cq_rptr - cptr) != 0) {
		queue->cq_wptr = cptr;

		if (nleft < 0)
			/* correct count if pointers crossed */
			nleft += QLEN;
	}

	return nleft-1;
}

/*
 * Pull_q: pulls a char from given queue and returns it. Returns no. chars
 *		still in queue via 'nl'. If queue was empty, NULL is returned
 *		and -1 returned via 'nl'.
 */
pull_q(nl, queue)
int *nl;
struct charq *queue;
{
	char		ret;
	register char	*cptr;
	int		nleft;

	if (nleft = queue->cq_wptr - (cptr = queue->cq_rptr)) {
		if (nleft < 0)
			nleft += QLEN;

		ret = *cptr++;

		/* check if off end */
		if (cptr >= queue->cq_buf + QLEN)
			queue->cq_rptr = queue->cq_buf;
		else
			queue->cq_rptr = cptr;
	}
	else
		/* no chars in queue */
		/* will return -1 via 'nl' */
		ret = '\0';

	*nl = nleft-1;
	return ret;
}

/*
 * Enq_q: enquires about queue length. Returns 0 is queue is empty.
 */
enq_q(queue)
struct charq *queue;
{
	register int	nleft;

	if (nleft = queue->cq_wptr - queue->cq_rptr) {
		if (nleft < 0)
			nleft += QLEN;
	}

	return nleft;
}
