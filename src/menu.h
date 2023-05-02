#ifndef __MENU_H
#define __MENU_H

#if defined(__APPLE2ENH__)
 #define CHAR_CURSOR              0x7F
 #define CHAR_SEPARATOR           '_'
 #define CHAR_BAR_MIDDLE          0xD3
 #define CHAR_BORDER_BOTTOM       0xCC
 #define CHAR_TITLEBAR_ENCLOSED   0xDC
 #define CHAR_BORDER_TOP          '_'
 #define CHAR_BORDER_RIGHT        0xDA
 #define CHAR_BORDER_LEFT         0xDF
 #define CHAR_BORDER_TOP_LEFT     0xDA
 #define CHAR_BORDER_TOP_RIGHT    0xDF
 #define CHAR_TITLEBAR_FOLDER     0xD4
 #define CHAR_SCROLLBAR_FULL      0xA0
 #define CHAR_SCROLLBAR_EMPTY     0xD6
 #define CHAR_SCROLLBAR_RIGHT     0xDF
 #define CHAR_SCROLLBAR_LEFT      0xDA
 #define CHAR_SCROLLBAR_TOP       0xDC
 #define CHAR_SCROLLBAR_BOTTOM    0xDC
 #define CHAR_BORDER_BOTTOM_LEFT  ' '
 #define CHAR_BORDER_BOTTOM_RIGHT ' '
#else
 #define CHAR_CURSOR              (' '|0x80)
 #define CHAR_SEPARATOR           '_'
 #define CHAR_BAR_MIDDLE          '-'
 #define CHAR_BORDER_BOTTOM       '-'
 #define CHAR_TITLEBAR_ENCLOSED   '-'
 #define CHAR_BORDER_TOP          '_'
 #define CHAR_BORDER_RIGHT        '!'
 #define CHAR_BORDER_LEFT         '!'
 #define CHAR_BORDER_TOP_LEFT     '.'
 #define CHAR_BORDER_TOP_RIGHT    '.'
 #define CHAR_TITLEBAR_FOLDER     '_'
 #define CHAR_SCROLLBAR_FULL      0xA0
 #define CHAR_SCROLLBAR_EMPTY     ':'
 #define CHAR_SCROLLBAR_RIGHT     ' '
 #define CHAR_SCROLLBAR_LEFT      ' '
 #define CHAR_SCROLLBAR_TOP       ' '
 #define CHAR_SCROLLBAR_BOTTOM    ' '
 #define CHAR_BORDER_BOTTOM_LEFT  '\''
 #define CHAR_BORDER_BOTTOM_RIGHT '\''
#endif


void repeatchar(char ch, int num) {
    while(num--) {
        cputc(ch);
    }
}

void hline() {
    repeatchar(CHAR_BAR_MIDDLE, 40);
}

void backdrop(char *str) {
    int w, i;

    w = strlen(str);
    if(w > 20) {
        w = 20;
    }

    clrscr();
    gotoy(0); gotox(1);
    cputs("V2 Analog");

    gotoy(0); gotox(39 - w);

    if((w >= 19) && str[19])
#if defined(__APPLE2ENH__)
        w = 19;
#else
        w = 17;
#endif

    for(i = 0; i < w; i++) {
        cputc(str[i]);
    }

#if defined(__APPLE2ENH__)
    if((i == 19) && str[i])
        cputc(0xC9); // Elipsis
#else
    if((i == 17) && str[i])
        cputs("...");
#endif

    gotoy(1); gotox(0);
    hline();

    gotoy(22); gotox(0);
    hline();

    gotoy(23); gotox(4);
    cputs("https://www.v2retrocomputing.com");
}

void printlimited(char *str, int maxwidth) {
    int i;

    if(strlen(str) <= maxwidth) {
        cputs(str);
    } else {
        for(i = 0; i < (maxwidth-3); i++) {
            cputc(str[i]);
        }
        cputs("...");
    }
}

