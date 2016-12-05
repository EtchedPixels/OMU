/*
 * System parameters for OMU
 * Notes:
 * NFPERU can be > NFILES because usually several files are 'DUP'-ed
 *		together in a process.
 */

# define        NFILES          7       /* total no. distinct files */
# define        NFPERU          9       /* no. files per user process */

# define        NPROC           4       /* total no. processes */
# define        NINODES         9       /* no. in core inode copies */
# define        NBUF            8       /* no. in core block buffers */
