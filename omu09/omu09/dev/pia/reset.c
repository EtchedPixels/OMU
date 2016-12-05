/*
 * Reset_pia - set 6522 for 20 hz clock, etc
 */
reset_pia(){

# ifdef PIA_REQD
	PIA->ier = 0;           /* no interrupts from pia */
	PIA->t1c_l = 0x50;      /* 50mS interrupts */
	PIA->t1c_h = 0xC3;

	/* free-run t1, one shot t2, latching mode ca1 */
	PIA->acr = 0x41;
# ifdef PRINTER
	PIA->ddrb = ~(ACK | BUSY);      /* printer handshake inputs */
	PIA->orb = 0xFF;                /* set all outputs high */
	PIA->ddra = PIA->ora = 0xFF;
# endif
	PIA->ier = 0xC0;        /* enable timer 1 to interrupt */
# endif
	return;
}
