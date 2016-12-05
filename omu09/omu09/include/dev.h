/*
 * Structure of a device handler table entry.
 */

struct dev {
	int             (*openfnc)();
	int             (*closefnc)();
	int             (*stratfnc)();          /* Block devs only */
	int             (*rd_fnc)();            /* Char devs only */
	int             (*wr_fnc)();            /* Char devs only */
};

# define        NULLDEV         (( int (*)()) '\0')
# define        ioctlfnc        stratfnc

/*
 * Mention device switches.
 */

extern struct dev bdevsw[];
extern struct dev cdevsw[];
