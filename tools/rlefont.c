#include <stdio.h>
#include <string.h>

unsigned char buf[2048];

int getrunlength(unsigned char *buf, int index) {
	int rl;
	unsigned char ch = buf[index];

	for(rl = 1; ((index+rl) < 2048) && (rl < 128); rl++) {
		if(buf[index+rl] != ch) break;
	}

	return rl - 1;
}

int main(int argc, char **argv) {
	int i;
	FILE *in, *out;

	if(argc != 3) {
		fprintf(stderr, "Usage:\r\n\t%s <input> <output>\r\n", argv[0]);
		return -1;
	}

	in = fopen(argv[1], "rb");
	if(!in) {
		fprintf(stderr, "Unable to open file '%s' for input.\r\n", argv[1]);
		return -1;
	}

	out = fopen(argv[2], "wb");
	if(!out) {
		fprintf(stderr, "Unable to open file '%s' for output.\r\n", argv[2]);
		fclose(in);
		return -1;
	}

	fread(buf, 1, 16, in);
	fwrite(buf, 1, 16, out);
	fread(buf, 1, 2048, in);

	while(i < 2048) {
		int rl = getrunlength(buf, i);

		fputc((buf[i] & 0x7F), out);

		if(rl != 0) {
			fputc((rl | 0x80), out);
		}

		i += 1 + rl;
	}

	fclose(in);
	fclose(out);

	return 0;
}
