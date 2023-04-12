#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	int i;
	FILE *a, *b, *out;
	unsigned char ca, cb;
	char *pathname;
	char *filename;

	if(argc != 5) {
		fprintf(stderr, "Usage\r\n\t%s <basename> <input0> <input1> <output>\r\n", argv[0]);
		return -1;
	}

	a = fopen(argv[2], "rb");
	if(!a) {
		fprintf(stderr, "Unable to open file '%s' for input.\r\n", argv[2]);
		return -1;
	}

	b = fopen(argv[3], "rb");
	if(!b) {
		fprintf(stderr, "Unable to open file '%s' for input.\r\n", argv[3]);
		fclose(a);
		return -1;
	}

	out = fopen(argv[4], "wb");
	if(!out) {
		fprintf(stderr, "Unable to open file '%s' for output.\r\n", argv[4]);
		fclose(a);
		fclose(b);
		return -1;
	}

        for(i=0; i < 16; i++) {
                if(i < strlen(argv[1])) {
			fputc(toupper(argv[1][i]), out);
		} else {
			fputc(0, out);
		}
        }

	for(i=0; i < 2048; i++) {
		ca = fgetc(a);
		cb = fgetc(b);
		fputc((ca ^ cb), out);
	}

	fclose(a);
	fclose(b);
	fclose(out);

	return 0;
}
