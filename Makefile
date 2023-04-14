DISK525A_OBJS = menu config
DISK525B_OBJS = menu
DISK525C_OBJS = menu fontmgr
DISK525D_OBJS = menu fontmgr
DISK35_OBJS = menu config fontmgr

FONT_OBJS = 01.us.une.cf    02.clintonv1.cf 03.reactive.cf  04.danpaymar.cf \
            05.blippo.cf    06.byte.cf      07.colossal.cf  08.count.cf     \
            09.flow.cf      0a.gothic.cf    0b.outline.cf   0c.pigfont.cf   \
            0d.pinocchio.cf 0e.slant.cf     0f.stop.cf      10.euro.une.cf  \
            11.euro.enh.cf  12.clintonv2.cf 13.germanenh.cf 14.germanune.cf \
            15.frenchenh.cf 16.frenchune.cf 17.hebrewenh.cf 18.hebrewune.cf \
            19.plus.enh.cf  1a.plus.une.cf  1b.katakana.cf  1c.cyrillic.cf  \
            1d.greek.cf     1e.esperanto.cf 1f.videx.cf     20.plus.une.cf  \
            21.us.enh.cf    22.us.enh.cf    23.cyrillic.cf  24.pcbold.cf    \
            25.aniron.cf

SYSTEM_TARGETS = menu
BASE_TARGETS = menu config fontmgr
ENH_TARGETS = menu config fontmgr
DISK_TARGETS = disk35 disk525a disk525b disk525c disk525d


# For this one see https://applecommander.github.io/
AC ?= tools/ac.jar
ACX ?= tools/acx.jar
DEFLATE ?= tools/deflater

# Just the usual way to find out if we're
# using cmd.exe to execute make rules.
ifneq ($(shell echo),)
  CMD_EXE = 1
endif

ifdef CMD_EXE
  NULLDEV = nul:
  DEL = -del /f
  RMDIR = rmdir /s /q
else
  NULLDEV = /dev/null
  DEL = $(RM)
  RMDIR = $(RM) -r
endif

ifdef CC65_HOME
  AS = $(CC65_HOME)/bin/ca65
  CC = $(CC65_HOME)/bin/cc65
  CL = $(CC65_HOME)/bin/cl65
  LD = $(CC65_HOME)/bin/ld65
else
  AS := $(if $(wildcard ../../bin/ca65*),../../bin/ca65,ca65)
  CC := $(if $(wildcard ../../bin/cc65*),../../bin/cc65,cc65)
  CL := $(if $(wildcard ../../bin/cl65*),../../bin/cl65,cl65)
  LD := $(if $(wildcard ../../bin/ld65*),../../bin/ld65,ld65)
endif

ENFORCESIZE = @(FILESIZE=`stat -c%s $1` ; \
	if [ $2 -gt 0 ]; then \
		if [ $$FILESIZE -gt $2 ] ; then \
			echo "ERROR: File $1 exceeds size limit ($$FILESIZE > $2)" ; \
			exit 1 ; \
		fi ; \
	fi )

OBJDIR = obj
SRCDIR = src

all: binaries disks
binaries: fonts systems bases enhs
fonts: $(addprefix $(OBJDIR)/,$(FONT_OBJS))
systems: fonts $(addprefix $(OBJDIR)/,$(addsuffix .system,$(SYSTEM_TARGETS)))
bases: $(addprefix $(OBJDIR)/,$(addsuffix .base,$(BASE_TARGETS)))
enhs: $(addprefix $(OBJDIR)/,$(addsuffix .enh,$(ENH_TARGETS)))
disks: $(addprefix $(OBJDIR)/,$(addsuffix .po,$(DISK_TARGETS)))

.PHONY: binaries fonts systems bases enhs disks $(SYSTEM_TARGETS)

$(OBJDIR)/%.xf: fonts/00.us.enh.pf fonts/%.pf
	tools/xorfont US.ENH....00.PF $^ $@

$(OBJDIR)/1a.plus.une.xf: fonts/19.plus.enh.pf fonts/1a.plus.une.pf
	tools/xorfont PLUS.ENH..19.PF $^ $@

$(OBJDIR)/20.plus.une.xf: fonts/19.plus.enh.pf fonts/20.plus.une.pf
	tools/xorfont PLUS.ENH..19.PF $^ $@

$(OBJDIR)/%.cf: $(OBJDIR)/%.xf
	tools/rlefont $< $@

