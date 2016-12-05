/*
 * One Man Unix system configuration file.
 */

# include       "include/dev.h"

int     fdopen(), fdstrat(), fdclose();
int     bdstrat();

/* Block Device Switch */
struct dev bdevsw[] = {
	fdopen,   fdclose,  fdstrat,      0,      0,/* 0 = fd (codata) */
};

int o_6551(), c_6551(), i_6551(), r_6551(), w_6551();
int r_null(), w_null();

/* Character Device Switch */
struct dev cdevsw[] = {
	o_6551,    c_6551,   i_6551, r_6551, w_6551, /* 0 = 6551 acia   */
	     0,         0,        0, r_null, w_null, /* 1 = null device */
};

int int_6551();

/* List of irq service routines */
int (*irqlist[])() = {
	int_6551,	0
};
