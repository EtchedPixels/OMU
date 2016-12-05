/*
 * Decoder for 6809 instructions - part of 6809 disassembler.
 *
 * Version 1    Written as part of interactive disassembler, 1982 S.Hosgood.
 * Version 2    Also used by Clever Disassembler, 21.11.83
 */


# include       "decode.h"

char *instr;
struct ea ea;

char *regs[] = {
	"d","x","y","u","s","pc",0,0,"a","b","cc","dp",0,0,0,0
};

char *group0[] = {
	" neg"," ?"," ?"," com"," lsr"," ?"," ror"," asr",
	" asl"," rol"," dec"," ?"," inc"," tst"," jmp"," clr"
};

char *group1[] = {
	" ?"," ?","4nop","4sync"," ?"," ?","7lbra","7lbsr",
	" ?","4 daa","9orcc"," ?","9andcc","4sex","8exg","8tfr"
};

char *group2[] = {
	"5bra","5brn","5bhi","5bls","5bhs","5blo","5bne","5beq",
	"5bvc","5bvs","5bpl","5bmi","5bge","5blt","5bgt","5ble"
};

char *group3[] = {
	"2leax","2leay","2leas","2leau","6pshs","6puls","6pshu","6pulu",
	" ?",   "4rts", "4abx", "4rti", "9cwai","4mul", " ?",   "4swi"
};

char *group8[] = {
	" suba"," cmpa"," sbca"," subd"," anda"," bita","  lda","  sta",
	" eora"," adca","  ora", " adda"," cmpx"," jsr","  ldx","  stx"
};

/* hold the bytes in the instruction for printing. */
int bytec, bytev[6], *bytep;


/*
 * This disassembles the code. On entry, 'pc' points to first byte
 * of instruction, at exit it points to next instruction's first
 * byte. Parameter on entry should be 0, indicates page 0 of opcodes.
 */

