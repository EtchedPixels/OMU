/*
 * Definitions for 6809 Dissassembler.
 */

# define        CC      0x01
# define        A       0x02
# define        B       0x04
# define        DP      0x08
# define        X       0x10
# define        Y       0x20
# define        SU      0x40
# define        PC      0x80

# define        ILLEGAL         -1
# define        IMM_W           0
# define        DIRECT          1
# define        INDEXED         2
# define        EXTENDED        3
# define        INHERENT        4
# define        RELATIVE        5
# define        REGISTERS       6
# define        L_REL           7
# define        XFR_REGS        8
# define        IMM_B           9

# define        INCBYONE        10
# define        INCBYTWO        11
# define        DECBYONE        12
# define        DECBYTWO        13
# define        CONST_OFF       14
# define        AREG_OFF        15
# define        BREG_OFF        16
# define        DREG_OFF        17
# define        PCREL           18

/*
 * Standard addressing mode structure.
 */
struct ea {
	int             mode;           /* as defined above */
	int             reg;
	int             index;
	unsigned        disp;           /* displacement or immediate data */
};