$(OBJDIR)/%.base: $(SRCDIR)/%.c
	$(CL) -O -t apple2 -C apple2-system.cfg -m $@.map -o $@.AppleSingle $^
	@~/applesingle/applesingle -r < $@.AppleSingle > $@ 2>$(NULLDEV)
	@echo $@"\r\t\t\t" `wc -c $@ | cut -d ' ' -f 1`

$(OBJDIR)/%.enh: $(SRCDIR)/%.c
	$(CL) -O -t apple2enh -C apple2-system.cfg -m $@.map -o $@.AppleSingle $^
	@~/applesingle/applesingle -r < $@.AppleSingle > $@ 2>$(NULLDEV)
	@echo $@"\r\t\t\t" `wc -c $@ | cut -d ' ' -f 1`

$(OBJDIR)/%.system: $(SRCDIR)/loader.c
	$(CL) -O -t apple2 -DNEXTNAME=\"`echo $(notdir $(basename $@)) | tr a-z A-Z`\" -C apple2-system.cfg -m $@.map -o $@.AppleSingle $(SRCDIR)/loader.c
	@~/applesingle/applesingle -r < $@.AppleSingle > $@ 2>$(NULLDEV)
	@echo $@"\r\t\t\t" `wc -c $@ | cut -d ' ' -f 1`

$(OBJDIR)/disk525a.po: $(OBJDIR)/menu.system $(addprefix $(OBJDIR)/,$(addsuffix .base,$(DISK525A_OBJS))) $(addprefix $(OBJDIR)/,$(addsuffix .enh,$(DISK525A_OBJS)))
	@cp prodos/pd525.po $@
	java -jar $(AC) -n   $@ V2A.SIDEA
	java -jar $(AC) -p   $@ MENU.SYSTEM            SYS 0x2000  <$(OBJDIR)/menu.system
	java -jar $(AC) -p   $@ MENU.BASE              SYS 0x2000  <$(OBJDIR)/menu.base
	java -jar $(AC) -p   $@ MENU.ENH               SYS 0x2000  <$(OBJDIR)/menu.enh
	java -jar $(AC) -p   $@ CONFIG.BASE            SYS 0x2000  <$(OBJDIR)/config.base
	java -jar $(AC) -p   $@ CONFIG.ENH             SYS 0x2000  <$(OBJDIR)/config.enh
	java -jar $(AC) -p   $@ BASIC.SYSTEM           SYS 0x2000  <prodos/basic.system

$(OBJDIR)/disk525b.po: $(addprefix $(OBJDIR)/,$(addsuffix .base,$(DISK525B_OBJS))) $(addprefix $(OBJDIR)/,$(addsuffix .enh,$(DISK525B_OBJS)))
	@cp prodos/pd525.po $@
	java -jar $(AC) -n   $@ V2A.SIDEB
	java -jar $(AC) -p   $@ BASIC.SYSTEM           SYS 0x2000  <prodos/basic.system
	java -jar $(ACX) mkdir -d=$@ ADTPRO
	java -jar $(AC) -p   $@ ADTPRO/ADTPRO          SYS 0x2000  <adtpro/adtpro
	java -jar $(AC) -p   $@ ADTPRO/ADTPRO.BIN      BIN 0x0800  <adtpro/adtpro.bin
	java -jar $(AC) -p   $@ ADTPRO/ADTPROAUD       SYS 0x2000  <adtpro/adtproaud
	java -jar $(AC) -p   $@ ADTPRO/ADTPROAUD.BIN   BIN 0x0800  <adtpro/adtproaud.bin
	java -jar $(AC) -p   $@ ADTPRO/ADTPROETH       SYS 0x2000  <adtpro/adtproeth
	java -jar $(AC) -p   $@ ADTPRO/ADTPROETH.BIN   BIN 0x0800  <adtpro/adtproeth.bin
	java -jar $(ACX) mkdir -d=$@ VDRIVE
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.BIN     BIN 0x2000  <adtpro/vedrive
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.LOW     SYS 0x2000  <adtpro/vedrive.low
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.SETUP   BAS 0x0801  <adtpro/vedrive.setup
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.SYSTEM  SYS 0x2000  <adtpro/vedrive.system
	java -jar $(AC) -p   $@ VDRIVE/VSDRIVE         SYS 0x2000  <adtpro/vsdrive
	java -jar $(AC) -p   $@ VDRIVE/VSDRIVE.LOW     SYS 0x2000  <adtpro/vsdrive.low