decode(page)
int page;
{
	int inst, code, bit7, flowbreak, tmp;
	static char mnem[10];

	/* first level, reset byte storage */
	if (page == 0) {
		bytep = bytev;
		bytec = 0;
	}

	code = getb();
	ea.mode = (code & 0xf0) >> 4;
	inst = code & 0xf;
	ea.index = 0;

	switch (ea.mode){
	case 0:
	case 4:
	case 5:
	case 6:
	case 7:
		strcpy(mnem, group0[inst]);
		if (ea.mode == 4){
			mnem[4] = 'a';
			ea.mode = INHERENT;
		}

		if (ea.mode == 5){
			mnem[4] = 'b';
			ea.mode = INHERENT;
		}

		if (inst == 0xE){
			/* JMP instructions */
			flowbreak = 1;
		}

		if (ea.mode == 0)
			ea.mode = DIRECT;

		if (code == 0x4E || code == 0x5E)
			/* illegal to have jmpa, jmpb */
			mnem[1] = '?';

		if (mnem[1] == '?'){
			mnem[2] = '\0';
			ea.mode = INHERENT;
		}

		if (ea.mode > 5) ea.mode &= 3;
		break;

	case 1:
		switch (inst){
		case 0:
		case 1:
			/* if in page one, change page and decode */
			if (page == 0)
				return decode(inst+1);
			break;

		case 0x06:
			/* long branch */
			flowbreak = 1;
			break;

		case 0x07:
			/* long branch to subroutine */
			break;

		case 0x0E:
		case 0x0F:
			/* tfr or exg could cause flow break */
			/* check postbyte and hang onto it in TMP */

			if (((tmp = getb()) & 0x0F) == 0x05)
				/* destination is 'pc' */
				flowbreak = 1;

			if (inst == 0x0E && (tmp & 0xF0) == 0x50)
				/* source of 'exg' is 'pc' */
				flowbreak = 1;
		}

		strcpy(mnem, group1[inst]);
		ea.mode = mnem[0]-'0';
		break;

	case 2:
		if (inst == 0)
			flowbreak = 1;

		strcpy(mnem, group2[inst]);
		if (page == 1){
			strcpy(mnem+1, group2[inst]);
			mnem[0] = '7';
			mnem[1] = 'l';
		}

		ea.mode = mnem[0]-'0';
		break;

	case 3:
		strcpy(mnem, group3[inst]);
		switch (inst){
		case 0x05:
		case 0x07:
			/* puls or pulu could cause flow break */
			if (code & 0x80)
				flowbreak = 1;
			break;

		case 0x09:
		case 0x0B:
			/* rts or rti */
			flowbreak = 1;
			break;

		case 0x0F:
			/* swi, swi2 or swi3 depending on page no. */
			if (page == 1)
				strcpy(mnem, "4swi2");
			else if (page == 2)
				strcpy(mnem, "4swi3");
		}

		ea.mode = mnem[0]-'0';
		/* temporary fiddle to give SWI operand byte */
		if (inst == 0xF)
			ea.mode = DIRECT;

		break;

	default:
		strcpy(mnem, group8[inst]);

		if (ea.mode & 0x04){
			/* instructions 'Cx', 'Dx', 'Ex' and 'Fx' use accb */
			/* ..or are wierd */
			switch (inst){
			case 3:
				strcpy(mnem, " addd");
				break;

			case 0xC:
				strcpy(mnem, "  ldd");
				break;

			case 0xD:
				strcpy(mnem, "  std");
				break;

			case 0xE:
				strcpy(mnem, "  ldu");
				if (page == 1)
					strcpy(mnem, "  lds");

				break;

			case 0xF:
				strcpy(mnem, "  stu");
				if (page == 1)
					strcpy(mnem, "  sts");
				break;

			default:
				/* just use accb, instead of 'a' */
				mnem[4] = 'b';
			}
		}
		else {
			/* usually, instructions are given in table */
			/* just deal with few exceptions */
			switch (inst){
			case 0x03:
				switch (page){
				case 1:
					strcpy(mnem, " cmpd");
					break;

				case 2:
					strcpy(mnem, " cmpu");
				}
				break;

			case 0x0C:
				switch (page){
				case 1:
					strcpy(mnem, " cmpy");
					break;

				case 2:
					strcpy(mnem, " cmps");
				}
				break;

			case 0x0D:
				/* note subroutine calls */
				break;

			case 0x0E:
				if (page == 1)
					strcpy(mnem, "  ldy");
				break;

			case 0x0F:
				if (page == 1)
					strcpy(mnem, "  sty");
			}
		}

		/* notch out illegals */
		if ((ea.mode & 0x03) == 0){
			if ((inst & 7) == 7) strcpy(mnem, "4?");
			if (inst == 0xD)
				if (ea.mode & 4)
					strcpy(mnem, "4?");
				else
					strcpy(mnem, "5bsr");
		}

		if (mnem[0] == ' ')
			ea.mode &= 0x03;
		else
			ea.mode = mnem[0]-'0';
	}

	/* now for the addressing modes */
	instr = mnem+1;
	while (*instr && *instr == ' ')
		instr++;

	ea.disp = 0;
	switch (ea.mode){
	case IMM_W:
		/*
		 * Immediate mode is 2 post bytes, unless acc a or b
		 * are in use...
		 * check this possibility using char 4 of mnemonic.
		 */
		if (mnem[4] != 'a' && mnem[4] != 'b')
			ea.disp = getw();
		else {
			ea.disp = getb();
			ea.mode = IMM_B;
		}
		break;

	case IMM_B:
		ea.disp = getb();
		break;

	case EXTENDED:
		ea.disp = getw();
		break;

	case REGISTERS:
		ea.disp = getb();
		break;

	case XFR_REGS:
		/* this has been got already */
		ea.disp = tmp;
		break;

	case L_REL:
		ea.disp = getw();
		ea.disp += cur_pc();
		break;

	case RELATIVE:
		ea.disp = getb();
		ea.disp += cur_pc();
		break;

	case INDEXED:
		ea.reg = getb();
		ea.disp = ea.reg & 0xf;
		ea.index = ea.reg & 0x10;
		bit7 = ea.reg & 0x80;
		ea.reg = ((ea.reg & 0x60) >> 5) + 1;
		if (bit7){
			switch (ea.disp){
			case 0:
				ea.mode = INCBYONE;
				break;

			case 1:
				ea.mode = INCBYTWO;
				break;

			case 2:
				ea.mode = DECBYONE;
				break;

			case 3:
				ea.mode = DECBYTWO;
				break;

			case 4:
				ea.disp = 0;
				ea.mode = CONST_OFF;
				break;

			case 5:
				ea.mode = BREG_OFF;
				break;

			case 6:
				ea.mode = AREG_OFF;
				break;

			case 7:
			case 0xA:
			case 0xE:
				ea.mode = ILLEGAL;
				break;

			case 8:
				ea.disp = getb();
				ea.mode = CONST_OFF;
				break;

			case 9:
				ea.disp = getw();
				ea.mode = CONST_OFF;
				break;

			case 0xB:
				ea.mode = DREG_OFF;
				break;

			case 0xC:
				ea.disp = getb();
				ea.disp += cur_pc();
				ea.mode = PCREL;
				break;

			case 0xD:
				ea.disp = getw();
				ea.disp += cur_pc();
				ea.mode = PCREL;
				break;

			case 0xF:
				ea.disp = getw();
				ea.mode = EXTENDED;
				break;
			}
		}
		else {
			/* five bit constant offset */
			if (ea.index)
				ea.disp |= 0xfff0;
			ea.mode = CONST_OFF;
			ea.index = 0;
		}
		break;

	case DIRECT:
		ea.disp = getb();
	}

	return flowbreak;
}

