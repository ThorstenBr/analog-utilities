#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "v2types.h"
#include "menu.h"

void main (void) {
    char filename[32];

    strcpy(filename, NEXTNAME);

    if(get_ostype() >= 0x30) {
        strcat(filename, ".ENH");
    } else {
        strcat(filename, ".BASE");
    }

    exec(filename, "");

    backdrop("Loader");
    window(" Error ", longmin(longmax(26, strlen(filename)+4), 38), 6, 1);
    gotoy(11); gotox(13);
    cputs("Unable to load file");
    gotoy(12); gotox(longmin(3, (20-strlen(filename))));
    printlimited(filename, 34);
}
