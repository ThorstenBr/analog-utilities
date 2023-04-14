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
#define UP_ARROW    ('+' | 0x80)
#define RIGHT_ARROW ('>' | 0x80)
#define DOWN_ARROW  ('+' | 0x80)
#endif

#define FONT_MAX 15

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

typedef struct fontentry_s {
    char pathname[FILENAME_MAX];
    char fontname[FILENAME_MAX];
} fontentry_t;

fontentry_t fontlist[FONT_MAX];

void print_menu_select(char *str, int width, int highlighted, int selected) {
    revers(highlighted);
    if(selected) {
        cputs(" [");
    } else {
        cputs("  ");
    }
    printlimited(str, width - 4);
    if(selected) {
        cputs("] ");
    } else {
        cputs("  ");
    }
    width -= longmin(width, strlen(str) + 4);
    if(width > 0)
        repeatchar(' ', width);
    revers(0);
}

void print_menu_item(char *str, int highlighted) {
    revers(highlighted);
    cputs(str);
    revers(0);
}

void remove_font(uint16_t block) {
    char dummy;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Removing Font,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    if(cfg_cmd1("Ce", block)) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to erase block.");
        goto cleanup;
    }

    backdrop(PROGNAME);
    message(" Success ", "Block erased.");

cleanup:
    dummy = *(volatile char*)0xCFFF;
}

void upload_font(uint8_t *buffer) {
    char dummy;
    uint16_t i;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Uploading font,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    CF_PTRL = 0;
    CF_PTRH = 0;
    for(i = 0; i < 4096; i++) {
        CF_DATW = buffer[i];
    }

    if(cfg_cmd0("CT")) {
        backdrop(PROGNAME);
        message(" Error ", "Communication Error");
        goto cleanup;
    }

    backdrop(PROGNAME);
    message(" Success ", "Font uploaded.");

cleanup:
    dummy = *(volatile char*)0xCFFF;
}

