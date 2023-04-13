#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "v2types.h"
#include "v2loader.h"

#define PROGNAME "Font Manager"
volatile uint16_t cardslot = 3;

#include "v2analog.h"
#include "menu.h"

#if defined(__APPLE2ENH__)
#define UP_ARROW    0xCB
#define RIGHT_ARROW 0xD5
#define DOWN_ARROW  0xCA
#else
#define UP_ARROW    '^'
#define RIGHT_ARROW ('>' | 0x80)
#define DOWN_ARROW  'v'
#endif

#define FONT_MAX 18

int selected_entry = 0;
int top_entry = 0;
int more_fonts = 0;
int arrow_line = 0;
int total_fonts = 0;
int total_pages = 0;
int line_count = 0;

DIR *fontdir;
uint8_t fontbuffer[2048];
uint8_t filebuffer[2048];
char prefix[FILENAME_MAX];
char pathname[FILENAME_MAX];
char fontname[FILENAME_MAX];

void print_menu_item(char *str, int highlighted) {
    revers(highlighted);
    cputs(str);
    revers(0);
}

void remove_font(uint16_t block) {
    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Removing Font,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    if(cfg_cmd1("fe", block)) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to erase block.");
        return;
    }

    backdrop(PROGNAME);
    message(" Success ", "Block erased.");
}

void upload_font(uint8_t *buffer) {
    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Uploading font,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    if(cfg_cmd1("CT", (uint16_t)buffer)) {
        backdrop(PROGNAME);
        message(" Error ", "Communication Error");
        return;
    }

    backdrop(PROGNAME);
    message(" Success ", "Font uploaded.");
}

int write_font(uint8_t *buffer, uint16_t block) {
    int rv = 1;
    char dummy;
    uint16_t i;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Flashing font,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    dummy = *(volatile char*)0xCFFF;

    for(i = 0; i < 4096; i++) {
        CF_DATA = buffer[i];
    }
    if(cfg_cmd1("fw", block)) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to write block.");
        goto cleanup;
    }

    backdrop(PROGNAME);
    message(" Success ", "Font uploaded.");
    rv = 0;

cleanup:
    dummy = *(volatile char*)0xCFFF;
    return rv;
}

int read_font(uint8_t *buffer, uint16_t block) {
    int rv = 1;
    char dummy;
    uint16_t i;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Retrieving font,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    dummy = *(volatile char*)0xCFFF;

    if(cfg_cmd1("fr", block)) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to read block.");
        goto cleanup;
    }
    for(i = 0; i < 4096; i++) {
        buffer[i] = CF_DATA;
    }

    backdrop(PROGNAME);
    message(" Success ", "Font downloaded.");
    rv = 0;

cleanup:
    dummy = *(volatile char*)0xCFFF;
    return rv;
}


typedef struct font_slot_s {
    char *entry;
    uint16_t block;
} font_slot_t;

font_slot_t font_slot[] = {
    { " 00 US            (Enh) ", 0x00 },
    { " 01 US          (UnEnh) ", 0x01 },
    { " 02 Clinton Turner V1   ", 0x02 },
    { " 03 ReActiveMicro (Enh) ", 0x03 },
    { " 04 Dan Paymar    (Enh) ", 0x04 },
    { " 05 Blippo Black  (Enh) ", 0x05 },
    { " 06 Byte          (Enh) ", 0x06 },
    { " 07 Colossal      (Enh) ", 0x07 },
    { " 08 Count         (Enh) ", 0x08 },
    { " 09 Flow          (Enh) ", 0x09 },
    { " 0A Gothic        (Enh) ", 0x0a },
    { " 0B Outline       (Enh) ", 0x0b },
    { " 0C PigFont       (Enh) ", 0x0c },
    { " 0D Pinocchio     (Enh) ", 0x0d },
    { " 0E Slant         (Enh) ", 0x0e },
    { " 0F Stop          (Enh) ", 0x0f },
    
    { " 10 Euro        (UnEnh) ", 0x10 },
    { " 11 Euro          (Enh) ", 0x11 },
    { " 12 Clinton Turner V2   ", 0x12 },
    { " 13 German        (Enh) ", 0x13 },
    { " 14 German      (UnEnh) ", 0x14 },
    { " 15 French        (Enh) ", 0x15 },
    { " 16 French      (UnEnh) ", 0x16 },
    { " 17 Hebrew        (Enh) ", 0x17 },
    { " 18 Hebrew      (UnEnh) ", 0x18 },
    { " 19 Apple II+     (Enh) ", 0x19 },
    { " 1A Apple II+   (UnEnh) ", 0x1a },
    { " 1B Katakana      (Enh) ", 0x1b },
    { " 1C Cyrillic      (Enh) ", 0x1c },
    { " 1D Greek         (Enh) ", 0x1d },
    { " 1E Esperanto     (Enh) ", 0x1e },
    { " 1F Videx         (Enh) ", 0x1f },

    { " 20 Apple II/II+        ", 0x20 },
    { " 21 Apple IIe           ", 0x21 },
    { " 22 Apple IIgs          ", 0x22 },
    { " 23 Pravetz             ", 0x23 },
    { " 24 Custom              ", 0x24 },
    { " 25 Custom              ", 0x25 },
    { " 26 Custom              ", 0x26 },
    { " 27 Custom              ", 0x27 },
};

