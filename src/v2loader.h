#include <zlib.h>

void do_jump(uint16_t addr) {
    void_jump jump = (void_jump)addr;
    jump();
}

void load_and_jump(char *filename, uint16_t execaddr) {
    FILE *f;
    size_t bytesread;
    long fsz;
    long total = 0;
    char *ptr, *loadaddr;
    f = fopen(filename, "rb");

    if(f == NULL) {
        return;
    }

    fseek(f, 0, SEEK_END);
    fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    ptr = loadaddr = (char *)execaddr;

    while(!feof(f) && (fsz > 0)) {
        bytesread = fread(ptr, 1, longmin(512, fsz), f);
        if(bytesread > 0) {
            ptr += bytesread;
            fsz -= bytesread;
        }
    };
    fclose(f);

    do_jump(execaddr);
}