$(OBJDIR)/disk525c.po: $(OBJDIR)/menu.system fonts $(addprefix $(OBJDIR)/,$(addsuffix .base,$(DISK525C_OBJS))) $(addprefix $(OBJDIR)/,$(addsuffix .enh,$(DISK525C_OBJS)))
	@cp prodos/pd525.po $@
	java -jar $(AC) -n   $@ FONTS
	java -jar $(ACX) mkdir -d=$@ FONTS
	java -jar $(AC) -p   $@ FONTS/US.ENH....00.PF  BIN 0xF027  <fonts/00.us.enh.pf
	java -jar $(AC) -p   $@ FONTS/US.UNE....01.CF  BIN 0xCF27  <$(OBJDIR)/01.us.une.cf
	java -jar $(AC) -p   $@ FONTS/CLINTONV1.02.CF  BIN 0xCF27  <$(OBJDIR)/02.clintonv1.cf
	java -jar $(AC) -p   $@ FONTS/REACTIVE..03.CF  BIN 0xCF27  <$(OBJDIR)/03.reactive.cf
	java -jar $(AC) -p   $@ FONTS/DANPAYMAR.04.CF  BIN 0xCF27  <$(OBJDIR)/04.danpaymar.cf
	java -jar $(AC) -p   $@ FONTS/BLIPPO....05.CF  BIN 0xCF27  <$(OBJDIR)/05.blippo.cf
	java -jar $(AC) -p   $@ FONTS/BYTE......06.CF  BIN 0xCF27  <$(OBJDIR)/06.byte.cf
	java -jar $(AC) -p   $@ FONTS/COLOSSAL..07.CF  BIN 0xCF27  <$(OBJDIR)/07.colossal.cf
	java -jar $(AC) -p   $@ FONTS/COUNT.....08.CF  BIN 0xCF27  <$(OBJDIR)/08.count.cf
	java -jar $(AC) -p   $@ FONTS/FLOW......09.CF  BIN 0xCF27  <$(OBJDIR)/09.flow.cf
	java -jar $(AC) -p   $@ FONTS/GOTHIC....0A.CF  BIN 0xCF27  <$(OBJDIR)/0a.gothic.cf
	java -jar $(AC) -p   $@ FONTS/OUTLINE...0B.CF  BIN 0xCF27  <$(OBJDIR)/0b.outline.cf
	java -jar $(AC) -p   $@ FONTS/PIGFONT...0C.CF  BIN 0xCF27  <$(OBJDIR)/0c.pigfont.cf
	java -jar $(AC) -p   $@ FONTS/PINOCCHIO.0D.PF  BIN 0xF027  <fonts/0d.pinocchio.pf
	java -jar $(AC) -p   $@ FONTS/SLANT.....0E.CF  BIN 0xCF27  <$(OBJDIR)/0e.slant.cf
	java -jar $(AC) -p   $@ FONTS/STOP......0F.CF  BIN 0xCF27  <$(OBJDIR)/0f.stop.cf
	java -jar $(AC) -p   $@ MENU.SYSTEM            SYS 0x2000  <$(OBJDIR)/menu.system
	java -jar $(AC) -p   $@ MENU.BASE              SYS 0x2000  <$(OBJDIR)/menu.base
	java -jar $(AC) -p   $@ MENU.ENH               SYS 0x2000  <$(OBJDIR)/menu.enh
	java -jar $(AC) -p   $@ FONTMGR.BASE           SYS 0x2000  <$(OBJDIR)/fontmgr.base
	java -jar $(AC) -p   $@ FONTMGR.ENH            SYS 0x2000  <$(OBJDIR)/fontmgr.enh