uint8_t parsehex8(char *str) {
    uint8_t ch = 0;
    switch(str[0]) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            ch = (str[0] - '0') << 4;
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            ch = (str[0] + 0xa - 'a') << 4;
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            ch = (str[0] + 0xA - 'A') << 4;
            break;
    }
    switch(str[1]) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            ch |= (str[1] - '0');
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            ch |= (str[1] + 0xa - 'a');
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            ch |= (str[1] + 0xA - 'A');
            break;
    }
    return ch;
}

int slot_menu() {
    int paint_menu = 2;
    int selected_slot = 0;
    int top = 0;
    int y, i;
    uint8_t block = parsehex8(fontname);

#if 0
    // Try to find a matching slot from our table
    for(i = 0; i < (sizeof(font_slot)/sizeof(font_slot_t)); i++) {
        if(block == font_slot[i].block) {
            selected_slot = i;
            top = (selected_slot / 15) * 15;
            break;
        }
    }
#endif

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop(PROGNAME);
            window(" Font Slot? ", 24, 17, 0);
            paint_menu = 1;
        }
        if(paint_menu > 0) {
            for(y = 0; y < 15; y++) {
                i = y+top;
                if(i < (sizeof(font_slot)/sizeof(font_slot_t))) {
                    gotoy(5+y); gotox(9);
                    print_menu_item(font_slot[i].entry, (selected_slot == i));
                } else {
                    gotoy(5+y); gotox(9);
                    repeatchar(' ', 22);
                }
            }

            paint_menu = 0;
        }

        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_slot > 0) {
                    selected_slot--;
                    if(selected_slot < top)
                        top = (selected_slot / 15) * 15;
                }
                paint_menu = 1;
                break;
            case 0x0A:
            case 0x15:
                if(selected_slot < ((sizeof(font_slot)/sizeof(font_slot_t))-1)) {
                    selected_slot++;
                    if(selected_slot > top + 14)
                        top = (selected_slot / 15) * 15;
                }
                paint_menu = 1;
                break;
            case 0x1B:
                return -1;
            case 0x0D:
                // Select
                return(selected_slot);
        }
    }
}

int action_menu_action(int action) {
    int selected_slot;
    switch(action) {
        case 0:
            upload_font(fontbuffer);
            return 2;
        case 1:
            selected_slot = slot_menu();
            if(selected_slot >= 0) {
                write_font(fontbuffer, font_slot[selected_slot].block);
            }
            return 2;
    }
    return 1;
}

void action_menu(void) {
    int paint_menu = 2;
    int selected_action = 0;

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop(PROGNAME);
            window(" Action? ", 18, 7, 0);
            paint_menu = 1;
        }
        if(paint_menu > 0) {
            gotoy(11); gotox(13);
            revers(selected_action == 0);
            cputs(" Test         ");

            gotoy(13); gotox(13);
            revers(selected_action == 1);
            cputs(" Install      ");

            revers(0);
            paint_menu = 0;
        }

        switch(cgetc()) {
            case 'T':
            case 't':
                paint_menu = action_menu_action(selected_action = 0);
                break;
            case 'I':
            case 'i':
                paint_menu = action_menu_action(selected_action = 1);
                break;
            case 0x08:
            case 0x0A:
            case 0x0B:
            case 0x15:
                selected_action = !selected_action;
                paint_menu = 1;
                break;
            case 0x1B:
                return;
            case 0x0D:
                // Select
                paint_menu = action_menu_action(selected_action);
                break;
        }
    }
}

int open_fontdir(void) {
    struct dirent *ent;
    char *ext;

    fontdir = opendir ("FONTS");
    if(fontdir == NULL) {
        fontdir = opendir (".");
        prefix[0] = 0;
    } else {
        strcpy(prefix, "FONTS/");
    }

    if(fontdir == NULL)
        return 0;

    total_fonts = 0;

    while (ent = readdir(fontdir)) {
        ext = strrchr (ent->d_name, '.');
        if (!ext || (strcasecmp (ext, ".pf") && strcasecmp (ext, ".cf")))
            continue;
        total_fonts++;
    }

    total_pages = (total_fonts - 1) / FONT_MAX;

    return 1;
}

