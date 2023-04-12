#ifndef __V2ANALOG_H
#define __V2ANALOG_H

#define CARD_REGISTER(n) (*(volatile unsigned char *)(0xC080 | (n) | (cardslot << 4)))

#define VGA_MONOMODE  CARD_REGISTER(0x1)
#define VGA_TBCOLOR   CARD_REGISTER(0x2)
#define VGA_BORDER    CARD_REGISTER(0x3)

#define CF_PTRL *((volatile unsigned char *)(0xC0ED | (cardslot << 8)))
#define CF_PTRH *((volatile unsigned char *)(0xC0EE | (cardslot << 8)))
#define CF_DATA *((volatile unsigned char *)(0xC0EF | (cardslot << 8)))

#define CMD_BUFFER ((volatile unsigned char *)(0xC0F0 | (cardslot << 8)))
#define RPY_BUFFER ((volatile unsigned char *)(0xC0F8 | (cardslot << 8)))

#define REPLY_OK      0x00
#define REPLY_BUSY    0xBB
#define REPLY_EOF     0x01
#define REPLY_NOFILE  0x02
#define REPLY_EPARAM  0x03
#define REPLY_ECMD    0x04

typedef enum {
    MODE_REBOOT = 0,
    MODE_DIAG,
    MODE_FS,
    MODE_VGACARD,
    MODE_APPLICARD,
    MODE_SERIAL,
    MODE_PARALLEL,
    MODE_MIDI,
    MODE_SNESMAX,
    MODE_ETHERNET
} v2mode_t;

typedef enum {
    SERIAL_LOOP = 0,
    SERIAL_USB,
    SERIAL_WIFI,
    SERIAL_PRINTER,
} serialmux_t;

typedef enum {
    USB_HOST_CDC,
    USB_GUEST_CDC,
    USB_GUEST_MIDI,
} usbmux_t;

typedef enum {
    WIFI_CLIENT = 0,
    WIFI_AP,
} wifimode_t;

#define FS_COMMAND        CARD_REGISTER(0x0)
#define FS_FILE           CARD_REGISTER(0x1)
#define FS_FLAGS          CARD_REGISTER(0x1)
#define FS_SIZELSB        CARD_REGISTER(0x2)
#define FS_SIZEMSB        CARD_REGISTER(0x3)
#define FS_OFFLSB         CARD_REGISTER(0x2)
#define FS_OFFMSB         CARD_REGISTER(0x3)
#define FS_WHENCE         CARD_REGISTER(0x4)
#define FS_BUSY           CARD_REGISTER(0xD)
#define FS_STATUS         CARD_REGISTER(0xE)
#define FS_EXECUTE        CARD_REGISTER(0xF)

#define FS_O_RDONLY    0x0001
#define FS_O_WRONLY    0x0002
#define FS_O_RDWR      0x0003
#define FS_O_CREAT     0x0100
#define FS_O_EXCL      0x0200
#define FS_O_TRUNC     0x0400
#define FS_O_APPEND    0x0800

#define FS_SEEK_SET    0
#define FS_SEEK_CUR    1
#define FS_SEEK_END    2

typedef enum {
    FS_OPEN  = 0x10,
    FS_CLOSE = 0x11,
    FS_READ  = 0x12,
    FS_WRITE = 0x13,
    FS_SEEK  = 0x14,
    FS_TELL  = 0x15,
} fscommand_t;

typedef enum {
    FS_ERR_OK = 0,           // No error
    FS_ERR_IO = -1,          // Error during device operation
    FS_ERR_CORRUPT = -2,     // Corrupted
    FS_ERR_NOENT = -3,       // No directory entry
    FS_ERR_EXIST = -4,       // Entry already exists
    FS_ERR_NOTDIR = -5,      // Entry is not a dir
    FS_ERR_ISDIR = -5,       // Entry is a dir
    FS_ERR_NOTEMPTY = -7,    // Dir is not empty
    FS_ERR_BADF = -8,        // Bad file number
    FS_ERR_FBIG = -9,        // File too large
    FS_ERR_INVAL = -10,      // Invalid parameter
    FS_ERR_NOSPC = -11,      // No space left on device
    FS_ERR_NOMEM = -12,      // No more memory available
    FS_ERR_NOATTR = -13,     // No data/attr available
    FS_ERR_NAMETOOLONG = -14 // File name too long
} fserror_t;

typedef enum {
    MACHINE_AUTO = 0,
    MACHINE_APPLE_II = 1,
    MACHINE_APPLE_IIE = 2,
    MACHINE_APPLE_IIGS = 3,
    MACHINE_PRAVETZ = 4,
    MACHINE_MAXVALUE = 5
} compat_t;

void hexprint(volatile uint8_t *buf, int size) {
    while(size--) {
        cprintf("%2X ", *buf++);
    }
}

int cfg_cmd3(char *cmd, uint16_t param0, uint16_t param1, uint16_t param2) {
    uint16_t timeout = 0x1fff;

    gotoy(16); gotox(8);
    cputc(cmd[0]);
    cputc(cmd[1]);
    cprintf(" $%04X $%04X $%04X", param0, param1, param2);
    
    RPY_BUFFER[7] = 0xFF;
    CMD_BUFFER[7] = (param2 >> 8) & 0xFF;
    RPY_BUFFER[6] = 0xFF;
    CMD_BUFFER[6] = param2 & 0xFF;
    RPY_BUFFER[5] = 0xFF;
    CMD_BUFFER[5] = (param1 >> 8) & 0xFF;
    RPY_BUFFER[4] = 0xFF;
    CMD_BUFFER[4] = param1 & 0xFF;
    RPY_BUFFER[3] = 0xFF;
    CMD_BUFFER[3] = (param0 >> 8) & 0xFF;
    RPY_BUFFER[2] = 0xFF;
    CMD_BUFFER[2] = param0 & 0xFF;
    RPY_BUFFER[1] = 0xFF;
    CMD_BUFFER[1] = cmd[1];
    RPY_BUFFER[0] = 0xFF;
    CMD_BUFFER[0] = cmd[0];

    // Wait for the command to start processing
    while(RPY_BUFFER[0] == 0xFF) {
        if(timeout > 0) timeout--;
    }

    // Wait while the command is processing
    while(RPY_BUFFER[0] == REPLY_BUSY) {
        if(timeout > 0) timeout--;
    }

    gotoy(18); gotox(8);
    hexprint(RPY_BUFFER, 8);

    return (timeout == 0) || (RPY_BUFFER[0] != REPLY_OK);
}

