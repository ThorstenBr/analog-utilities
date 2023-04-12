#include <stdio.h>

int main(int argc, char **argv) {
	int i = 0;
	FILE *in, *out;
	unsigned char ch, last;

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

	while(i < 2048) {
		ch = fgetc(in);

                if(ch & 0x80) {
			ch = (ch & 0x7f) ? (ch & 0x7f) : 128;
			while(ch--) {
				fputc(last, out);
				i++;
			}
                } else {
			fputc(ch, out);
			last = ch;
			i++;
		}
	}

	fclose(in);
	fclose(out);

	return 0;
}