int load_font(void) {
    struct dirent *ent;
    char *ext;
    int current_entry = 0;
    FILE *f;
    uint8_t ch, last = 0;
    uint16_t i, o, len;

    backdrop(PROGNAME);
    gotoy(12); gotox(14);
    cputs("Loading Font");

    rewinddir(fontdir);
    while (ent = readdir(fontdir)) {
        ext = strrchr (ent->d_name, '.');
        if (!ext || (strcasecmp (ext, ".pf") && strcasecmp (ext, ".cf")))
            continue;

        // Seek to first entry on the screen
        if(current_entry < selected_entry) {
            current_entry++;
            continue;
        }

        strcpy(pathname, prefix);
        strcat(pathname, ent->d_name);
        strcpy(fontname, ent->d_name);
        
        // Pixel Font
        if(!strcasecmp (ext, ".pf")) {
            f = fopen(pathname, "rb");
            if(f == NULL) {
                backdrop(PROGNAME);
                gotoy(4); gotox(2);
                cputs(pathname);
                message(" Error ", "Unable to open font file.");
                return 0;
            }
            fread(fontbuffer, 512, 4, f);
            fclose(f);
            return 1;
        }

        // XOR & RLE Compressed Font
        if(!strcasecmp (ext, ".cf")) {
            f = fopen(pathname, "rb");
            if(f == NULL) {
                backdrop(PROGNAME);
                gotoy(4); gotox(2);
                cputs(pathname);
                message(" Error ", "Unable to open font file.");
                return 0;
            }

            // Get base font name
            strcpy(pathname, prefix);
            fread(pathname + strlen(prefix), 1, 16, f);
            pathname[strlen(prefix)+15] = 0;

            fread(filebuffer, 512, 4, f);
            fclose(f);
            
            // Undo the RLE compression
            i = 0;
            o = 0;
            while(o < 2048) {
                ch = filebuffer[i++];
                if(ch & 0x80) {
                    ch = (ch & 0x7f) ? (ch & 0x7f) : 128;
                    len = longmin(ch, (2048-o));
                    memset(fontbuffer+o, last, len);
                    o += len;
                } else {
                    fontbuffer[o++] = ch;
                    last = ch;
                }
            }

            // If this font is XOR'ed with a base font, undo that now.
            if(strlen(pathname) != strlen(prefix)) {
                f = fopen(pathname, "rb");
                if(f == NULL) {
                    backdrop(PROGNAME);
                    gotoy(4); gotox(2);
                    cputs(pathname);
                    message(" Error ", "Unable to open base font.");
                    return 0;
                }
                
                fread(filebuffer, 512, 4, f);
                fclose(f);

                i = 0;
                while(i < 2048) {
                    fontbuffer[i++] ^= filebuffer[i++];
                }
                fclose(f);
            }

            return 1;
        }
    }
    return 0;
}

int save_font(char *filename) {
    int current_entry = 0;
    FILE *f;

    strcpy(pathname, prefix);
    strcat(pathname, filename);

    f = fopen(pathname, "wb");
    if(f == NULL) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to open font file.");
        return 0;
    }

    backdrop(PROGNAME);
    gotoy(12); gotox(14);
    cputs("Saving Font");

    fwrite(fontbuffer, 512, 4, f);

    fclose(f);

    return 1;
}

void index_fonts(void) {
    struct dirent *ent;
    char *ext;
    int current_entry = 0;

    rewinddir(fontdir);

    backdrop(PROGNAME);
    line_count = 0;
    if(top_entry > 0) {
        gotoy(2); gotox(1);
        cputc(UP_ARROW);
    }
    while (ent = readdir(fontdir)) {
        ext = strrchr (ent->d_name, '.');
        if (!ext || (strcasecmp (ext, ".pf") && strcasecmp (ext, ".cf")))
            continue;

        // Seek to first entry on the screen
        if(current_entry < top_entry) {
            current_entry++;
            continue;
        }

        if(line_count < FONT_MAX) {
            gotoy(3+line_count);
            if(current_entry == selected_entry) {
                gotox(1);
                cputc(RIGHT_ARROW);
                arrow_line = 3+line_count;
            }
            gotox(3);
            printlimited(ent->d_name, 32);
            cputs("\r\n");
            current_entry++;
            line_count++;
        }
    }

    more_fonts = (total_fonts > current_entry);
    if(more_fonts) {
        gotoy(21); gotox(1);
        cputc(DOWN_ARROW);
    }
}

