/*
 * Handler for Codata Format 5.25in Floppy Disks
 * Uses 'seek', 'read' and 'write' routines in machine monitor.
 *
 * S.Hosgood 19.jan.84
 * Version 2:	Rewritten to use machine monitor routines, 10.aug.86
 */

# include       "../../include/buf.h"

# define        B_MAX           1440
# define        B_PER_TRK       9
# define	NREPS		5

int	fd_nopen[4];

/*
 * Fdopen - keeps track of how many 'files' are open on device. Forces
 *	a restore on 1st open.
 */
fdopen(drive)
{

	if (fd_nopen[drive] <= 0) {
		if (seek_fd(drive, 0, 0) < 0) {
			printf("Deverr: drive %d unavailiable\n", drive);
			return -1;
		}

		fd_nopen[drive] = 1;
	}
	else
		fd_nopen[drive]++;

	return 0;
}

fdclose(drive)
{

	fd_nopen[drive]--;
	return 0;
}

fdstrat(buf)
struct buf *buf;
{
	int trk, sector, drive, side, reps;

	/* verify track number and minor device no */
	if (buf->b_bno >= B_MAX) {
		deverr("block no. too large", 1, buf);
		buf->b_flags = ABORT;
		return -1;
	}

	if ((drive = buf->b_dev) >= 4) {
		deverr("minor dev out of range", 1, buf);
		buf->b_flags = ABORT;
		return -1;
	}

	trk = buf->b_bno / B_PER_TRK;
	sector = (buf->b_bno % B_PER_TRK) + 1;
	side = (trk & 0x01)? 1: 0;
	trk >>= 1;

	/* select drive and seek to track reqd */
	for (reps = 0; reps < NREPS; reps++) {
		if (seek_fd(drive, trk, side) >= 0)
			break;
	}

	if (reps) {
		deverr("seek failed", reps, buf);

		/* abort on NREPS-th fail */
		if (reps >= NREPS) {
			buf->b_flags = ABORT;
			return -1;
		}
	}

	/* finally process the block */
	if (buf->b_flags & WRITE)
		reps = dowrite(sector, buf->b_buf);
	else
		reps = doread(sector, buf->b_buf);

	/* errors? */
	if (reps) {
		deverr(buf->b_flags & WRITE? "write": "read", reps, buf);

		if (reps >= NREPS) {
			buf->b_flags |= ABORT;
			return -1;
		}
	}

	buf->b_flags = 0;
	return 0;
}

/*
 * Deverr - general error message
 */
deverr(str, count, buf)
char *str;
struct buf *buf;
{

	printf("Deverr: ");
	if (count >= NREPS)
		printf("Fatal\007 ");
	else if (count > 1)
		printf("%d of ", count);

	printf("%s at blk %d on %d\n", str, buf->b_bno, buf->b_dev);
	return;
}

doread(sector, buf)
char *buf;
{
	int	count;

	for (count = 0; count < NREPS; count++)
		if (n_errs(read_fd(sector, buf)) == 0)
			break;

	return count;
}

dowrite(sector, buf)
char *buf;
{

	int	count;

	for (count = 0; count < NREPS; count++)
		if (n_errs(write_fd(sector, buf)) == 0)
			break;

	return count;
}

n_errs(status)
{
	int	count;

	count = 0;

	if (status & 0x0040) {
		/* write protect */
		printf("WPROT,");
		count++;
	}

	if (status & 0x0010) {
		/* record not found */
		printf("RNF,");
		count++;
	}

	if (status & 0x0008) {
		/* CRC error */
		printf("CRC,");
		count++;
	}

	if (status & 0x0004) {
		/* Lost Data */
		printf("LostData,");
		count++;
	}

	return count;
}
