#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "v2types.h"
#include "cfgtoken.h"
#include "v2loader.h"

#define PROGNAME "Config Utility"
volatile uint16_t cardslot = 3;

#include "v2analog.h"
#include "menu.h"

#define menutop 3
#define menuleft 2

uint16_t cfb;

serialmux_t serialmux[2] = { SERIAL_PRINTER, SERIAL_WIFI };
uint16_t baud[2] = { 19200, 19200 };

usbmux_t usbmux = USB_GUEST_CDC;
compat_t machine = MACHINE_AUTO;
wifimode_t wifimode = WIFI_AP;
char wifi_ssid[32] = "V2RetroNet";
char wifi_psk[32] = "Analog";
uint32_t wifi_address = 0x00000000;
uint32_t wifi_netmask = 0xFFFFFF00;

char jd_host[32] = "192.168.0.1";
uint16_t jd_port = 9100;

#define MAX_CFG_SIZE 1024
uint8_t blockbuffer[MAX_CFG_SIZE];

#define CARD_TIMEOUT 0x3fff
uint16_t timeout = CARD_TIMEOUT;

uint8_t default_font = 0;
uint8_t mono_palette = 0;
uint8_t terminal_fgcolor = 0xF;
uint8_t terminal_bgcolor = 0x0;
uint8_t terminal_border = 0x0;

uint16_t default_gs_palette[16] = {
    0x0000, 0x0606, 0x000a, 0x0a2f,
    0x0040, 0x0444, 0x008f, 0x066f,
    0x0220, 0x0f40, 0x0888, 0x0f6f,
    0x00c2, 0x0cc0, 0x09fa, 0x0fff
};

uint16_t default_lc_palette[16] = {
    0x0000, 0x00c3, 0x0005, 0x14f,
    0x0010, 0x0092, 0x0027, 0x0df,
    0x0048, 0x01d0, 0x0124, 0x1e7,
    0x0031, 0x01b0, 0x013d, 0x1ff
};

uint16_t rgbpalette[16];
uint8_t video7_enabled = 1;
uint8_t jumpers = 0;
uint8_t hardware_type = 0x00;
uint8_t config_rev = 0x00;

typedef struct menu_strings_s {
    char *str;
} menu_strings_t;

uint16_t baudvalue[9] = {
    75, 150, 300, 600, 1200, 2400, 4800, 9600, 19200
};

menu_strings_t baud_strings[9] = {
    { "75" },
    { "150" },
    { "300" }, 
    { "600" },
    { "1200" },
    { "2400" },
    { "4800" },
    { "9600" },
    { "19200" }
};

menu_strings_t wifi_mode_str[2] = {
    {"Access Point"},
    {"Client"}
};

menu_strings_t serialmux_strings[4] = {
    { "Loopback" },
    { "USB CDC" },
    { "WiFi Modem" },
    { "Printer" }
};

menu_strings_t font_strings[40] = {
    { "00: US Enhanced     " },
    { "01: US Un-Enhanced  " },
    { "02: Clinton V1 Enh  " },
    { "03: Reactive Micro  " },
    { "04: Dan Paymar Enh  " },
    { "05: Blippo Enhanced " },
    { "06: Byte Enhanced   " },
    { "07: Colossal Enh    " },
    { "08: Count Enhanced  " },
    { "09: Flow Enhanced   " },
    { "0A: Gothic Enhanced " },
    { "0B: Outline Enhanced" },
    { "0C: PigFont Enhanced" },
    { "0D: Pinocchio Enh   " },
    { "0E: Slant Enhanced  " },
    { "0F: Stop Enhanced   " },
    { "10: Euro UnEnhanced " },
    { "11: Euro Enhanced   " },
    { "12: Clinton V2      " },
    { "13: German Enh      " },
    { "14: German Un-Enh   " },
    { "15: French Enh      " },
    { "16: French Un-Enh   " },
    { "17: Hebrew Enh      " },
    { "18: Hebrew Un-Enh   " },
    { "19: Plus Enhanced   " },
    { "1A: Plus Un-Enhanced" },
    { "1B: KataKana        " },
    { "1C: Cyrillic        " },
    { "1D: Greek           " },
    { "1E: Esperanto       " },
    { "1F: Videx Enhanced  " },
    { "20: Apple II Un-Enh " },
    { "21: Apple IIe Enh   " },
    { "22: Apple IIgs Enh  " },
    { "23: Pravetz Enhanced" },
    { "24: PCBold Enhanced " },
    { "25: Custom          " },
    { "26: Custom          " },
    { "27: Custom          " },
};
#define FONT_COUNT (sizeof(font_strings)/sizeof(menu_strings_t))

menu_strings_t monochrome_strings[9] = {
    { "Disabled" },
    { "B&W" },
    { "Inverse" },
    { "Amber" },
    { "Amber I" },
    { "Green" },
    { "Green I" },
    { "C64" },
    { "Custom" }
};

menu_strings_t color_strings[16] = {
    { "Black" },
    { "Magenta" },
    { "D.Blue" },
    { "H.Violet" },
    { "D.Green" },
    { "D.Grey" },
    { "H.Blue" },
    { "L.Blue" },
    { "Brown" },
    { "H.Orange" },
    { "L.Grey" },
    { "Pink" },
    { "H.Green" },
    { "Yellow" },
    { "Aqua" },
    { "White" }
};

menu_strings_t machine_strings[MACHINE_MAXVALUE] = {
    { "Auto Detect" },
    { "Apple II" },
    { "Apple IIe" },
    { "Apple IIgs" },
    { "Pravetz" },
    { "Basis" },
};

void ok_button(void) {
    gotoy(14); gotox(18);
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

void error_window()
{
    backdrop(PROGNAME);
    window(" Error ", 28, 7, 1);
    gotoy(11); gotox(7);
}

uint8_t cfg_erase(uint16_t block)
{
    if(cfg_cmd1("fe", block)) {
        error_window();
        gotox(8);
        cprintf("Unable to erase block $%04X", block);
        ok_button();
        return 1;
    }
    return 0;
}

uint8_t cfg_update(uint16_t next)
{
    int cfg,i;
    for (cfg=0;cfg<=1;cfg++)
    {
		if(cfg_erase(next+cfg)) {
		    return 0;
		}

		CF_PTRL = 0;
		CF_PTRH = 0;
		for(i = 0; i < 4096; i++) {
			if (i<sizeof(blockbuffer))
				CF_DATW = blockbuffer[i];
			else
				CF_DATW = 0xff;
		}

		if(cfg_cmd1("fw", next+cfg)) {
		    ok_button();

		    backdrop(PROGNAME);
		    window(" Error ", 32, 7, 1);
		    gotoy(11); gotox(6);
		    cprintf("Unable to write block $%04X", next+cfg);
		    ok_button();

		    return 0;
		}
	}
	return 1;
}

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

void print_menu_select_int(char *str, int i, int width, int highlighted, int selected) {
    revers(highlighted);
    if(selected) {
        cputs(" [");
    } else {
        cputs("  ");
    }
    width -= 4 + cprintf(str, i);
    if(selected) {
        cputs("] ");
    } else {
        cputs("  ");
    }
    if(width > 0)
        repeatchar(' ', width);
    revers(0);
}

void print_menu_select_ip(uint32_t ip, int width, int highlighted, int selected) {
    uint8_t temp_ip[4];
    
    temp_ip[0] = (ip >> 24) & 0xff;
    temp_ip[1] = (ip >> 16) & 0xff;
    temp_ip[2] = (ip >> 8) & 0xff;
    temp_ip[3] = (ip >> 0) & 0xff;

    width -= 11;
    if(temp_ip[0] > 99) width--;
    if(temp_ip[0] > 9) width--;
    if(temp_ip[1] > 99) width--;
    if(temp_ip[1] > 9) width--;
    if(temp_ip[2] > 99) width--;
    if(temp_ip[2] > 9) width--;
    if(temp_ip[3] > 99) width--;
    if(temp_ip[3] > 9) width--;

    revers(highlighted);
    if(selected) {
        cputs(" [");
    } else {
        cputs("  ");
    }
    if(width > 0)
        repeatchar(' ', width);
    cprintf("%d.%d.%d.%d", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3]);
    if(selected) {
        cputs("] ");
    } else {
        cputs("  ");
    }
    revers(0);
}