int select_font(void) {
    for(;;) {
        switch(cgetc()) {
            case 0x0D:
                // Select
                if(load_font()) {
                    action_menu();
                }
                return 1;
            case 'T':
            case 't':
                if(load_font()) {
                    upload_font(fontbuffer);
                }
                return 1;
            case 'I':
            case 'i':
                if(load_font()) {
                    int selected_slot = slot_menu();
                    if(selected_slot >= 0) {
                        write_font(fontbuffer, font_slot[selected_slot].block);
                    }
                }
                return 1;
            case 0x08:
            case 0x0B:
                // Left (UP)
                if(selected_entry > 0) {
                    selected_entry--;
                }
                if(selected_entry < 0) {
                    selected_entry = 0;
                }

                // Entry is on the same page, don't re-read the disk directory
                if((selected_entry >= top_entry) && (selected_entry < (top_entry + line_count))) {
                    gotoy(arrow_line); gotox(1);
                    cputc(' ');

                    arrow_line = 3+(selected_entry - top_entry);
                    gotoy(arrow_line); gotox(1);
                    cputc(RIGHT_ARROW);
                    break;
                } else {
                    // Entry is on another page, re-read the disk directory
                    top_entry = (selected_entry / FONT_MAX) * FONT_MAX;
                    return 1;
                }
                return 1;
            case 0x0A:
            case 0x15:
                // Right (DOWN)
                if(selected_entry < (total_fonts - 1)) {
                    selected_entry++;
                }
                if(selected_entry >= total_fonts) {
                    selected_entry = total_fonts - 1;
                }

                // Entry is on the same page, don't re-read the disk directory
                if((selected_entry >= top_entry) && (selected_entry < (top_entry + line_count))) {
                    gotoy(arrow_line); gotox(1);
                    cputc(' ');

                    arrow_line = 3+(selected_entry - top_entry);
                    gotoy(arrow_line); gotox(1);
                    cputc(RIGHT_ARROW);
                    break;
                } else {
                    // Entry is on another page, re-read the disk directory
                    top_entry = (selected_entry / FONT_MAX) * FONT_MAX;
                    return 1;
                }
                return 1;
            case 0x1B:
                // Abort
                return 0;
        }
    }
}

void browse_fonts(void) {
    if(!open_fontdir()) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to open font folder.");
        return;
    }

    do {
        index_fonts();
    } while(select_font());
    closedir(fontdir);
}

int main_menu_action(int action) {
    int selected_slot;

    switch(action) {
        case 0:
            browse_fonts();
            return 2;
        case 1:
            memset(fontname, 0, sizeof(fontname));
            selected_slot = slot_menu();
            if(selected_slot >= 0) {
                read_font(fontbuffer, font_slot[selected_slot].block);
                //save_font(font_slot[selected_slot].diskname);
            }
            return 2;
        case 2:
            memset(fontname, 0, sizeof(fontname));
            selected_slot = slot_menu();
            if(selected_slot >= 0) {
                remove_font(font_slot[selected_slot].block);
            }
            return 2;
    }
    return 1;
}

void main_menu(void) {
    int paint_menu = 2;
    int selected_action = 0;

    for(;;) {
        if(paint_menu == 2) {
            backdrop(PROGNAME);
            window(" Main Menu ", 24, 7, 0);
            paint_menu = 1;
        }
        if(paint_menu > 0) {
            gotoy(10); gotox(9);
            revers(selected_action == 0);
            cputs(" Browse fonts         ");

            gotoy(12); gotox(9);
            revers(selected_action == 1);
            cputs(" Download stored font ");

            gotoy(14); gotox(9);
            revers(selected_action == 2);
            cputs(" Remove stored font   ");

            revers(0);
            paint_menu = 0;
        }

        switch(cgetc()) {
            case 'B':
            case 'b':
                paint_menu = main_menu_action(selected_action = 0);
                break;
            case 'D':
            case 'd':
                paint_menu = main_menu_action(selected_action = 1);
                break;
            case 'R':
            case 'r':
                paint_menu = main_menu_action(selected_action = 2);
                break;
            case 0x08:
            case 0x0B:
                if(selected_action > 0)
                    selected_action--;
                paint_menu = 1;
                break;
            case 0x0A:
            case 0x15:
                if(selected_action < 2)
                    selected_action++;
                paint_menu = 1;
                break;
            case 0x1B:
                if(confirm(" Are you sure? ", "Quit the Font Manager?"))
                    return;

                paint_menu = 2;
                break;
            case 0x0D:
                // Select
                paint_menu = main_menu_action(selected_action);
                break;
        }
    }
}

void main (void) {
    if(!prompt_slot(PROGNAME)) {
        exec("MENU.SYSTEM", "");
        return;
    }
    
    backdrop(PROGNAME);
    gotoy(12); gotox(13);
    cputs("Indexing Fonts");

    main_menu();
    clrscr();
    exec("MENU.SYSTEM", "");
}
