/*
 * Mkdump - makes an executable file of the code whose text is <arg1> to <arg2>
 *              and whose data is from <arg2> to <arg3>, and bss from <arg3>
 *              to <arg4>. All numbers in hex. Filename is <arg5>.
 */
mkdump(argc, argv)
char *argv[];
{
	int fd, nbytes;
	struct hddr filhdr;
	char *byteptr;
	unsigned length;
	unsigned textstart, datastart, bssstart, bssend;

	if (argc != 6){
		printf("Usage: %s <textstart> <datastart> <bssstart> <bssend> <file>\n", argv[0]);
		return;
	}

	textstart = hatoi(argv[1]);
	datastart = hatoi(argv[2]);
	bssstart = hatoi(argv[3]);
	bssend = hatoi(argv[4]);

	if ((fd = creat(argv[5], 0755)) == -1){
		printf("%s: cannot open %s\n", argv[0], argv[5]);
		return;
	}

	filhdr.tstart = textstart;
	filhdr.tsize = datastart - textstart;
	filhdr.dstart = datastart;
	filhdr.dsize = bssstart - datastart;
	filhdr.bstart = bssstart;
	filhdr.bsize = bssend - bssstart;
	filhdr.entry = textstart;
	filhdr.magic = 0427;
	filhdr.flags = 1;

	if (write(fd, &filhdr, sizeof filhdr) != sizeof filhdr){
		printf("%s: write error\n", argv[0]);
		close(fd);
		return;
	}

	/* now write out text and data segments (contiguous). */
	byteptr = textstart;
	length = bssstart-textstart;

	while (length){
		nbytes = (length > 512)? 512: length;

		if (write(fd, byteptr, nbytes) != nbytes){
			printf("%s: write error\n", argv[0]);
			break;
		}
		length -= nbytes;
		byteptr += 512;
	}

	close(fd);
	return;
}

/*
 * Hatoi - convert ascii hex string to int.
 */
hatoi(str)
char *str;
{
	char ch;
	unsigned val;

	val = 0;
	while (ch = *str++){
		if (ch >= '0' && ch <= '9')
			val = (val << 4) | (ch - '0');
		else if (ch >= 'A' && ch <= 'F')
			val = (val << 4) | (ch - 'A' + 10);
		else if (ch >= 'a' && ch <= 'f')
			val = (val << 4) | (ch - 'a' + 10);
		else
			break;
	}

	return val;
}