void print_menu_select_str(char *str1, char *str2, int width, int highlighted, int selected) {
    revers(highlighted);
    if(selected) {
        cputs(" [");
    } else {
        cputs("  ");
    }
    width -= 4 + cprintf(str1, str2);
    if(selected) {
        cputs("] ");
    } else {
        cputs("  ");
    }
    if(width > 0)
        repeatchar(' ', width);
    revers(0);
}

int list_menu(char *screen_title, char *window_title, menu_strings_t *menu_str, int max_item, int current_item, int default_item) {
    int paint_menu = 2;
    int selected_item = ((current_item > max_item) || (current_item < 0)) ? 0 : current_item;
    int y;
    int top = -1;
    int i;

    y = 12 - 5;

    while(paint_menu >= 0) {
        if(top != ((selected_item / 10) * 10)) {
            top = (selected_item / 10) * 10;
        }
        if(paint_menu == 2) {
            backdrop(screen_title);
            window(window_title, 26, 12, 0);
        }
        if(paint_menu > 0) {
            for(i = 0; i < longmin((max_item-top)+1, 10); i++) {
                gotoy(y+i); gotox(8);
                print_menu_select(menu_str[i+top].str, 24, (selected_item == i+top), (current_item == default_item) && (current_item == i+top));
            }
            if(max_item > 9) while(i < 10) {
                gotoy(y+i); gotox(8);
                repeatchar(' ', 24);
                i++;
            }
            paint_menu = 0;
        }
        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_item > 0) {
                    selected_item--;
                    paint_menu = 1;
                }
                break;
            case 0x15:
            case 0x0A:
                if(selected_item < max_item) {
                    selected_item++;
                    paint_menu = 1;
                }
                break;
            case 0x0D:
                return selected_item;
            case 0x1B:
                paint_menu = -1;
        }
    }

    return default_item;
}

void default_config() {
    serialmux[0] = SERIAL_PRINTER;
    serialmux[1] = SERIAL_WIFI;
    baud[0] = 19200;
    baud[1] = 19200;
    usbmux = USB_GUEST_CDC;

    wifimode = WIFI_AP;
    strcpy(wifi_ssid, "V2RetroNet");
    strcpy(wifi_psk, "Analog");
    wifi_address = 0x00000000;
    wifi_netmask = 0xFFFFFF00;

    strcpy(jd_host, "192.168.0.1");
    jd_port = 9100;
    machine = MACHINE_AUTO;
    
    hardware_type = 'L';
    jumpers = 0;

    memcpy(rgbpalette, default_lc_palette, sizeof(rgbpalette));
}

int parse_config() {
    int i = 0;
    uint32_t *ptr = (uint32_t*)blockbuffer;
    uint32_t configlen = sizeof(blockbuffer)/sizeof(uint32_t);

    if(ptr[i++] != NEWCONFIG_MAGIC) {
        return 0;
    }

    while(i < configlen) {
        if(ptr[i] == NEWCONFIG_EOF_MARKER) {
            configlen = i++;
        } else
        switch(ptr[i] & 0x0000FFFF) {
        case CFGTOKEN_REVISION:
            config_rev = (ptr[i] >> 16) & 0xFF;
            break;
        case CFGTOKEN_HOST_AUTO:
            machine = MACHINE_AUTO;
            break;
        case CFGTOKEN_HOST_II:
            machine = MACHINE_APPLE_II;
            break;
        case CFGTOKEN_HOST_IIE:
            machine = MACHINE_APPLE_IIE;
            break;
        case CFGTOKEN_HOST_IIGS:
            machine = MACHINE_APPLE_IIGS;
            break;
        case CFGTOKEN_HOST_PRAVETZ:
            machine = MACHINE_PRAVETZ;
            break;
        case CFGTOKEN_HOST_BASIS:
            machine = MACHINE_BASIS;
            break;
        case CFGTOKEN_MUX_LOOP:
            serialmux[(ptr[i] >> 16) & 1] = SERIAL_LOOP;
            break;
        case CFGTOKEN_MUX_USB:
            serialmux[(ptr[i] >> 16) & 1] = SERIAL_USB;
            break;
        case CFGTOKEN_MUX_WIFI:
            serialmux[(ptr[i] >> 16) & 1] = SERIAL_WIFI;
            break;
        case CFGTOKEN_MUX_PRN:
            serialmux[(ptr[i] >> 16) & 1] = SERIAL_PRINTER;
            break;
        case CFGTOKEN_SER_BAUD:
            baud[(ptr[i] >> 16) & 1] = ptr[i+1];
            break;
        case CFGTOKEN_USB_HOST:
            usbmux = USB_HOST_CDC;
            break;
        case CFGTOKEN_USB_GUEST:
            usbmux = USB_GUEST_CDC;
            break;
        case CFGTOKEN_USB_MIDI:
            usbmux = USB_GUEST_MIDI;
            break;
        case CFGTOKEN_WIFI_AP:
            wifimode = WIFI_AP;
            break;
        case CFGTOKEN_WIFI_CL:
            wifimode = WIFI_CLIENT;
            break;
        case CFGTOKEN_WIFI_SSID:
            memset(wifi_ssid, 0, sizeof(wifi_ssid));
            strncpy(wifi_ssid, (char*)(&ptr[i+1]), (ptr[i] >> 24));
            break;
        case CFGTOKEN_WIFI_PSK:
            memset(wifi_psk, 0, sizeof(wifi_psk));
            strncpy(wifi_psk, (char*)(&ptr[i+1]), (ptr[i] >> 24));
            break;
        case CFGTOKEN_WIFI_IP:
            wifi_address = ptr[i+1];
            break;
        case CFGTOKEN_WIFI_NM:
            wifi_netmask = ptr[i+1];
            break;
        case CFGTOKEN_JD_HOST:
            memset(jd_host, 0, sizeof(jd_host));
            strncpy(jd_host, (char*)(&ptr[i+1]), (ptr[i] >> 24));
            break;
        case CFGTOKEN_JD_PORT:
            jd_port = ptr[i+1];
            break;
        case CFGTOKEN_FONT_00:
            default_font = (ptr[i] >> 16) & 0x3F;
            if (default_font >= FONT_COUNT)
	            default_font = 0;
            break;
        case CFGTOKEN_MONO_00:
            mono_palette = (ptr[i] >> 20) & 0xF;
            if(mono_palette) mono_palette = (mono_palette & 0x7) + 1;
            break;
        case CFGTOKEN_TBCOLOR:
            terminal_fgcolor = (ptr[i] >> 20) & 0xF;
            terminal_bgcolor = (ptr[i] >> 16) & 0xF;
            break;
        case CFGTOKEN_BORDER:
            terminal_border = (ptr[i] >> 16) & 0xF;
            break;
        case CFGTOKEN_VIDEO7:
            video7_enabled = (ptr[i] >> 16) & 0x1;
            break;
        case CFGTOKEN_RGBCOLOR:
            rgbpalette[(ptr[i] >> 16) & 0xF] = ptr[i+1];
            break;
        }

        // Advance by the number of dwords for this token
        i += 1 + (((ptr[i] >> 24) + 3) >> 2);
    }

    return i*4;
}

