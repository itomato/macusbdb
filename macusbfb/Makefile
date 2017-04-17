##########################################################################
# User configuration and firmware specific object files	
##########################################################################

# The target, flash and ram of the LPC1xxx microprocessor.
# Use for the target the value: LPC11xx, LPC13xx or LPC17xx
TARGET = LPC13xx
FLASH = 32K
SRAM = 8K

# For USB support the LPC1xxx reserves 384 bytes from the sram,
# if you don't want to use the USB features, just use 0 here.
SRAM_USB = 384

VPATH = 
OBJS = main.o debug.o font_8x8.o usb.o

##########################################################################
# Library files 
##########################################################################
VPATH += core core/cpu
VPATH += core/systick
VPATH += core/libc core/wdt
OBJS += cpu.o systick.o
OBJS += stdio.o string.o
OBJS += sysinit.o

##########################################################################
# GNU GCC compiler prefix and location
##########################################################################
CROSS_COMPILE = arm-elf-
AS = $(CROSS_COMPILE)gcc
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
OUTFILE = firmware

##########################################################################
# Startup files
##########################################################################

LD_PATH = lpc1xxx
LD_SCRIPT = $(LD_PATH)/linkscript.ld
LD_TEMP = $(LD_PATH)/memory.ld

ifeq (LPC11xx,$(TARGET))
  CORTEX_TYPE=m0
else
  CORTEX_TYPE=m3
endif

CPU_TYPE = cortex-$(CORTEX_TYPE)
VPATH += lpc1xxx
OBJS += $(TARGET)_handlers.o LPC1xxx_startup.o

##########################################################################
# Compiler settings, parameters and flags
##########################################################################
#
CFLAGS  = -c -Os -I. -Wall  -mthumb -ffunction-sections -fdata-sections -fmessage-length=0 -mcpu=$(CPU_TYPE) -DTARGET=$(TARGET) -fno-builtin -g
ASFLAGS = -c -Os -I. -Wall -mthumb -ffunction-sections -fdata-sections -fmessage-length=0 -mcpu=$(CPU_TYPE) -D__ASSEMBLY__ -x assembler-with-cpp -g
LDFLAGS = -Wl,--gc-sections -mthumb -g
OCFLAGS = --strip-unneeded

all: firmware
	../checksum/checksum firmware.bin
#Enable this to write to the firmware to the lpc. Change sdx in wherever your lpc
#shows up.
#	sudo rmmod macusbfb || true
#	while [ ! -e /dev/sdx ]; do sleep 1; done;  sudo mcopy -o -i /dev/sdx firmware.bin ::/; sudo blockdev --flushbufs /dev/sdx

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o : %.s
	$(AS) $(ASFLAGS) -o $@ $<

firmware: $(OBJS) $(SYS_OBJS)
	-@echo "MEMORY" > $(LD_TEMP)
	-@echo "{" >> $(LD_TEMP)
	-@echo "  flash(rx): ORIGIN = 0x00000000, LENGTH = $(FLASH)" >> $(LD_TEMP)
	-@echo "  sram(rwx): ORIGIN = 0x10000000+$(SRAM_USB), LENGTH = $(SRAM)-$(SRAM_USB)" >> $(LD_TEMP)
	-@echo "}" >> $(LD_TEMP)
	-@echo "INCLUDE $(LD_SCRIPT)" >> $(LD_TEMP)
	$(LD) $(LDFLAGS) -T $(LD_TEMP) -o $(OUTFILE).elf $(OBJS)
	-@echo ""
	$(SIZE) $(OUTFILE).elf
	-@echo ""
	$(OBJCOPY) $(OCFLAGS) -O binary $(OUTFILE).elf $(OUTFILE).bin
	$(OBJCOPY) $(OCFLAGS) -O ihex $(OUTFILE).elf $(OUTFILE).hex

clean:
	rm -f $(OBJS) $(LD_TEMP) $(OUTFILE).elf $(OUTFILE).bin $(OUTFILE).hex