int write_font(uint8_t *buffer, uint16_t block) {
    int rv = 1;
    char dummy;
    uint16_t i;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Erasing flash,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    if(cfg_cmd1("Ce", block)) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to erase block.");
        goto cleanup;
    }

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Writing flash,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    CF_PTRL = 0;
    CF_PTRH = 0;
    for(i = 0; i < 4096; i++) {
        CF_DATW = buffer[i];
    }
    if(cfg_cmd1("Cw", block)) {
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

    if(cfg_cmd1("Cr", block)) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to read block.");
        goto cleanup;
    }

    CF_PTRL = 0;
    CF_PTRH = 0;
    for(i = 0; i < 4096; i++) {
        buffer[i] = CF_DATR;
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
    { "00 US            Enh", 0x00 },
    { "01 US          UnEnh", 0x01 },
    { "02 Clinton Turner V1", 0x02 },
    { "03 ReActiveMicro Enh", 0x03 },
    { "04 Dan Paymar    Enh", 0x04 },
    { "05 Blippo Black  Enh", 0x05 },
    { "06 Byte          Enh", 0x06 },
    { "07 Colossal      Enh", 0x07 },
    { "08 Count         Enh", 0x08 },
    { "09 Flow          Enh", 0x09 },
    { "0A Gothic        Enh", 0x0a },
    { "0B Outline       Enh", 0x0b },
    { "0C PigFont       Enh", 0x0c },
    { "0D Pinocchio     Enh", 0x0d },
    { "0E Slant         Enh", 0x0e },
    { "0F Stop          Enh", 0x0f },
    
    { "10 Euro        UnEnh", 0x10 },
    { "11 Euro          Enh", 0x11 },
    { "12 Clinton Turner V2", 0x12 },
    { "13 German        Enh", 0x13 },
    { "14 German      UnEnh", 0x14 },
    { "15 French        Enh", 0x15 },
    { "16 French      UnEnh", 0x16 },
    { "17 Hebrew        Enh", 0x17 },
    { "18 Hebrew      UnEnh", 0x18 },
    { "19 Apple II+     Enh", 0x19 },
    { "1A Apple II+   UnEnh", 0x1a },
    { "1B Katakana      Enh", 0x1b },
    { "1C Cyrillic      Enh", 0x1c },
    { "1D Greek         Enh", 0x1d },
    { "1E Esperanto     Enh", 0x1e },
    { "1F Videx         Enh", 0x1f },

    { "20 Apple II/II+", 0x20 },
    { "21 Apple IIe", 0x21 },
    { "22 Apple IIgs", 0x22 },
    { "23 Pravetz", 0x23 },
    { "24 Custom", 0x24 },
    { "25 Custom", 0x25 },
    { "26 Custom", 0x26 },
    { "27 Custom", 0x27 },
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
    int paint_menu = 0x7;
    int selected_slot = 0;
    int top = 0;
    int y, i;
    uint8_t block = parsehex8(fontname);
    int total_slots = (sizeof(font_slot)/sizeof(font_slot_t));

#if 0
    // Try to find a matching slot from our table
    for(i = 0; i < (sizeof(font_slot)/sizeof(font_slot_t)); i++) {
        if(block == font_slot[i].block) {
            selected_slot = i;
            top = (selected_slot / FONT_MAX) * FONT_MAX;
            break;
        }
    }
#endif

    while(paint_menu != -1) {
        if(paint_menu & 0x4) {
            backdrop(PROGNAME);
            window(" Font Slot? ", 28, FONT_MAX+2, 0);
        }
        if(paint_menu & 0x2) {
            y=5;
            if((top > 0) || (total_slots >= (FONT_MAX+top))) {
                for(i = 1; i < FONT_MAX-1; i++) {
                    gotoy(y+i); gotox(31);
                    cputc(CHAR_SCROLLBAR_LEFT);
                    if(((i < (FONT_MAX / 3)) && (top < (total_slots / 3))) ||
                       ((i >= (FONT_MAX / 3)) && (i < ((FONT_MAX * 2) / 3)) && (top >= (total_slots / 3)) && (top < (total_slots * 2) / 3)) ||
                       ((i >= ((FONT_MAX * 2) / 3)) && (top >= ((total_slots * 2) / 3)))) {
                        cputc(CHAR_SCROLLBAR_FULL);
                    } else {
                        cputc(CHAR_SCROLLBAR_EMPTY);
                    }
                    cputc(CHAR_SCROLLBAR_RIGHT);
                }
                gotoy(y-1); gotox(32);
                cputc(CHAR_SCROLLBAR_TOP);
                gotoy(y+0); gotox(31);
                cputc(CHAR_SCROLLBAR_LEFT);
                cputc(UP_ARROW);
                cputc(CHAR_SCROLLBAR_RIGHT);
                gotoy(y+FONT_MAX-1); gotox(31);
                cputc(CHAR_SCROLLBAR_LEFT);
                cputc(DOWN_ARROW);
                cputc(CHAR_SCROLLBAR_RIGHT);
                gotoy(y+FONT_MAX); gotox(32);
                cputc(CHAR_SCROLLBAR_BOTTOM);
            }
        }
        if(paint_menu & 0x1) {
            for(y = 0; y < 15; y++) {
                i = y+top;
                if(i < (sizeof(font_slot)/sizeof(font_slot_t))) {
                    gotoy(5+y); gotox(7);
                    print_menu_select(font_slot[i].entry, 24, (selected_slot == i), 0);
                } else {
                    gotoy(5+y); gotox(7);
                    repeatchar(' ', 24);
                }
            }

        }

        paint_menu = 0;
        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_slot > 0) {
                    selected_slot--;
                    paint_menu |= 1;
                    if(selected_slot < top) {
                        top = (selected_slot / 15) * 15;
                        paint_menu |= 2;
                    }
                }
                break;
            case 0x0A:
            case 0x15:
                if(selected_slot < ((sizeof(font_slot)/sizeof(font_slot_t))-1)) {
                    selected_slot++;
                    paint_menu |= 1;
                    if(selected_slot > top + 14) {
                        top = (selected_slot / 15) * 15;
                        paint_menu |= 2;
                    }
                }
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
        
        if (total_fonts < (sizeof(fontlist)/sizeof(fontentry_t))) {
            strcpy(fontlist[total_fonts].pathname, prefix);
            strcat(fontlist[total_fonts].pathname, ent->d_name);
            strcpy(fontlist[total_fonts].fontname, ent->d_name);
        }
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
                    fontbuffer[i] ^= filebuffer[i];
                    i++;
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

    line_count = 0;
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
            strcpy(fontlist[line_count].pathname, prefix);
            strcat(fontlist[line_count].pathname, ent->d_name);
            strcpy(fontlist[line_count].fontname, ent->d_name);

            current_entry++;
            line_count++;
        }
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
                return 0xB;
            case 'T':
            case 't':
                if(load_font()) {
                    upload_font(fontbuffer);
                }
                return 0xB;
            case 'I':
            case 'i':
                if(load_font()) {
                    int selected_slot = slot_menu();
                    if(selected_slot >= 0) {
                        write_font(fontbuffer, font_slot[selected_slot].block);
                    }
                }
                return 0xB;
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
                    return 0x1;
                } else {
                    // Entry is on another page, re-read the disk directory
                    return 0x7;
                }
                return 0;
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
                    return 0x1;
                } else {
                    // Entry is on another page, re-read the disk directory
                    return 0x7;
                }
                return 0;
            case 0x1B:
                // Abort
                return -1;
        }
    }
}