int build_config(uint32_t rev) {
    int j, i = 0;
    uint32_t *ptr = (uint32_t*)blockbuffer;

    memset(blockbuffer, 0xFF, sizeof(blockbuffer));

    ptr[i++] = NEWCONFIG_MAGIC;
    ptr[i++] = CFGTOKEN_REVISION | ((rev & 0xff) << 16);

    if (default_font >= FONT_COUNT)
	    default_font = 0;
    ptr[i++] = CFGTOKEN_FONT_00 | ((((uint32_t)default_font) & 0x3F) << 16);
    if(mono_palette) {
        ptr[i++] = CFGTOKEN_MONO_80 | ((((uint32_t)mono_palette-1) & 0x7) << 20);
    } else {
        ptr[i++] = CFGTOKEN_MONO_00;
    }
    ptr[i++] = CFGTOKEN_TBCOLOR | ((((uint32_t)terminal_fgcolor) & 0xF) << 20) | ((((uint32_t)terminal_bgcolor) & 0xF) << 16);
    ptr[i++] = CFGTOKEN_BORDER | ((((uint32_t)terminal_border) & 0xF) << 16);

    ptr[i++] = CFGTOKEN_VIDEO7 | ((((uint32_t)video7_enabled) & 0x1) << 16);
    for(j = 0; j < 16; j++) {
        ptr[i++] = CFGTOKEN_RGBCOLOR | 0x02000000 | (((uint32_t)j) << 16);
        ptr[i++] = rgbpalette[j];
    }

    switch(machine) {
    default:
    case MACHINE_AUTO:
        ptr[i++] = CFGTOKEN_HOST_AUTO;
        break;
    case MACHINE_APPLE_II:
        ptr[i++] = CFGTOKEN_HOST_II;
        break;
    case MACHINE_APPLE_IIE:
        ptr[i++] = CFGTOKEN_HOST_IIE;
        break;
    case MACHINE_APPLE_IIGS:
        ptr[i++] = CFGTOKEN_HOST_IIGS;
        break;
#if 0
    case MACHINE_PRAVETZ:
        ptr[i++] = CFGTOKEN_HOST_PRAVETZ;
        break;
    case MACHINE_BASIS:
        ptr[i++] = CFGTOKEN_HOST_BASIS;
        break;
#endif
    }

    switch(serialmux[0]) {
    case SERIAL_USB:
        ptr[i++] = CFGTOKEN_MUX_USB;
        break;
    case SERIAL_WIFI:
        ptr[i++] = CFGTOKEN_MUX_WIFI;
        break;
    case SERIAL_PRINTER:
        ptr[i++] = CFGTOKEN_MUX_PRN;
        break;
    default:
    case SERIAL_LOOP:
        ptr[i++] = CFGTOKEN_MUX_LOOP;
        break;
    }
    ptr[i++] = CFGTOKEN_SER_BAUD;
    ptr[i++] = baud[0];

    switch(serialmux[1]) {
    case SERIAL_USB:
        ptr[i++] = CFGTOKEN_MUX_USB | 0x010000;
        break;
    case SERIAL_WIFI:
        ptr[i++] = CFGTOKEN_MUX_WIFI | 0x010000;
        break;
    case SERIAL_PRINTER:
        ptr[i++] = CFGTOKEN_MUX_PRN | 0x010000;
        break;
    default:
    case SERIAL_LOOP:
        ptr[i++] = CFGTOKEN_MUX_LOOP | 0x010000;
        break;
    }
    ptr[i++] = CFGTOKEN_SER_BAUD | 0x010000;
    ptr[i++] = baud[1];


    switch(usbmux) {
    case USB_HOST_CDC:
        ptr[i++] = CFGTOKEN_USB_HOST;
        break;
    default:
    case USB_GUEST_CDC:
        ptr[i++] = CFGTOKEN_USB_GUEST;
        break;
    case USB_GUEST_MIDI:
        ptr[i++] = CFGTOKEN_USB_MIDI;
        break;
    }

    switch(wifimode) {
    case WIFI_AP:
        ptr[i++] = CFGTOKEN_WIFI_AP;
        break;
    case WIFI_CLIENT:
        ptr[i++] = CFGTOKEN_WIFI_CL;
        break;
    }

    ptr[i++] = CFGTOKEN_WIFI_SSID | (((uint32_t)strlen(wifi_ssid)+1) << 24);
    strncpy((char*)(&ptr[i]), wifi_ssid, longmin(strlen(wifi_ssid)+1,31));
    i += (strlen(wifi_ssid)+4) >> 2;

    ptr[i++] = CFGTOKEN_WIFI_PSK | (((uint32_t)strlen(wifi_psk)+1) << 24);
    strncpy((char*)(&ptr[i]), wifi_psk, longmin(strlen(wifi_psk)+1,31));
    i += (strlen(wifi_psk)+4) >> 2;

    ptr[i++] = CFGTOKEN_WIFI_IP;
    ptr[i++] = wifi_address;

    ptr[i++] = CFGTOKEN_WIFI_NM;
    ptr[i++] = wifi_netmask;

    ptr[i++] = CFGTOKEN_JD_HOST | (((uint32_t)strlen(jd_host)+1) << 24);
    strncpy((char*)(&ptr[i]), jd_host, longmin(strlen(jd_host)+1,31));
    ptr += (strlen(jd_host)+4) >> 2;

    ptr[i++] = CFGTOKEN_JD_PORT;
    ptr[i++] = jd_port;

    ptr[i++] = NEWCONFIG_EOF_MARKER;

    return i*4;
}

