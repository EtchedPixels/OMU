/*
 * Base changer routine.
 * Input C-type number syntax, outputs octal, decimal + hex.
 * Sums numbers if several given, mixed type input is OK.
 */

main(argc, argv)
char *argv[];
{
	register int total, val;

	if (argc == 1) {
		printf("Usage: %s <any base number>*\n", argv[0]);
		exit(-1);
	}

	argv++;
	total = 0;
	for (argc--; argc; argc--) {
		val = convert(*argv++);
		printf("0%o\t%u\t0x%x\n", val, val, val);
		total += val;
	}

	if (total != val)
		printf("\nTotal:\n0%o\t%u\t0x%x\n", total, total, total);

	exit (0);
}

convert(str)
char *str;
{

	if (*str == '0') {
		/* octal or hex */
		str++;
		if (*str == 'x' || *str == 'X')
			return htoi(str+1);
		else
			return otoi(str);
	}

	return atoi(str);
}

htoi(str)
char *str;
{
	register unsigned val, hexch;

	val = 0;
	while (*str) {
		if (*str >= '0' && *str <= '9')
			hexch = *str - '0';
		else if (*str >= 'A' && *str <= 'F')
			hexch = *str - 'A' + 10;
		else if (*str >= 'a' && *str <= 'f')
			hexch = *str - 'a' + 10;
		else
			break;

		val = (val << 4) | (hexch & 0x0F);
		str++;
	}

	return val;
}

otoi(str)
char *str;
{
	register unsigned val, octch;

	val = 0;
	while (*str) {
		if (*str >= '0' && *str <= '7')
			octch = *str - '0';
		else
			break;

		val = (val << 3) | (octch & 007);
		str++;
	}

	return val;
}