/*
 * Routine to print a decoded instruction.
 */
pr_inst()
{

	/* instruction itself */
	printf("\t%s", instr);

	/* followed by addressing mode */
	if (ea.mode != INHERENT){
		putchar('\t');

		if (ea.index)
			putchar('[');

		switch (ea.mode){
		case DIRECT:
/*
			printf("$%x", ea.disp);
 */
			printf("<$");	/* indicate direct page */
			prhex2(ea.disp);
			break;

		case IMM_B:
/*
			printf("#$%x", ea.disp & 0x0FF);
 */
			printf("#$");	/* indicate immediate */
			prhex2(ea.disp);
			break;

		case IMM_W:
			printf("#$%x", ea.disp);
			break;

		case EXTENDED:
		case RELATIVE:
		case L_REL:
			/* may be a label */
			poss_lab(ea.disp);
			break;

		case REGISTERS:
			/* push - pull list */
			if ((ea.disp & A) && (ea.disp & B))
				printf("d ");
			else if (ea.disp & A)
				printf("a ");
			else if (ea.disp & B)
				printf("b ");

			if (ea.disp & X)
				printf("x ");

			if (ea.disp & Y)
				printf("y ");

			if (ea.disp & SU)
				if (instr[3] == 'u')
					printf("s ");
				else
					printf("u ");

			if (ea.disp & CC)
				printf("cc ");

			if (ea.disp & DP)
				printf("dp ");

			if (ea.disp & PC)
				printf("pc");
			break;

		case XFR_REGS:
			/* register pair for tfr or exg */
			printf("%s,%s", regs[(ea.disp >> 4) & 0x0F], regs[ea.disp & 0x0F]);
			break;

		case INCBYTWO:
			printf(",%s++", regs[ea.reg]);
			break;

		case INCBYONE:
			printf(",%s+", regs[ea.reg]);
			break;

		case DECBYTWO:
			printf(",--%s", regs[ea.reg]);
			break;

		case DECBYONE:
			printf(",-%s", regs[ea.reg]);
			break;

		case CONST_OFF:
			if (ea.disp)
				printf("%d", ea.disp);
			goto index;

		case AREG_OFF:
			putchar('a');
			goto index;

		case BREG_OFF:
			putchar('b');
			goto index;

		case DREG_OFF:
			putchar('d');

		index:
			printf(",%s", regs[ea.reg]);
			break;

		case PCREL:
			poss_lab(ea.disp);
			printf(",pc");
		}

		if (ea.index)
			putchar(']');
	}

	putchar('\n');
	return;
}

/*
 * Getb - uses get_byte, but stores byte obtained.
 */
getb()
{
	int tmp;

	tmp = get_byte();
	*bytep++ = tmp;
	bytec++;
	return tmp;
}

/*
 * Getw - uses getb to get a word, storing bytes obtained.
 */
getw()
{
	int tmp;

	tmp = getb();
	tmp = (tmp << 8) | (getb() & 0xFF);
	return tmp;
}

/*
 * Pr_bytes - prints bytes used by 'decode' in std width field.
 */
pr_bytes()
{
	int tmp;

	bytep = bytev;
	for (tmp = 0; tmp < 7; tmp++) {
		if (bytec) {
			/* print a byte */
/*
BUG in PDP printf..
			printf("%2.2x ", *bytep++);
 */
			prhex2(*bytep++);
			putchar(' ');
			bytec--;
		}
		else
			/* pad out */
			printf("   ");
	}

	return;
}
