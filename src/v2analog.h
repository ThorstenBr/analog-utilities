#ifndef __V2ANALOG_H
#define __V2ANALOG_H

#define CARD_REGISTER(n) (*(volatile unsigned char *)(0xC080 | (n) | (cardslot << 4)))

#define VGA_MONOMODE  CARD_REGISTER(0x1)
#define VGA_TBCOLOR   CARD_REGISTER(0x2)
#define VGA_BORDER    CARD_REGISTER(0x3)

#define CF_PTRL *((volatile unsigned char *)(0xC0EC | (cardslot << 8)))
#define CF_PTRH *((volatile unsigned char *)(0xC0ED | (cardslot << 8)))
#define CF_DATR *((volatile unsigned char *)(0xC0EE | (cardslot << 8)))
#define CF_DATW *((volatile unsigned char *)(0xC0EF | (cardslot << 8)))

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

typedef enum {
    MACHINE_AUTO = 0,
    MACHINE_APPLE_II = 1,
    MACHINE_APPLE_IIE = 2,
    MACHINE_APPLE_IIGS = 3,
    MACHINE_PRAVETZ = 4,
    MACHINE_BASIS = 5,
    MACHINE_MAXVALUE = 6
} compat_t;

void hexprint(volatile uint8_t *buf, int size) {
    while(size--) {
        cprintf("%2X ", *buf++);
    }
}

int cfg_cmd3(char *cmd, uint16_t param0, uint16_t param1, uint16_t param2) {
    uint16_t timeout = 0x1fff;

#if 0
    gotoy(16); gotox(8);
    cputc(cmd[0]);
    cputc(cmd[1]);
    cprintf(" $%04X $%04X $%04X", param0, param1, param2);
#endif
    
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

#if 0
    gotoy(18); gotox(8);
    hexprint(RPY_BUFFER, 8);
#endif

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

#endif /* __V2ANALOG_H */