$(OBJDIR)/disk525d.po: $(OBJDIR)/menu.system fonts $(addprefix $(OBJDIR)/,$(addsuffix .base,$(DISK525D_OBJS))) $(addprefix $(OBJDIR)/,$(addsuffix .enh,$(DISK525D_OBJS)))
	@cp prodos/pd525.po $@
	java -jar $(AC) -n   $@ FONTS
	java -jar $(ACX) mkdir -d=$@ FONTS
	java -jar $(AC) -p   $@ FONTS/US.ENH....00.PF  BIN 0xF027  <fonts/00.us.enh.pf
	java -jar $(AC) -p   $@ FONTS/EURO.UNE..10.CF  BIN 0xCF27  <$(OBJDIR)/10.euro.une.cf
	java -jar $(AC) -p   $@ FONTS/EURO.ENH..11.CF  BIN 0xCF27  <$(OBJDIR)/11.euro.enh.cf
	java -jar $(AC) -p   $@ FONTS/CLINTONV2.12.CF  BIN 0xCF27  <$(OBJDIR)/12.clintonv2.cf
	java -jar $(AC) -p   $@ FONTS/GERMANENH.13.CF  BIN 0xCF27  <$(OBJDIR)/13.germanenh.cf
	java -jar $(AC) -p   $@ FONTS/GERMANUNE.14.CF  BIN 0xCF27  <$(OBJDIR)/14.germanune.cf
	java -jar $(AC) -p   $@ FONTS/FRENCHENH.15.CF  BIN 0xCF27  <$(OBJDIR)/15.frenchenh.cf
	java -jar $(AC) -p   $@ FONTS/FRENCHUNE.16.CF  BIN 0xCF27  <$(OBJDIR)/16.frenchune.cf
	java -jar $(AC) -p   $@ FONTS/HEBREWENH.17.CF  BIN 0xCF27  <$(OBJDIR)/17.hebrewenh.cf
	java -jar $(AC) -p   $@ FONTS/HEBREWUNE.18.CF  BIN 0xCF27  <$(OBJDIR)/18.hebrewune.cf
	java -jar $(AC) -p   $@ FONTS/PLUS.ENH..19.PF  BIN 0xF027  <fonts/19.plus.enh.pf
	java -jar $(AC) -p   $@ FONTS/PLUS.UNE..1A.CF  BIN 0xCF27  <$(OBJDIR)/1a.plus.une.cf
	java -jar $(AC) -p   $@ FONTS/KATAKANA..1B.PF  BIN 0xF027  <fonts/1b.katakana.pf
	java -jar $(AC) -p   $@ FONTS/CYRILLIC..1C.CF  BIN 0xCF27  <$(OBJDIR)/1c.cyrillic.cf
	java -jar $(AC) -p   $@ FONTS/GREEK.....1D.CF  BIN 0xCF27  <$(OBJDIR)/1d.greek.cf
	java -jar $(AC) -p   $@ FONTS/ESPERANTO.1E.CF  BIN 0xCF27  <$(OBJDIR)/1e.esperanto.cf
	java -jar $(AC) -p   $@ FONTS/VIDEX.....1F.CF  BIN 0xCF27  <$(OBJDIR)/1f.videx.cf
	java -jar $(AC) -p   $@ FONTS/PLUS.UNE..20.CF  BIN 0xCF27  <$(OBJDIR)/20.plus.une.cf
	java -jar $(AC) -p   $@ FONTS/US.ENH....21.CF  BIN 0xCF27  <$(OBJDIR)/21.us.enh.cf
	java -jar $(AC) -p   $@ FONTS/US.ENH....22.CF  BIN 0xCF27  <$(OBJDIR)/22.us.enh.cf
	java -jar $(AC) -p   $@ FONTS/CYRILLIC..23.CF  BIN 0xCF27  <$(OBJDIR)/23.cyrillic.cf
	java -jar $(AC) -p   $@ FONTS/PCBOLD....24.CF  BIN 0xCF27  <$(OBJDIR)/24.pcbold.cf
	java -jar $(AC) -p   $@ FONTS/ANIRON....25.CF  BIN 0xCF27  <$(OBJDIR)/25.aniron.cf
	java -jar $(AC) -p   $@ FONTS/ANIRON....25.PF  BIN 0xF027  <fonts/25.aniron.pf
	java -jar $(AC) -p   $@ MENU.SYSTEM            SYS 0x2000  <$(OBJDIR)/menu.system
	java -jar $(AC) -p   $@ MENU.BASE              SYS 0x2000  <$(OBJDIR)/menu.base
	java -jar $(AC) -p   $@ MENU.ENH               SYS 0x2000  <$(OBJDIR)/menu.enh
	java -jar $(AC) -p   $@ FONTMGR.BASE           SYS 0x2000  <$(OBJDIR)/fontmgr.base
	java -jar $(AC) -p   $@ FONTMGR.ENH            SYS 0x2000  <$(OBJDIR)/fontmgr.enh

