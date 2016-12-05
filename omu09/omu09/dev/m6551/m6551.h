/*
 * Character i/o using 6551 ACIA.
 */

struct acia {
	char    data;
	char    status;
	char    command;
	char    control;
};

/* bits in status register */
# define TX_QUIET       0x10
# define RX_FULL        0x08
# define ERROR_BITS     0x07

/* control register */
# define SPEED_9600     0x0E
# define SPEED_7200     0x0D
# define SPEED_4800     0x0C
# define SPEED_3600     0x0B
# define SPEED_2400     0x0A
# define SPEED_1800     0x09
# define SPEED_1200     0x08
# define SPEED_600      0x07
# define SPEED_300      0x06
# define SPEED_150      0x05
# define SPEED_134.58   0x04
# define SPEED_110      0x03
# define SPEED_75       0x02
# define SPEED_50       0x01
# define SPEED_EXT      0x00

# define INT_CLK        0x10

# define SEVEN_BIT      0x20

# define TWO_STOP       0x80


/* command register */
# define ODD_PARITY     0x20
# define EVEN_PARITY    0x60
# define MARK_PAR       0xA0
# define SPACE_PAR      0xE0

# define ECHOBACK       0x10

# define NO_TX_INT      0x08
# define LET_TX_INT     0x04

# define NO_RX_INT      0x02

# define LET_RX         0x01

/* define TTY struct */
# define        NTERM           2
# define        TTYBUFSIZE      100

struct ttystruct {
	int     col;            /* current column on screen, 1-80 ish */
	int     nopens;
	char    buf[TTYBUFSIZE];
	int     c_left;
	char    *c_ptr;
} tty[NTERM];