void cfgfile_upload(char *pdfile, uint16_t block) {
    FILE *f;
    size_t bytesread;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Writing file,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");
    
    f = fopen(pdfile, "rb");
    if(f == NULL) {
        backdrop(PROGNAME);
        window(" Error ", longmax(28, strlen(pdfile)+2), 7, 1);
        gotoy(11); gotox(7);
        cputs("Unable to open prodos file");
        gotoy(12); gotox(20 - (strlen(pdfile)/2));
        cputs(pdfile);
        ok_button();

        return;
    }

    memset(blockbuffer, 0xff, sizeof(blockbuffer));
    bytesread = fread(blockbuffer, 1, sizeof(blockbuffer), f);
    if(bytesread == 0) {
        backdrop(PROGNAME);
        window(" Error ", longmax(28, strlen(pdfile)+2), 7, 1);
        gotoy(11); gotox(7);
        cputs("Unable to read prodos file");
        gotoy(12); gotox(20 - (strlen(pdfile)/2));
        cputs(pdfile);
        ok_button();

        goto cleanup;
    }

    parse_config();
    if (0 == cfg_update(block))
	    goto cleanup;

cleanup:
    if(f != NULL)
        fclose(f);
}

void cfgfile_download(char *pdfile, uint16_t block) {
    FILE *f;
    size_t byteswritten;
    int i;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Reading file,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    f = fopen(pdfile, "wb");
    if(f == NULL) {
        backdrop(PROGNAME);
        window(" Error ", longmax(28, strlen(pdfile)+2), 7, 1);
        gotoy(11); gotox(7);
        cputs("Unable to open prodos file");
        gotoy(12); gotox(20 - (strlen(pdfile)/2));
        cputs(pdfile);
        ok_button();

        return;
    }

    if(cfg_cmd1("fr", block)) {
        ok_button();

        error_window();
        cprintf("Unable to read block $%4X", block);
        ok_button();

        goto cleanup;
    }

    CF_PTRL = 0;
    CF_PTRH = 0;
    for(i = 0; i < sizeof(blockbuffer); i++) {
        blockbuffer[i] = CF_DATR;
    }

    byteswritten = fwrite(blockbuffer, 1, sizeof(blockbuffer), f);
    if(byteswritten != sizeof(blockbuffer)) {
        ok_button();

        backdrop(PROGNAME);
        window(" Error ", longmax(30, strlen(pdfile)+2), 7, 1);
        gotoy(11); gotox(6);
        cputs("Unable to write prodos file");
        gotoy(12); gotox(20 - (strlen(pdfile)/2));
        cputs(pdfile);
        ok_button();

        goto cleanup;
    }
    
cleanup:
    if(f != NULL)
        fclose(f);
}

void restore_config() {
    uint16_t last, next;

    // Get current config blocks
    if(cfg_cmd0("fc")) {
        error_window();
        cputs("Unable to get config block");
        ok_button();
        return;
    }

    next = RPY_BUFFER[5];
    next <<= 8;
    next |= RPY_BUFFER[4];

    last = RPY_BUFFER[3];
    last <<= 8;
    last |= RPY_BUFFER[2];

    cfgfile_upload("CONFIG.BACKUP", next);

cleanup:
    return;
}

void backup_config() {
    uint16_t last;

    // Get current config blocks
    if(cfg_cmd0("fc")) {
        error_window();
        cputs("Unable to get config block");
        ok_button();
        goto cleanup;
    }
    
    last = RPY_BUFFER[3];
    last <<= 8;
    last |= RPY_BUFFER[2];

    cfgfile_download("CONFIG.BACKUP", last);

cleanup:
    return;
}

