/*
 * Bdstrat - strategy routine for double density BBC style disks.
 */
bdstrat(buf)
struct buf *buf;
{
	int trk, sector, drive, side, bno;
	struct drive_info *fds;

	/* verify track number and minor device no */
	if (buf->b_bno >= BBC_MAX){
		deverr("block no. too large");
		buf->b_flags = ABORT;
		return -1;
	}

	if ((drive = buf->b_dev) >= 4){
		deverr("minor dev out of range");
		buf->b_flags = ABORT;
		return -1;
	}

	bno = buf->b_bno << 1;
	trk = bno / BBC_PER_TRK;
	sector = bno % BBC_PER_TRK;
	side = SIDE_0;

	/* select drive */
	if (select(drive) == -1){
		deverr("drive not ready");
		buf->b_flags = ABORT;
		return -1;
	}

	/* find current position */
	fds = &fdstatus[drive];
	if ((FDC->track = fds->d_curtrk) != trk){
		if (doseek(trk)) {
			/* seek error */
			buf->b_flags = ABORT;
			return -1;
		}

		fds->d_curtrk = trk;
	}

	/* finally process the block */
	if (buf->b_flags & WRITE){
/*
		write_fd(WRITE_SECT | IBM_SECTORS, sector, side, buf->b_buf);

		while ( !(AUX->latch & WRITING));
 */
		;
	}
	else {
		/* read this and next sector - total 512 bytes */
		doread(drive, sector, side, buf->b_buf);
		doread(drive, sector+1, side, buf->b_buf+256);
	}

	buf->b_flags = 0;
	return 0;
}

deverr(str)
char *str;
{

	printf("DEVERR FDC: %s\n", str);
	return;
}

