typedef struct {
	int     hi;
	int     lo;
} Long;

typedef Long            daddr_t;        /* disk address */
typedef char *          caddr_t;        /* core address */
typedef unsigned int    ino_t;          /* i-node number */
typedef long            time_t;         /* a time */
typedef int             label_t[6];     /* program status */
typedef int             dev_t;          /* device code */
typedef Long            off_t;          /* offset in file */

/* selectors and constructor for device code */
# define        major(x)        ( int )(( unsigned )(x) >> 8)
# define        minor(x)        ( int )((x) & 0xFF)

# define        makedev(x,y)    (dev_t)((x) << 8 | (y))