void read_config() {
    int i;
    uint16_t last;
    
    default_config();

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(13);
    cputs("Reading file,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    // Get card hardware type
    if(cfg_cmd0("Ih")) {
        error_window();
        cputs("Unable to get hardware id");
        ok_button();
        goto cleanup;
    }
    if(RPY_BUFFER[1] != 0x02) {
        error_window();
        gotox(8);
        cputs("Unknown hardware vendor");
        ok_button();
        goto cleanup;
    }
    switch(RPY_BUFFER[2]) {
        case 'G':
            hardware_type = 'G';
            memcpy(rgbpalette, default_gs_palette, sizeof(rgbpalette));
            break;
        case 'L':
            hardware_type = 'L';
            memcpy(rgbpalette, default_lc_palette, sizeof(rgbpalette));
            break;
        case 'W':
            hardware_type = 'W';
            memcpy(rgbpalette, default_lc_palette, sizeof(rgbpalette));
            break;
    }
    if(hardware_type == 'G') {
        if(cfg_cmd0("Ij")) {
            error_window();
            gotox(9);
            cputs("Unable to get jumpers");
            ok_button();
            goto cleanup;
        }
        jumpers = RPY_BUFFER[1];
        if(jumpers & 1) {
                machine = MACHINE_APPLE_IIGS;
        }
    }

    // Get current config blocks
    if(cfg_cmd0("fc")) {
        error_window();
        cputs("Unable to get config block");
        ok_button();
        goto cleanup;
    }
    
    last = RPY_BUFFER[3];
    last <<= 8;
    last |= RPY_BUFFER[2];

    if(cfg_cmd1("fr", last)) {
        backdrop(PROGNAME);
        window(" Error ", 30, 7, 1);
        gotoy(11); gotox(7);
        cprintf("Unable to read block $%4X", last);
        ok_button();

        goto cleanup;
    }

    CF_PTRL = 0;
    CF_PTRH = 0;
    for(i = 0; i < sizeof(blockbuffer); i++) {
        blockbuffer[i] = CF_DATR;
    }

    parse_config();

cleanup:
    return;
}

int write_config() {
    uint16_t next;

    build_config(config_rev+1);

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(10);
    cputs("Saving configuration,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    // Get current config blocks
    if(cfg_cmd0("fc")) {
        error_window();
        cputs("Unable to get config block");
        ok_button();
        goto cleanup;
    }

	next = RPY_BUFFER[5];
	next <<= 8;
	next |= RPY_BUFFER[4];

    if (0 == cfg_update(next))
	    goto cleanup;
    return 1;

cleanup:
    return 0;
}

void apply_config(void) {
    timeout = CARD_TIMEOUT;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(10); gotox(13);
    cputs("Rebooting card");
    gotoy(11); gotox(9);
    cputs("to apply configuration,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    // Ask card to reboot.
    cfg_cmd0("Rb");

    while(timeout--);
}

int format_card(void) {
    uint16_t last, next;

    backdrop(PROGNAME);
    window(" Please Wait ", 26, 6, 1);
    gotoy(11); gotox(12);
    cputs("Erasing configs,");
    gotoy(12); gotox(8);
    cputs("your screen may flicker.");

    // Get current config blocks
    if(cfg_cmd0("fc")) {
        error_window();
        gotox(8);
        cputs("Unable to get config block");
        ok_button();

        goto cleanup;
    }
    
    next = RPY_BUFFER[5];
    next <<= 8;
    next |= RPY_BUFFER[4];

    last = RPY_BUFFER[3];
    last <<= 8;
    last |= RPY_BUFFER[2];
    
    if(last != next) {
        if(cfg_erase(last)) {
            goto cleanup;
        }
    }
    if(cfg_erase(next)) {
        goto cleanup;
    }

    return 1;
    
cleanup:
    return 0;
}

int baud_menu(char *screen_title, int baudrate) {
    int i, selected_item = 0;

    for(i = 0; i < (sizeof(baudvalue)/sizeof(uint16_t)); i++) {
        if(baudrate == baudvalue[i]) {
            selected_item = i;
        }
    }

    selected_item = list_menu(screen_title, " Baud Rate ", baud_strings, (sizeof(baud_strings)/sizeof(menu_strings_t))-1, selected_item, selected_item);

    return baudvalue[selected_item];
}

static char tolower(char c) {
    if((c >= 'A') && (c <= 'Z')) {
        return (c-'A')+'a';
    }
    return c;
}

void jdhost_edit(void) {
    char temp_host[32];
    int c, l;
    int paint_menu = 1;
    int y = 12 - 1;
    int x = 7;

    strcpy(temp_host, jd_host);
    l = strlen(temp_host);

    for(;;) {
        if(paint_menu) {
            backdrop("JetDirect");
            window(" Server ", 30, 5, 1);
            gotoy(y+0); gotox(x);
            cputs("Enter hostname or IP:");
            gotoy(y+2); gotox(x);
            cputs("_________________________");
            gotoy(y+2); gotox(x);
            cputs(temp_host);
            paint_menu = 0;
        }

        gotox(x+l);
        cputc(CHAR_CURSOR);
        c = cgetc();
        if(c == 0x08) {
            if(l > 0) {
                // Remove flashing cursor
                gotox(x+l);
                cputc('_');

                // Remove deleted character
                l--;
                gotox(x+l);
                cputc('_');
                temp_host[l] = 0;
            }
        } else if(c == 0x0D) {
            strcpy(jd_host, temp_host);
            return;
        } else if(c == 0x1B) {
            if(confirm(" Are you sure? ", "Go back without saving?")) {
                return;
            }
            paint_menu = 1;
        } else if(((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9')) || (c == '.')) {
            if(l < 24) {
                gotox(x+l);
                cputc(tolower(c));
                temp_host[l] = tolower(c);
                l++;
                temp_host[l] = 0;
            }
        }
    }
}

void jdport_edit(void) {
    char temp_port[8];
    int c, l;
    int paint_menu = 1;
    int y = 12 - 1;
    int x = 15;

    sprintf(temp_port, "%i", jd_port);
    l = strlen(temp_port);

    for(;;) {
        if(paint_menu) {
            backdrop("JetDirect");
            window(" TCP/IP Port ", 16, 5, 1);
            gotoy(y+0); gotox(x);
            cputs("Enter port:");
            gotoy(y+2); gotox(x);
            cputs("______");
            gotoy(y+2); gotox(x);
            cputs(temp_port);
            paint_menu = 0;
        }

        gotox(x+l);
        cputc(CHAR_CURSOR);
        c = cgetc();
        if(c == 0x08) {
            if(l > 0) {
                // Remove flashing cursor
                gotox(x+l);
                cputc('_');

                // Remove deleted character
                l--;
                gotox(x+l);
                cputc('_');
                temp_port[l] = 0;
            }
        } else if(c == 0x0D) {
            jd_port = strtoul(temp_port, NULL, 0);
            return;
        } else if(c == 0x1B) {
            gotox(x+l);
            cputc('_');
            if(confirm(" Are you sure? ", "Go back without saving?")) {
                return;
            }
            paint_menu = 1;
        } else if((c >= '0') && (c <= '9')) {
            if(l < 5) {
                gotox(x+l);
                cputc(c);
                temp_port[l] = c;
                l++;
                temp_port[l] = 0;
            }
        }
    }
}

void ssid_edit(void) {
    char temp_ssid[32];
    int c, l;
    int paint_menu = 1;
    int y = 12 - 1;
    int x = 7;

    strcpy(temp_ssid, wifi_ssid);
    l = strlen(temp_ssid);

    for(;;) {
        if(paint_menu) {
            backdrop("WiFi");
            window(" Network Name (SSID) ", 30, 5, 1);
            gotoy(y+0); gotox(x);
            cputs("Enter network name:");
            gotoy(y+2); gotox(x);
            cputs("_________________________");
            gotoy(y+2); gotox(x);
            cputs(temp_ssid);
            paint_menu = 0;
        }

        gotox(x+l);
        cputc(CHAR_CURSOR);
        c = cgetc();
        if(c == 0x08) {
            if(l > 0) {
                // Remove flashing cursor
                gotox(x+l);
                cputc('_');

                // Remove deleted character
                l--;
                gotox(x+l);
                cputc('_');
                temp_ssid[l] = 0;
            }
        } else if(c == 0x0D) {
            strcpy(wifi_ssid, temp_ssid);
            return;
        } else if(c == 0x1B) {
            gotox(x+l);
            cputc('_');
            if(confirm(" Are you sure? ", "Go back without saving?")) {
                return;
            }
            paint_menu = 1;
        } else if(c >= 32) {
            if(l < 24) {
                gotox(x+l);
                cputc(c);
                temp_ssid[l] = c;
                l++;
                temp_ssid[l] = 0;
            }
        }
    }
}

void psk_edit(void) {
    char temp_psk[32];
    int c, l;
    int paint_menu = 1;
    int y = 12 - 1;
    int x = 7;

    strcpy(temp_psk, wifi_psk);
    l = strlen(temp_psk);

    for(;;) {
        if(paint_menu) {
            backdrop("WiFi");
            window(" Passphrase (PSK) ", 30, 5, 1);
            gotoy(y+0); gotox(x);
            cputs("Enter passphrase:");
            gotoy(y+1); gotox(x);
            cputs("_________________________");
            gotoy(y+1); gotox(x);
            cputs(temp_psk);
            paint_menu = 0;
        }

        gotox(x+l);
        cputc(CHAR_CURSOR);
        c = cgetc();
        if(c == 0x08) {
            if(l > 0) {
                // Remove flashing cursor
                gotox(x+l);
                cputc('_');

                // Remove deleted character
                l--;
                gotox(x+l);
                cputc('_');
                temp_psk[l] = 0;
            }
        } else if(c == 0x0D) {
            strcpy(wifi_psk, temp_psk);
            return;
        } else if(c == 0x1B) {
            gotox(x+l);
            cputc('_');
            if(confirm(" Are you sure? ", "Go back without saving?")) {
                return;
            }
            paint_menu = 1;
        } else if(c >= 32) {
            if(l < 24) {
                gotox(x+l);
                cputc(c);
                temp_psk[l] = c;
                l++;
                temp_psk[l] = 0;
            }
        }
    }
}

int valid_netmask(uint32_t ip) {
    int i;
    uint32_t first_one = 32;
    uint32_t bit_test = 1;
    
    if((ip & 0x80000000) == 0) return 0;

    for(i = 0; i < 31; i++) {
        if(ip & bit_test) { // Bit is set
            if(i < first_one) {
                first_one = i;
            }
        } else { // Bit is clear
            if(i > first_one) { // But we've seen a 1 already
                return 0;
            }
        }
        bit_test <<= 1;
    }

    return 1;
}

int valid_ip_or_dhcp(uint32_t ip) {
    if(ip == 0) return 1;

    if((ip & 0xFF000000) == 0) return 0;
    if(ip == wifi_netmask) return 0;

    if((ip & ~wifi_netmask) == 0) return 0;
    if((ip & ~wifi_netmask) == ~wifi_netmask) return 0;

    return 1;
}

// Parse a decimal dotted quad segment
uint16_t parse_dq(char *ptr, char **ptr_out) {
    uint16_t rv = 0;
    while((*ptr != 0) && (*ptr != '.')) {
        if((*ptr >= '0') && (*ptr <= '9')) {
            rv *= 10;
            rv += *ptr - '0';
        } else {
            *ptr_out = ptr;
            return 0xFFFF;
        }
        ptr++;
    }

    if(*ptr == 0) {
        *ptr_out = NULL;
    } else {
        *ptr_out = ptr;
    }

    return rv;
}

uint32_t address_edit(uint32_t ip, int is_netmask) {
    char temp_address[32];
    int c, l;
    int paint_menu = 1;
    uint16_t temp_ip[4];
    int y = 12 - 1;
    int x = is_netmask ? 12 : 5;
    
    temp_ip[0] = (ip >> 24) & 0xff;
    temp_ip[1] = (ip >> 16) & 0xff;
    temp_ip[2] = (ip >> 8) & 0xff;
    temp_ip[3] = (ip >> 0) & 0xff;

    sprintf(temp_address, "%d.%d.%d.%d", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3]);
    l = strlen(temp_address);

    for(;;) {
        if(paint_menu) {
            backdrop("WiFi");
            window(is_netmask ? " Netmask " : " IP Address ", is_netmask ? 20 : 32, 5, 1);
            if(is_netmask) {
                gotoy(y+0); gotox(x);
                cputs("Enter netmask:");
            } else {
                gotoy(y+0); gotox(x);
                cputs("Enter IP or 0.0.0.0 for DHCP:");
            }
            gotoy(y+2); gotox(x);
            cputs("________________");
            gotoy(y+2); gotox(x);
            cputs(temp_address);
            paint_menu = 0;
        }

        gotox(x+l);
        cputc(CHAR_CURSOR);
        c = cgetc();
        if(c == 0x08) {
            if(l > 0) {
                // Remove flashing cursor
                gotox(x+l);
                cputc('_');

                // Remove deleted character
                l--;
                gotox(x+l);
                cputc('_');
                temp_address[l] = 0;
            }
        } else if((c == 0x0A) || (c == 0x0D)) {
            char *ptr = temp_address;
            temp_ip[0] = parse_dq(ptr, &ptr);
            if((ptr != NULL) && (*ptr == '.')) {
                ptr++;
                temp_ip[1] = parse_dq(ptr, &ptr);
            }
            if((ptr != NULL) && (*ptr == '.')) {
                ptr++;
                temp_ip[2] = parse_dq(ptr, &ptr);
            }
            if((ptr != NULL) && (*ptr == '.')) {
                ptr++;
                temp_ip[3] = parse_dq(ptr, &ptr);
                if((temp_ip[0] < 256) && (temp_ip[1] < 256) &&
                  (temp_ip[2] < 256) && (temp_ip[3] < 256)) {
                    ip = temp_ip[0];
                    ip <<= 8;
                    ip |= temp_ip[1];
                    ip <<= 8;
                    ip |= temp_ip[2];
                    ip <<= 8;
                    ip |= temp_ip[3];
                    if((is_netmask && valid_netmask(ip)) || (!is_netmask && valid_ip_or_dhcp(ip))) 
                        return ip;
                }
            }
            message(" Error ", "Invalid Address Specified");
            paint_menu = 1;
        } else if(c == 0x1B) {
            backdrop("WiFi");
            if(confirm(" Are you sure? ", "Go back without saving?")) {
                return ip;
            }
            paint_menu = 2;
        } else if(c >= 32) {
            if(l < 15) {
                gotox(x+l);
                cputc(c);
                temp_address[l] = c;
                l++;
                temp_address[l] = 0;
            }
        }
    }
}

void color_editor(int palette_entry) {
    int y = 12 - 1;
    int selected_item = 1;
    int paint_menu = 2;
    int maxval = (hardware_type == 'G') ? 15 : 7;
    int rgb[3];

    rgb[0] = ((hardware_type == 'G') ? ((rgbpalette[palette_entry] >> 8) & 0xf) : ((rgbpalette[palette_entry] >> 6) & 0x7));
    rgb[1] = ((hardware_type == 'G') ? ((rgbpalette[palette_entry] >> 4) & 0xf) : ((rgbpalette[palette_entry] >> 3) & 0x7));
    rgb[2] = ((hardware_type == 'G') ? ((rgbpalette[palette_entry] >> 0) & 0xf) : ((rgbpalette[palette_entry] >> 0) & 0x7));

    VGA_TBCOLOR = 0xF0;
    VGA_BORDER = palette_entry;
    VGA_MONOMODE = 0xF0;

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop("Palette Editor");
            window(color_strings[palette_entry].str, 32, 5, 1);
            gotoy(y+2); gotox(11);
            cputs("Press [CR] to save");
        }
        if(paint_menu) {
            paint_menu = 0;
            gotoy(y+0); gotox(10);
            print_menu_select_int("%1x", rgb[0], 6, (selected_item == 0), 0);
            gotoy(y+0); gotox(17);
            print_menu_select_int("%1x", rgb[1], 6, (selected_item == 1), 0);
            gotoy(y+0); gotox(24);
            print_menu_select_int("%1x", rgb[2], 6, (selected_item == 2), 0);
        }
        switch(cgetc()) {
            case 0x08:
                if(selected_item > 0) {
                    selected_item--;
                    paint_menu = 1;
                }
                break;
            case 0x15:
                if(selected_item < 2) {
                    selected_item++;
                    paint_menu = 1;
                }
                break;
            case '+':
            case 0x0B:
                if(rgb[selected_item] < maxval) {
                    rgb[selected_item]++;
                    CARD_REGISTER(0xC) = (palette_entry << 4) | (selected_item+1);
                    CARD_REGISTER(0xD) = rgb[selected_item];
                    paint_menu = 1;
                }
                break;
            case '-':
            case 0x0A:
                if(rgb[selected_item] > 0) {
                    rgb[selected_item]--;
                    CARD_REGISTER(0xC) = (palette_entry << 4) | (selected_item+1);
                    CARD_REGISTER(0xD) = rgb[selected_item];
                    paint_menu = 1;
                }
                break;
            case 0x0D:
                if(hardware_type == 'G') {
                    rgbpalette[palette_entry] = ((rgb[0] & 0xf) << 8) | ((rgb[1] & 0xf) << 4) | ((rgb[2] & 0xf) << 0);
                } else {
                    rgbpalette[palette_entry] = ((rgb[0] & 0x7) << 6) | ((rgb[1] & 0x7) << 3) | ((rgb[2] & 0x7) << 0);
                }
                paint_menu = -1;
                break;
            case 0x1B:
                paint_menu = -1;
        }
    }

    VGA_TBCOLOR = (terminal_fgcolor << 4) | terminal_bgcolor;
    VGA_BORDER = terminal_border;
    VGA_MONOMODE = (mono_palette << 4);
}

int video_menu_action(int action) {
    int edit_color = -1;
    switch(action) {
        case 0:
            default_font = list_menu("Video Settings", " Default Font ", font_strings, FONT_COUNT-1, default_font, default_font);
            return 2;
        case 1:
            mono_palette = list_menu("Video Settings", " Monochrome Mode ", monochrome_strings, (sizeof(monochrome_strings)/sizeof(menu_strings_t))-1, mono_palette, mono_palette);
            return 2;
        case 2:
            terminal_fgcolor = list_menu("Video Settings", " Foreground Color ", color_strings, (sizeof(color_strings)/sizeof(menu_strings_t))-1, terminal_fgcolor, terminal_fgcolor);
            return 2;
        case 3:
            terminal_bgcolor = list_menu("Video Settings", " Background Color ", color_strings, (sizeof(color_strings)/sizeof(menu_strings_t))-1, terminal_bgcolor, terminal_bgcolor);
            return 2;
        case 4:
            terminal_border = list_menu("Video Settings", " Border Color ", color_strings, (sizeof(color_strings)/sizeof(menu_strings_t))-1, terminal_border, terminal_border);
            return 2;
        case 5:
            video7_enabled = !video7_enabled;
            return 1;
        case 6:
            edit_color = 0;
            do {
                edit_color = list_menu("Video Settings", " Edit Palette ", color_strings, (sizeof(color_strings)/sizeof(menu_strings_t))-1, edit_color, -1);
                if(edit_color >= 0) {
                    color_editor(edit_color);
                }
            } while(edit_color != -1);
            return 2;
        case 7:
            return -1;
    }
}

void video_menu(void) {
    int paint_menu = 2;
    int selected_item = 0;
    int y = 12 - 7;

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop("Video Settings");
            window(" Video Settings ", 26, 17, 0);
        }
        if(paint_menu > 0) {
            gotoy(y+0); gotox(8);
            print_menu_select_int("Default Font: %6x", default_font, 24, (selected_item == 0), 0);
            gotoy(y+2); gotox(8);
            print_menu_select_str("Monochrome: %8s", monochrome_strings[mono_palette].str, 24, (selected_item == 1), 0);
            gotoy(y+4); gotox(8);
            print_menu_select_str("Foreground: %8s", color_strings[terminal_fgcolor].str, 24, (selected_item == 2), 0);
            gotoy(y+6); gotox(8);
            print_menu_select_str("Background: %8s", color_strings[terminal_bgcolor].str, 24, (selected_item == 3), 0);
            gotoy(y+8); gotox(8);
            print_menu_select_str("Border: %12s", color_strings[terminal_border & 0xf].str, 24, (selected_item == 4), 0);
            gotoy(y+10); gotox(8);
            print_menu_select_str("Video 7:    %8s", video7_enabled ? " Enabled" : "Disabled", 24, (selected_item == 5), 0);
            gotoy(y+12); gotox(8);
            print_menu_select("Edit Palette", 24, (selected_item == 6), 0);

            gotoy(y+14); gotox(8);
            print_menu_select("Back to Main Menu", 24, (selected_item == 7), 0);

            paint_menu = 0;
        }
        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_item > 0) {
                    selected_item--;
                    paint_menu = 1;
                }
                break;
            case 0x15:
            case 0x0A:
                if(selected_item < 7) {
                    selected_item++;
                    paint_menu = 1;
                }
                break;
            case 0x0D:
                paint_menu = video_menu_action(selected_item);
                break;
            case 0x1B:
                paint_menu = -1;
        }
    }
}