void browse_fonts(void) {
    int paint_menu = 0xF;
    int i;
    int y = 12 - 7;

    if(!open_fontdir()) {
        backdrop(PROGNAME);
        message(" Error ", "Unable to open font folder.");
        return;
    }

    while(paint_menu >= 0) {
        if(paint_menu & 0x8) {
            backdrop(PROGNAME);
            window(" Font Browser ", 26, 17, 0);
        }            
        if(paint_menu & 0x4) {
            top_entry = (selected_entry / FONT_MAX) * FONT_MAX;
            index_fonts();
        }
        if(paint_menu & 0x2) {
            more_fonts = (total_fonts >= (FONT_MAX+top_entry));
            if((top_entry > 0) || (more_fonts)) {
                for(i = 1; i < FONT_MAX-1; i++) {
                    gotoy(y+i); gotox(30);
                    cputc(CHAR_SCROLLBAR_LEFT);
                    if(i < (FONT_MAX / 3)) {
                        if(top_entry > (total_fonts / 3)) {
                            cputc(CHAR_SCROLLBAR_EMPTY);
                        } else {
                            cputc(CHAR_SCROLLBAR_FULL);
                        }
                    } else if(i < ((FONT_MAX * 2) / 3)) {
                            cputc(CHAR_SCROLLBAR_FULL);
                    } else {
                        if(top_entry < (total_fonts / 3)) {
                            cputc(CHAR_SCROLLBAR_EMPTY);
                        } else {
                            cputc(CHAR_SCROLLBAR_FULL);
                        }
                    }
                    cputc(CHAR_SCROLLBAR_RIGHT);
                }
                gotoy(y-1); gotox(31);
                cputc(CHAR_SCROLLBAR_TOP);
                gotoy(y+0); gotox(30);
                cputc(CHAR_SCROLLBAR_LEFT);
                cputc(UP_ARROW);
                cputc(CHAR_SCROLLBAR_RIGHT);
                gotoy(y+FONT_MAX-1); gotox(30);
                cputc(CHAR_SCROLLBAR_LEFT);
                cputc(DOWN_ARROW);
                cputc(CHAR_SCROLLBAR_RIGHT);
                gotoy(y+FONT_MAX); gotox(31);
                cputc(CHAR_SCROLLBAR_BOTTOM);
            }
        }
        if(paint_menu & 0x1) {
            for(i = 0; i < longmin((total_fonts-top_entry), FONT_MAX); i++) {
                gotoy(y+i); gotox(8);
                print_menu_select(fontlist[i].fontname, 22, (selected_entry == i+top_entry), 0);
            }
            while(i < FONT_MAX) {
                gotoy(y+i); gotox(8);
                repeatchar(' ', 22);
                i++;
            }
        }
        paint_menu = select_font();
    };
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
        case 3:
            selected_slot = slot_menu();
            if(selected_slot >= 0) {
                CARD_REGISTER(0x0B) = selected_slot;
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
            window(" Main Menu ", 26, 9, 0);
            paint_menu = 1;
        }
        if(paint_menu > 0) {
            gotoy(9); gotox(8);
            print_menu_select("Browse fonts", 24, (selected_action == 0), 0);

            gotoy(11); gotox(8);
            print_menu_select("Download stored font", 24, (selected_action == 1), 0);

            gotoy(13); gotox(8);
            print_menu_select("Remove stored font", 24, (selected_action == 2), 0);

            gotoy(15); gotox(8);
            print_menu_select("Select current font", 24, (selected_action == 3), 0);

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
            case 'S':
            case 's':
                paint_menu = main_menu_action(selected_action = 3);
                break;
            case 0x08:
            case 0x0B:
                if(selected_action > 0)
                    selected_action--;
                paint_menu = 1;
                break;
            case 0x0A:
            case 0x15:
                if(selected_action < 3)
                    selected_action++;
                paint_menu = 1;
                break;
            case 0x1B:
                backdrop(PROGNAME);
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
        goto cleanup;
    }
    
    backdrop(PROGNAME);
    gotoy(12); gotox(13);
    cputs("Indexing Fonts");

    main_menu();

cleanup:
    backdrop(PROGNAME);
    gotoy(12); gotox(13);
    cputs("Launching Menu");

    exec("MENU.SYSTEM", "");
}
