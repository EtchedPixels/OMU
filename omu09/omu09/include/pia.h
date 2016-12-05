/*
 * Definitions for the 6522 VIA
 */

struct pia {
	char    orb;
	char    ora;
	char    ddrb;
	char    ddra;
	char    t1c_l;
	char    t1c_h;
	char    t1l_l;
	char    t1l_h;
	char    t2c_l;
	char    t2c_h;
	char    sr;
	char    acr;
	char    pcr;
	char    ifr;
	char    ier;
	char    ora_nshk;
};

# define        CA1     0x02

# define        PIA     (( struct pia * ) 0xEE80 )
# define        ira     ora
# define        irb     orb