int wifi_menu_action(int action) {
    switch(action) {
        case 0:
            wifimode = (wifimode == WIFI_AP) ? WIFI_CLIENT : WIFI_AP;
            return 1;
        case 1:
            ssid_edit();
            return 2;
        case 2:
            psk_edit();
            return 2;
        case 3:
            if(wifimode != WIFI_AP) {
                wifi_address = address_edit(wifi_address, 0);
                return 2;
            }
            return 0;
        case 4:
            if(wifimode != WIFI_AP) {
                wifi_netmask = address_edit(wifi_netmask, 1);
                return 2;
            }
            return 0;
        case 5:
            return -1;
    }
}

void wifi_menu(void) {
    int paint_menu = 2;
    int selected_item = 0;
    int y = 12 - 7;

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop(PROGNAME);
            window(" WiFi Options ", 26, 17, 0);
        }
        if(paint_menu > 0) {
            gotoy(y+0); gotox(8);
            print_menu_select_str("Mode: %14s", wifi_mode_str[wifimode == WIFI_CLIENT].str, 24, (selected_item == 0), 0);

            gotoy(y+2); gotox(8);
            print_menu_select("Network Name:", 24, (selected_item == 1), 0);
            gotoy(y+3); gotox(8);
            print_menu_select_str("%20s", wifi_ssid, 24, (selected_item == 1), 1);

            gotoy(y+5); gotox(8);
            print_menu_select("Pre-Shared Key:", 24, (selected_item == 2), 0);
            gotoy(y+6); gotox(8);
            print_menu_select_str("%20s", "*****", 24, (selected_item == 2), 1);

            gotoy(y+8); gotox(8);
            print_menu_select("IP Address:", 24, (selected_item == 3), 0);
            gotoy(y+9); gotox(8);
            if(wifimode == WIFI_AP) {
                print_menu_select_str("%20s", "10.65.50.1", 24, (selected_item == 3), 1);
            } else if(wifi_address == 0) {
                print_menu_select_str("%20s", "DHCP", 24, (selected_item == 3), 1);
            } else {
                print_menu_select_ip(wifi_address, 24, (selected_item == 3), 1);
            }

            gotoy(y+11); gotox(8);
            print_menu_select("Netmask:", 24, (selected_item == 4), 0);
            gotoy(y+12); gotox(8);
            if(wifimode == WIFI_AP) {
                print_menu_select_str("%20s", "255.255.255.0", 24, (selected_item == 4), 1);
            } else {
                print_menu_select_ip(wifi_netmask, 24, (selected_item == 4), 1);
            }

            gotoy(y+14); gotox(8);
            print_menu_select("Back to Main Menu", 24, (selected_item == 5), 0);

            paint_menu = 0;
        }

        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_item > 0) {
                    selected_item--;
                    paint_menu = 1;
                }
                break;
            case 0x15:
            case 0x0A:
                if(selected_item < 5) {
                    selected_item++;
                    paint_menu = 1;
                }
                break;
            case 0x0D:
                paint_menu = wifi_menu_action(selected_item);
                break;
            case 0x1B:
                paint_menu = -1;
        }
    }
}

