#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "v2types.h"
#include "v2loader.h"

#define PROGNAME "Startup Menu"
volatile uint16_t cardslot = 3;

#include "v2analog.h"
#include "menu.h"

int confirm_exit(void) {
    backdrop(PROGNAME);
    if(confirm(" Are you sure? ", "Quit?")) {
        clrscr();
        return -1;
    }
    return 2;
}

typedef enum { 
    MENU_TERMINATOR = 0,
    MENU_MISSING,
    MENU_SEPARATOR,
    MENU_LAUNCH_CHECKARGS,
    MENU_LAUNCH_NOCHECKARGS,
    MENU_EXIT
} menu_type_t;

typedef struct menu_s {
    int enabled;
    menu_type_t type;
    char *entry;
    char *filename_enhanced;
    char *filename_base;
    char *arguments;
} menu_t;

menu_t launcher[] = {
    { 0, MENU_LAUNCH_NOCHECKARGS, " Config Utility       ", "CONFIG.ENH",     "CONFIG.BASE",    "" },
    { 0, MENU_LAUNCH_NOCHECKARGS, " Font Manager         ", "FONTMGR.ENH",    "FONTMGR.BASE",   "" },
    { 0, MENU_SEPARATOR,          NULL,                     NULL,             NULL,             NULL },
    { 0, MENU_LAUNCH_NOCHECKARGS, " Applesoft Basic      ", "BASIC.SYSTEM",   "BASIC.SYSTEM",   "" },
    { 0, MENU_LAUNCH_NOCHECKARGS, " ADTPro Serial        ", "ADTPRO/ADTPRO",  "ADTPRO/ADTPRO",  "" },
    { 0, MENU_LAUNCH_NOCHECKARGS, " Virtual Serial Drive ", "VDRIVE/VSDRIVE", "VDRIVE/VSDRIVE", "" },
    { 0, MENU_SEPARATOR,          NULL,                     NULL,             NULL,             NULL },
    { 1, MENU_EXIT,               " Exit                 ", NULL,             NULL,             NULL },
    { 0, MENU_TERMINATOR,         NULL,                     NULL,             NULL,             NULL },
};

int main_menu_action(int action) {
    FILE *f;
    switch(launcher[action].type) {
        case MENU_LAUNCH_CHECKARGS:
            // Type 1: Launch with Filename Argument
            if(strlen(launcher[action].arguments)) {
                f = fopen(launcher[action].arguments, "rb");
                if(f != NULL) {
                    fclose(f);
                } else {
                    return 2;
                }
            }

            // Type 2: Launch without checking arguments
        case MENU_LAUNCH_NOCHECKARGS:
#if defined(__APPLE2ENH__)
            f = fopen(launcher[action].filename_enhanced, "rb");
            if(f != NULL) {
                fclose(f);

                backdrop(PROGNAME);
                gotoy(12); gotox(9);
                cputs("Launching Application");

                exec(launcher[action].filename_enhanced, launcher[action].arguments);
            }
#endif
            f = fopen(launcher[action].filename_base, "rb");
            if(f != NULL) {
                fclose(f);

                backdrop(PROGNAME);
                gotoy(12); gotox(9);
                cputs("Launching Application");

                exec(launcher[action].filename_base, launcher[action].arguments);
            }

        default:
            return 2;

        case MENU_EXIT:
            return confirm_exit();
   }
}