int cfg_cmd2(char *cmd, uint16_t param0, uint16_t param1) {
    return cfg_cmd3(cmd, param0, param1, 0);
}

int cfg_cmd1(char *cmd, uint16_t param0) {
    return cfg_cmd3(cmd, param0, 0, 0);
}

int cfg_cmd0(char *cmd) {
    return cfg_cmd3(cmd, 0, 0, 0);
}




#define NEWCONFIG_MAGIC    0x0001434E // "NC\x01\x00"

#define CFGTOKEN_MODE_VGA  0x0000564D // "MV\x00\x00" VGA
#define CFGTOKEN_MODE_PCPI 0x00005A4D // "MZ\x00\x00" PCPI Applicard
#define CFGTOKEN_MODE_SER  0x0000534D // "MS\x00\x00" Serial
#define CFGTOKEN_MODE_PAR  0x0000504D // "MP\x00\x00" Parallel
#define CFGTOKEN_MODE_SNES 0x0000474D // "MG\x00\x00" SNESMAX
#define CFGTOKEN_MODE_MIDI 0x00004D4D // "MM\x00\x00" SNESMAX
#define CFGTOKEN_MODE_NET  0x0000454D // "ME\x00\x00" Ethernet
#define CFGTOKEN_MODE_FILE 0x0000464D // "MF\x00\x00" Filesystem

#define CFGTOKEN_HOST_AUTO 0x00004148 // "HA\x00\x00" Autodetect
#define CFGTOKEN_HOST_II   0x00003248 // "H2\x00\x00" II/II+
#define CFGTOKEN_HOST_IIE  0x00004548 // "HE\x00\x00" IIe
#define CFGTOKEN_HOST_IIGS 0x00004748 // "HG\x00\x00" IIgs
#define CFGTOKEN_HOST_PRAVETZ 0x00005048 // "HP\x00\x00" Pravetz

#define CFGTOKEN_MUX_LOOP  0x00004C53 // "SL\x00\x00" Serial Loopback
#define CFGTOKEN_MUX_USB   0x00005553 // "SU\x00\x00" USB CDC
#define CFGTOKEN_MUX_WIFI  0x00005753 // "SW\x00\x00" WiFi Modem
#define CFGTOKEN_MUX_PRN   0x00005053 // "SP\x00\x00" WiFi Printer
#define CFGTOKEN_SER_BAUD  0x02004253 // "SB\x00\x00" Serial Baudrate

#define CFGTOKEN_USB_HOST  0x00004855 // "UH\x00\x00" USB CDC Host
#define CFGTOKEN_USB_GUEST 0x00004755 // "UG\x00\x00" USB CDC Guest Device
#define CFGTOKEN_USB_MIDI  0x00004D55 // "UM\x00\x00" USB MIDI Guest Device

#define CFGTOKEN_WIFI_AP   0x00004157 // "WA\x00\x00" WiFi AP
#define CFGTOKEN_WIFI_CL   0x00004357 // "WC\x00\x00" WiFi Client

#define CFGTOKEN_WIFI_SSID 0x00005357 // "WS\x00\xSS" WiFi SSID
#define CFGTOKEN_WIFI_PSK  0x00005057 // "WS\x00\xSS" WiFi PSK

#define CFGTOKEN_WIFI_IP   0x04504957 // "WIP\xSS" WiFi IP
#define CFGTOKEN_WIFI_NM   0x044D4E57 // "WNM\xSS" WiFi Netmask

#define CFGTOKEN_JD_HOST   0x0000484A // "JH\x00\x01" JetDirect Hostname
#define CFGTOKEN_JD_PORT   0x0200444A // "JD\x00\x01" JetDirect Port

#define CFGTOKEN_MONO_00   0x00005056 // "VP\x00\x00" Full Color Video
#define CFGTOKEN_MONO_80   0x00805056 // "VP\x80\x00" B&W Video
#define CFGTOKEN_MONO_90   0x00905056 // "VP\x90\x00" B&W Inverse
#define CFGTOKEN_MONO_A0   0x00A05056 // "VP\xA0\x00" Amber
#define CFGTOKEN_MONO_B0   0x00B05056 // "VP\xB0\x00" Amber Inverse
#define CFGTOKEN_MONO_C0   0x00C05056 // "VP\xC0\x00" Green
#define CFGTOKEN_MONO_D0   0x00D05056 // "VP\xD0\x00" Green Inverse
#define CFGTOKEN_MONO_E0   0x00E05056 // "VP\xE0\x00" C64
#define CFGTOKEN_MONO_F0   0x00F05056 // "VP\xF0\x00" Custom

#define CFGTOKEN_TBCOLOR   0x00005456 // "VT\xXX\x00" Custom default TBCOLOR
#define CFGTOKEN_BORDER    0x00004256 // "VB\xXX\x00" Custom default BORDER


#endif /* __V2ANALOG_H */