int io_menu_action(int action) {
    switch(action) {
        case 0:
            serialmux[0] = list_menu("Serial Port 0", " Port Mux ", serialmux_strings, (sizeof(serialmux_strings)/sizeof(menu_strings_t))-1, serialmux[0], serialmux[0]);
            return 2;
        case 1:
            baud[0] = baud_menu("Serial Port 0", baud[0]);
            return 2;
        case 2:
            serialmux[1] = list_menu("Serial Port 1", " Port Mux ", serialmux_strings, (sizeof(serialmux_strings)/sizeof(menu_strings_t))-1, serialmux[1], serialmux[1]);
            return 2;
        case 3:
            baud[1] = baud_menu("Serial Port 1", baud[1]);
            return 2;
        case 4:
            jdhost_edit();
            return 2;
        case 5:
            jdport_edit();
            return 2;
        case 6:
            return -1;
    }
}

void io_menu(void) {
    int paint_menu = 2;
    int selected_item = 0;
    int y = 12 - 7;

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop(PROGNAME);
            window(" I/O Settings ", 26, 17, 0);
        }
        if(paint_menu > 0) {
            gotoy(y+0); gotox(8);
            print_menu_select_str("Serial 0: %10s", serialmux_strings[serialmux[0]].str, 24, (selected_item == 0), 0);
            gotoy(y+1); gotox(8);
            print_menu_select_int(" Baud: %13d", baud[0], 24, (selected_item == 1), 0);

            gotoy(y+3); gotox(8);
            print_menu_select_str("Serial 1: %10s", serialmux_strings[serialmux[1]].str, 24, (selected_item == 2), 0);
            gotoy(y+4); gotox(8);
            print_menu_select_int(" Baud: %13d", baud[1], 24, (selected_item == 3), 0);

            gotoy(y+6); gotox(8);
            print_menu_select("JetDirect Server:", 24, (selected_item == 4), 0);
            gotoy(y+7); gotox(8);
            print_menu_select_str("%20s", jd_host, 24, (selected_item == 4), 1);

            gotoy(y+9); gotox(8);
            print_menu_select("JetDirect Port:", 24, (selected_item == 5), 0);
            gotoy(y+10); gotox(8);
            print_menu_select_int("%20d", jd_port, 24, (selected_item == 5), 1);

            gotoy(y+14); gotox(8);
            print_menu_select("Back to Main Menu", 24, (selected_item == 6), 0);

            paint_menu = 0;
        }

        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_item > 0) {
                    selected_item--;
                    paint_menu = 1;
                }
                break;
            case 0x15:
            case 0x0A:
                if(selected_item < 6) {
                    selected_item++;
                    paint_menu = 1;
                }
                break;
            case 0x0D:
                paint_menu = io_menu_action(selected_item);
                break;
            case 0x1B:
                paint_menu = -1;
        }
    }
}