void main (void) {
    int i, y;
    int paint_menu = 2;
    int selected_item = 0;
    int launcher_rows = 0;
    int last_item = 0;
    int window_top;
    int separator_allowed;
    int go = 0;
    FILE *f;

    backdrop(PROGNAME);
    gotoy(12); gotox(14);
    cputs("Loading Menu");

    // Check for the presence of every menu item and it's arguments
    separator_allowed = 0;
    for(i = 0; launcher[i].type != MENU_TERMINATOR; i++) {
        switch(launcher[i].type) {
            case MENU_LAUNCH_CHECKARGS:
                if(strlen(launcher[i].arguments)) {
                    f = fopen(launcher[i].arguments, "rb");
                    if(f != NULL) {
                        fclose(f);
                    } else {
                        launcher[i].type = MENU_MISSING;
                        break;
                    }
                }
            case MENU_LAUNCH_NOCHECKARGS:
#if defined(__APPLE2ENH__)
                f = fopen(launcher[i].filename_enhanced, "rb");
                if(f != NULL) {
                    fclose(f);
                } else {
#endif
                    f = fopen(launcher[i].filename_base, "rb");
                    if(f != NULL) {
                        fclose(f);
                    } else {
                        launcher[i].type = MENU_MISSING;
                        break;
                    }
#if defined(__APPLE2ENH__)
                }
            case MENU_EXIT:
                launcher_rows+=2;
                last_item = i;
                separator_allowed = 1;
                break;
            case MENU_SEPARATOR:
                if(separator_allowed) {
                    launcher_rows++;
                    separator_allowed = 0;
                }
                break;
#endif
        }
    }
 
    // Make sure a valid item is selected.
    while((launcher[selected_item].type < MENU_LAUNCH_CHECKARGS) && (selected_item < last_item)) {
        selected_item++;
    }

    window_top = 13 - ((launcher_rows+1)/2);

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop(PROGNAME);

            window(" Applications ", 24, launcher_rows+1, 0);

            y = 0;
            separator_allowed = 0;
            for(i = 0; launcher[i].type != MENU_TERMINATOR; i++) {
                switch(launcher[i].type) {
                    case MENU_MISSING:
                        break;
                    case MENU_LAUNCH_CHECKARGS:
                    case MENU_LAUNCH_NOCHECKARGS:
                    case MENU_EXIT:
                        y+=2;
                        separator_allowed = 1;
                        break;
                    case MENU_SEPARATOR:
                        if(separator_allowed) {
                            gotoy(window_top+y); gotox(8);
                            repeatchar(CHAR_BORDER_BOTTOM, 24);
                            y++;
                            separator_allowed = 0;
                        }
                        break;
                }
            }
        }
        if(paint_menu) {
            y = 0;
            separator_allowed = 0;
            for(i = 0; launcher[i].type != MENU_TERMINATOR; i++) {
                switch(launcher[i].type) {
                    case MENU_MISSING:
                        break;
                    case MENU_LAUNCH_CHECKARGS:
                    case MENU_LAUNCH_NOCHECKARGS:
                    case MENU_EXIT:
                        gotoy(window_top+y); gotox(9);
                        revers(selected_item == i);
                        cputs(launcher[i].entry);
                        revers(0);
                        y+=2;
                        separator_allowed = 1;
                        break;
                    case MENU_SEPARATOR:
                        if(separator_allowed) {
                            y++;
                            separator_allowed = 0;
                        }
                        break;
                }
            }
            paint_menu = 0;
        }

        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_item > 0) {
                    selected_item--;
                    // Skip separators and inactive items
                    while((launcher[selected_item].type < MENU_LAUNCH_CHECKARGS) && (selected_item > 0)) {
                        selected_item--;
                    }
                    while((launcher[selected_item].type < MENU_LAUNCH_CHECKARGS) && (selected_item < last_item)) {
                        selected_item++;
                    }
                    paint_menu = 1;
                }
                break;
            case 0x15:
            case 0x0A:
                if(selected_item < last_item) {
                    selected_item++;
                    // Skip separators and inactive items
                    while((launcher[selected_item].type < MENU_LAUNCH_CHECKARGS) && (selected_item < last_item)) {
                        selected_item++;
                    }
                    while((launcher[selected_item].type < MENU_LAUNCH_CHECKARGS) && (selected_item > 0)) {
                        selected_item--;
                    }
                    paint_menu = 1;
                }
                break;

            case 0x1B:
                paint_menu = confirm_exit();
                break;
            case 0x0D:
                go = 1;
                break;
        }

        if(go) {
            paint_menu = main_menu_action(selected_item);
            go = 0;
        }
    }
}