$(OBJDIR)/disk35.po: $(OBJDIR)/menu.system $(addprefix $(OBJDIR)/,$(addsuffix .base,$(DISK35_OBJS))) $(addprefix $(OBJDIR)/,$(addsuffix .enh,$(DISK35_OBJS)))
	@cp prodos/pd35.po $@
	java -jar $(AC) -n   $@ V2ANALOG
	java -jar $(AC) -p   $@ MENU.SYSTEM            SYS 0x2000  <$(OBJDIR)/menu.system
	java -jar $(AC) -p   $@ BASIC.SYSTEM           SYS 0x2000  <prodos/basic.system
	java -jar $(ACX) mkdir -d=$@ ADTPRO
	java -jar $(AC) -p   $@ ADTPRO/ADTPRO          SYS 0x2000  <adtpro/adtpro
	java -jar $(AC) -p   $@ ADTPRO/ADTPRO.BIN      BIN 0x0800  <adtpro/adtpro.bin
	java -jar $(AC) -p   $@ ADTPRO/ADTPROAUD       SYS 0x2000  <adtpro/adtproaud
	java -jar $(AC) -p   $@ ADTPRO/ADTPROAUD.BIN   BIN 0x0800  <adtpro/adtproaud.bin
	java -jar $(AC) -p   $@ ADTPRO/ADTPROETH       SYS 0x2000  <adtpro/adtproeth
	java -jar $(AC) -p   $@ ADTPRO/ADTPROETH.BIN   BIN 0x0800  <adtpro/adtproeth.bin
	java -jar $(ACX) mkdir -d=$@ VDRIVE
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.BIN     BIN 0x2000  <adtpro/vedrive
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.LOW     SYS 0x2000  <adtpro/vedrive.low
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.SETUP   BAS 0x0801  <adtpro/vedrive.setup
	java -jar $(AC) -p   $@ VDRIVE/VEDRIVE.SYSTEM  SYS 0x2000  <adtpro/vedrive.system
	java -jar $(AC) -p   $@ VDRIVE/VSDRIVE         SYS 0x2000  <adtpro/vsdrive
	java -jar $(AC) -p   $@ VDRIVE/VSDRIVE.LOW     SYS 0x2000  <adtpro/vsdrive.low
	java -jar $(ACX) mkdir -d=$@ FONTS
	java -jar $(AC) -p   $@ FONTS/US.ENH....00.PF  BIN 0xF027  <fonts/00.us.enh.pf
	java -jar $(AC) -p   $@ FONTS/US.UNE....01.CF  BIN 0xCF27  <$(OBJDIR)/01.us.une.cf
	java -jar $(AC) -p   $@ FONTS/CLINTONV1.02.CF  BIN 0xCF27  <$(OBJDIR)/02.clintonv1.cf
	java -jar $(AC) -p   $@ FONTS/REACTIVE..03.CF  BIN 0xCF27  <$(OBJDIR)/03.reactive.cf
	java -jar $(AC) -p   $@ FONTS/DANPAYMAR.04.CF  BIN 0xCF27  <$(OBJDIR)/04.danpaymar.cf
	java -jar $(AC) -p   $@ FONTS/BLIPPO....05.CF  BIN 0xCF27  <$(OBJDIR)/05.blippo.cf
	java -jar $(AC) -p   $@ FONTS/BYTE......06.CF  BIN 0xCF27  <$(OBJDIR)/06.byte.cf
	java -jar $(AC) -p   $@ FONTS/COLOSSAL..07.CF  BIN 0xCF27  <$(OBJDIR)/07.colossal.cf
	java -jar $(AC) -p   $@ FONTS/COUNT.....08.CF  BIN 0xCF27  <$(OBJDIR)/08.count.cf
	java -jar $(AC) -p   $@ FONTS/FLOW......09.CF  BIN 0xCF27  <$(OBJDIR)/09.flow.cf
	java -jar $(AC) -p   $@ FONTS/GOTHIC....0A.CF  BIN 0xCF27  <$(OBJDIR)/0a.gothic.cf
	java -jar $(AC) -p   $@ FONTS/OUTLINE...0B.CF  BIN 0xCF27  <$(OBJDIR)/0b.outline.cf
	java -jar $(AC) -p   $@ FONTS/PIGFONT...0C.CF  BIN 0xCF27  <$(OBJDIR)/0c.pigfont.cf
	java -jar $(AC) -p   $@ FONTS/PINOCCHIO.0D.PF  BIN 0xF027  <fonts/0d.pinocchio.pf
	java -jar $(AC) -p   $@ FONTS/SLANT.....0E.CF  BIN 0xCF27  <$(OBJDIR)/0e.slant.cf
	java -jar $(AC) -p   $@ FONTS/STOP......0F.CF  BIN 0xCF27  <$(OBJDIR)/0f.stop.cf
	java -jar $(AC) -p   $@ FONTS/EURO.UNE..10.CF  BIN 0xCF27  <$(OBJDIR)/10.euro.une.cf
	java -jar $(AC) -p   $@ FONTS/EURO.ENH..11.CF  BIN 0xCF27  <$(OBJDIR)/11.euro.enh.cf
	java -jar $(AC) -p   $@ FONTS/CLINTONV2.12.CF  BIN 0xCF27  <$(OBJDIR)/12.clintonv2.cf
	java -jar $(AC) -p   $@ FONTS/GERMANENH.13.CF  BIN 0xCF27  <$(OBJDIR)/13.germanenh.cf
	java -jar $(AC) -p   $@ FONTS/GERMANUNE.14.CF  BIN 0xCF27  <$(OBJDIR)/14.germanune.cf
	java -jar $(AC) -p   $@ FONTS/FRENCHENH.15.CF  BIN 0xCF27  <$(OBJDIR)/15.frenchenh.cf
	java -jar $(AC) -p   $@ FONTS/FRENCHUNE.16.CF  BIN 0xCF27  <$(OBJDIR)/16.frenchune.cf
	java -jar $(AC) -p   $@ FONTS/HEBREWENH.17.CF  BIN 0xCF27  <$(OBJDIR)/17.hebrewenh.cf
	java -jar $(AC) -p   $@ FONTS/HEBREWUNE.18.CF  BIN 0xCF27  <$(OBJDIR)/18.hebrewune.cf
	java -jar $(AC) -p   $@ FONTS/PLUS.ENH..19.PF  BIN 0xF027  <fonts/19.plus.enh.pf
	java -jar $(AC) -p   $@ FONTS/PLUS.UNE..1A.CF  BIN 0xCF27  <$(OBJDIR)/1a.plus.une.cf
	java -jar $(AC) -p   $@ FONTS/KATAKANA..1B.PF  BIN 0xF027  <fonts/1b.katakana.pf
	java -jar $(AC) -p   $@ FONTS/CYRILLIC..1C.CF  BIN 0xCF27  <$(OBJDIR)/1c.cyrillic.cf
	java -jar $(AC) -p   $@ FONTS/GREEK.....1D.CF  BIN 0xCF27  <$(OBJDIR)/1d.greek.cf
	java -jar $(AC) -p   $@ FONTS/ESPERANTO.1E.CF  BIN 0xCF27  <$(OBJDIR)/1e.esperanto.cf
	java -jar $(AC) -p   $@ FONTS/VIDEX.....1F.CF  BIN 0xCF27  <$(OBJDIR)/1f.videx.cf
	java -jar $(AC) -p   $@ FONTS/PLUS.UNE..20.CF  BIN 0xCF27  <$(OBJDIR)/20.plus.une.cf
	java -jar $(AC) -p   $@ FONTS/US.ENH....21.CF  BIN 0xCF27  <$(OBJDIR)/21.us.enh.cf
	java -jar $(AC) -p   $@ FONTS/US.ENH....22.CF  BIN 0xCF27  <$(OBJDIR)/22.us.enh.cf
	java -jar $(AC) -p   $@ FONTS/CYRILLIC..23.CF  BIN 0xCF27  <$(OBJDIR)/23.cyrillic.cf
	java -jar $(AC) -p   $@ FONTS/PCBOLD....24.CF  BIN 0xCF27  <$(OBJDIR)/24.pcbold.cf
	java -jar $(AC) -p   $@ FONTS/ANIRON....25.CF  BIN 0xCF27  <$(OBJDIR)/25.aniron.cf
	java -jar $(AC) -p   $@ FONTS/ANIRON....25.PF  BIN 0xF027  <fonts/25.aniron.pf
	java -jar $(AC) -p   $@ MENU.BASE              SYS 0x2000  <$(OBJDIR)/menu.base
	java -jar $(AC) -p   $@ MENU.ENH               SYS 0x2000  <$(OBJDIR)/menu.enh
	java -jar $(AC) -p   $@ CONFIG.BASE            SYS 0x2000  <$(OBJDIR)/config.base
	java -jar $(AC) -p   $@ CONFIG.ENH             SYS 0x2000  <$(OBJDIR)/config.enh
	java -jar $(AC) -p   $@ FONTMGR.BASE           SYS 0x2000  <$(OBJDIR)/fontmgr.base
	java -jar $(AC) -p   $@ FONTMGR.ENH            SYS 0x2000  <$(OBJDIR)/fontmgr.enh

clean:
	@$(DEL) obj/* 2>$(NULLDEV)