void window(char *title, int width, int height, int type) {
    int w, x, y, a, b, c;

    if(width > 38) {
        width = 40;
    }

    if(height > 22) {
       height = 22;
    }

    y = 12 - ((height+2)/2);

    if(strlen(title) > (width-2)) {
        w = width-2;
    } else {
        w = strlen(title);
    }

    if(width < 40) {
      x = 20 - (width/2);
    } else {
      x = 0;
    }

#if defined(__APPLE2ENH__)
    a = 0;
#else
    a = (width < 38);
#endif
    b = (width < 40);
    x -= a+b;

#if defined(__APPLE2ENH__)
    if(y > 2) {
        gotoy(y-1); gotox(x);
        if(a) cputc(' ');
        if(b) cputc(' ');
        repeatchar(CHAR_BORDER_TOP, type ? width : w);
        if(!type && (w < width)) repeatchar(' ', width - w);
        if(b) cputc(' ');
        if(a) cputc(' ');
    }
#else
    if(y > 2) {
        gotoy(y-1); gotox(x);
        repeatchar(' ', width+a+a+b+b);
    }
#endif
    if(y+height < 22) {
        gotoy(y+height); gotox(x);
        repeatchar(' ', width+a+a+b+b);
    }

    gotoy(y); gotox(x);
    if(a) cputc(' ');
    if(b) cputc(CHAR_BORDER_TOP_LEFT);
    if(type) {
        c = (width - w) / 2;
        repeatchar(CHAR_TITLEBAR_ENCLOSED, c);
    }
    revers(1);
    printlimited(title, w);
    revers(0);
#if defined(__APPLE2ENH__)
    if(!type) cputc(CHAR_TITLEBAR_FOLDER);
#endif
    if(type) {
        repeatchar(CHAR_TITLEBAR_ENCLOSED, width - (w+c));
        if(b) cputc(CHAR_BORDER_TOP_RIGHT);
    } else {
#if defined(__APPLE2ENH__)
        repeatchar(CHAR_BORDER_TOP, width-(w+1));
        if(b) cputc(' ');
#else
        repeatchar(CHAR_BORDER_TOP, width-w);
        if(b) cputc(CHAR_BORDER_TOP_RIGHT);
#endif
    }
    if(a) cputc(' ');

    y++;

#if defined(__APPLE2ENH__)
    gotoy(y); gotox(x);
    if(a) cputc(' ');
    if(b) cputc(CHAR_BORDER_RIGHT);
    repeatchar(CHAR_BORDER_BOTTOM, width);
    if(b) cputc(CHAR_BORDER_LEFT);
    if(a) cputc(' ');

    for(c = 1; c < height-1; c++) {
#else
    for(c = 0; c < height; c++) {
#endif
        gotoy(y+c); gotox(x);
        if(a) cputc(' ');
        if(b) cputc(CHAR_BORDER_RIGHT);
        repeatchar(' ', width);
        if(b) cputc(CHAR_BORDER_LEFT);
        if(a) cputc(' ');
    }
#if defined(__APPLE2ENH__)
    gotoy(y+c); gotox(x);
    if(a) cputc(' ');
    if(b) cputc(CHAR_BORDER_RIGHT);
    repeatchar(CHAR_BORDER_TOP, width);
    if(b) cputc(CHAR_BORDER_LEFT);
    if(a) cputc(' ');
    c++;
#endif

    if((y+c) < 22) {
        gotoy(y+c); gotox(x);
        if(a) cputc(' ');
        if(b) cputc(CHAR_BORDER_BOTTOM_LEFT);
        repeatchar(CHAR_BORDER_BOTTOM, width);
        if(b) cputc(CHAR_BORDER_BOTTOM_RIGHT);
        if(a) cputc(' ');
    }
}

void message(char *title, char *str) {
    int w, x, c;

    if(strlen(str) > 34) {
        c = w = 34;
    } else {
        c = w = strlen(str);
    }

    if(strlen(title) > 34) {
        c = 34;
    } else if(strlen(title) > w) {
        c = strlen(title);
    }

    window(title, c+4, 7, 1);

    x = 20 - (w / 2);
    gotoy(11); gotox(x);
    printlimited(str, w);
//    for(c = 0; (c < w) && str[c]; c++) {
//        cputc(str[c]);
//    }

    x = 20 - 2;
    gotoy(13); gotox(x);
    revers(1);
    cputs(" OK ");
    revers(0);

    for(;;) {
        switch(cgetc()) {
        case 0x0A:
        case 0x0D:
        case 0x1B:
        case 'O':
        case 'o':
            return;
        }
    }
}

int confirm(char *title, char *str) {
    int w, x, c, sel = 1;

    if(strlen(str) > 34) {
        c = w = 34;
    } else {
        c = w = strlen(str);
    }

    if(strlen(title) > 34) {
        c = 34;
    } else if(strlen(title) > w) {
        c = strlen(title);
    }

    window(title, c+4, 7, 1);

    x = 20 - (w / 2);
    gotoy(11); gotox(x);
    printlimited(str, w);

    for(;;) {
        x = 20 - 5;
        gotoy(13); gotox(x);
        if(sel) {
            revers(1);
            cputs(" Yes ");
            revers(0);
            cputs(" [No]");
        } else {
            cputs("[Yes] ");
            revers(1);
            cputs(" No ");
            revers(0);
        }

        switch(cgetc()) {
        case 0x08:
        case 0x0A:
        case 0x0B:
        case 0x15:
            sel = !sel;
            break;
        case 0x0D:
            return sel;
        case 'Y':
        case 'y':
            return 1;
        case 0x1B:
        case 'N':
        case 'n':
            return 0;
        }
    }
}

#ifdef __V2ANALOG_H
int prompt_slot(char *progname) {
    int c;
    int done=0;

    // Attempt to find the lowest numbered V2 Analog Card
    for(c = 7; c > 0; c--) {
        if(!memcmp("V2A", (uint8_t *)(0xC0F8 + (c << 8)), 3)) {
            cardslot = c;
        }
    }

    backdrop(progname);

    window(" Slot Number? ", 38, 7, 1);
    gotoy(11); gotox(2);
    cputs("Which slot is the card installed in?");

    while(!done) {
#if defined(__APPLE2ENH__)
        gotoy(12); gotox(19);
        repeatchar(CHAR_BORDER_TOP, 3);
#endif
        gotoy(13); gotox(19);
        revers(1);
        printf(" %i ", cardslot);
        revers(0);

        c = cgetc();
        if((c >= '1') && (c <= '7')) {
            cardslot = c - '0';
            return 1;
        } else 
        switch(c) {
            case 0x08:
            case 0x0A:
            case 0x2D:
                if(cardslot > 0)
                    cardslot--;
                break;
            case 0x0B:
            case 0x15:
            case 0x2B:
                if(cardslot < 7)
                    cardslot++;
                break;
            case 0x0D:
                done = 1;
                return 1;
            case 0x1B:
                return 0;
        }
    }

}
#endif

#endif /* __MENU_H */