int main_menu_action(int action) {
    switch(action) {
        case 0:
            machine = list_menu(PROGNAME, " Host Type ", machine_strings, (sizeof(machine_strings)/sizeof(menu_strings_t))-1, machine, machine);
            return 2;
        case 1:
            video_menu();
            return 2;
        case 2:
            io_menu();
            return 2;
        case 3:
            wifi_menu();
            return 2;
        case 4:
            backup_config();
            return 2;
        case 5:
            restore_config();
            apply_config();
            return 2;
        case 6:
            backdrop(PROGNAME);
            if(confirm(" Are you sure? ", "Format Card?")) {
                format_card();
                default_config();
                return 2;
            }
            return 2;
        case 7:
            backdrop(PROGNAME);
            if(confirm(" Are you sure? ", "Quit without saving?")) {
                clrscr();
                exec("MENU.SYSTEM", "");
                return -1;
            }
            return 2;
        case 8:
            backdrop(PROGNAME);
            if(confirm(" Are you sure? ", "Save and exit?")) {
                write_config();
                apply_config();
                clrscr();
                exec("MENU.SYSTEM", "");
                return -1;
            }
            return 2;
   }
}

void main (void) {
    int paint_menu = 2;
    int selected_item = 0;
    int y = 12 - 6;

    if(!prompt_slot(PROGNAME)) {
        goto cleanup;
    }

    switch(get_ostype() & 0xF0) {
        default:
        case 0x10:
            machine = MACHINE_APPLE_II;
            break;
        case 0x30:
            machine = MACHINE_APPLE_IIE;
            break;
        case 0x80:
            machine = MACHINE_APPLE_IIGS;
            break;
    }

    read_config();

    while(paint_menu >= 0) {
        if(paint_menu == 2) {
            backdrop(PROGNAME);

            window(" Main Menu ", 26, 15, 0);
            gotoy(y+4); gotox(7);
            repeatchar(CHAR_SEPARATOR, 26);
            gotoy(y+9); gotox(7);
            repeatchar(CHAR_SEPARATOR, 26);
        }
        if(paint_menu) {
            gotoy(y+0); gotox(8);
            if(machine < MACHINE_MAXVALUE) {
                print_menu_select_str("Machine: %11s", machine_strings[machine].str, 24, (selected_item == 0), 0);
            } else {
                print_menu_select("Machine:     Unknown", 24, (selected_item == 0), 0);
            }
            gotoy(y+1); gotox(8);
            print_menu_select("Video Settings", 24, (selected_item == 1), 0);
            gotoy(y+2); gotox(8);
            print_menu_select("I/O Settings", 24, (selected_item == 2), 0);
            gotoy(y+3); gotox(8);
            print_menu_select("WiFi Settings", 24, (selected_item == 3), 0);

            gotoy(y+6); gotox(8);
            print_menu_select("Backup config file", 24, (selected_item == 4), 0);
            gotoy(y+7); gotox(8);
            print_menu_select("Restore config file", 24, (selected_item == 5), 0);
            gotoy(y+8); gotox(8);
            print_menu_select("Erase config files", 24, (selected_item == 6), 0);

            gotoy(y+11); gotox(8);
            print_menu_select("Quit without saving", 24, (selected_item == 7), 0);
            gotoy(y+12); gotox(8);
            print_menu_select("Save config & Exit", 24, (selected_item == 8), 0);
            paint_menu = 0;
        }

        switch(cgetc()) {
            case 0x08:
            case 0x0B:
                if(selected_item > 0) {
                    selected_item--;
                    paint_menu = 1;
                }
                break;
            case 0x15:
            case 0x0A:
                if(selected_item < 8) {
                    selected_item++;
                    paint_menu = 1;
                }
                break;
            case 'M':
            case 'm':
                paint_menu = main_menu_action(selected_item = 0);
                break;
            case 'V':
            case 'v':
                paint_menu = main_menu_action(selected_item = 1);
                break;
            case 'W':
            case 'w':
                paint_menu = main_menu_action(selected_item = 2);
                break;
            case 'I':
            case 'i':
                paint_menu = main_menu_action(selected_item = 3);
                break;
            case 'B':
            case 'b':
                paint_menu = main_menu_action(selected_item = 4);
                break;
            case 'R':
            case 'r':
                paint_menu = main_menu_action(selected_item = 5);
                break;
            case 'E':
            case 'e':
                paint_menu = main_menu_action(selected_item = 6);
                break;
            case 0x1B:
            case 'Q':
            case 'q':
                paint_menu = main_menu_action(selected_item = 7);
                break;
            case 'S':
            case 's':
                paint_menu = main_menu_action(selected_item = 8);
                break;
            case 0x0D:
                paint_menu = main_menu_action(selected_item);
                break;
        }
    }

cleanup:
    backdrop(PROGNAME);
    gotoy(12); gotox(13);
    cputs("Launching Menu");

    exec("MENU.SYSTEM", "");
}
