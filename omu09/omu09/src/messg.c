/*
 * Message output etc for One-Man-Unix. Includes a cut-down printf
 * to avoid the size overhead of the real one.
 */

/*
 * Panic - print message and quit cleanly.
 */
panic(messg)
char *messg;
{

	printf("%s\n", messg);
	sync();
	exit(0);
}

/*
 * Printf: Standard formatted output routine.
 * Format specification: %f:
 *      f       one of the formats given below.
 *
 * Formats availiable:
 *      s       null terminated string.
 *      x       hex number.
 *      d       decimal number.
 */

/*VARARGS1*/
printf(fmt, args)
char *fmt;
int args;
{
	int *ap;
	char c;

	ap = &args;
	while (1) {
		while ((c = *fmt++) != '%') {
			if (c == '\0')
				return;

			putchar(c);
		}

		/* format specifier encountered */
		switch (c = *fmt++) {
		case 's':
			prints(*(( char ** ) ap)++);
			break;

		case 'x':
			printx(*(( unsigned * ) ap)++);
			break;

		case 'd':
			printd(*ap++);
			break;

		default:
			putchar(c);
		}
	}
}

printx(var)
unsigned var;
{


	if (var & 0xFFF0)
		printx(var >> 4);

	var &= 0xf;
	putchar( var>9? var-10+'A': var+'0' );
	return;
}

printd(val)
{

	if (val < 0){
		val = -val;
		putchar('-');

		if (val < 0){
			prints("32768");
			return;
		}
	}

	printu(( unsigned ) val);
	return;
}

printu(val)
unsigned val;
{

	if (val > 9)
		printu(val / 10);

	putchar('0'+ (val%10));
	return;
}

prints(str)
char *str;
{
	while (*str) putchar(*str++);
	return;
}
