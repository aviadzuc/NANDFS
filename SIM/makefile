# Hey Emacs, this is a -*- makefile -*-
#
# WinARM template makefile 
# by Martin Thomas, Kaiserslautern, Germany 
# <eversmith@heizung-thomas.de>
#
# based on the WinAVR makefile written by Eric B. Weddington, J?rg Wunsch, et al.
# Released to the Public Domain
# Please read the make user manual!
#
#
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make program = Download the hex file to the device, using lpc21isp
#
# (TODO: make filename.s = Just compile filename.c into the assembler code only)
#
# To rebuild project do "make clean" then "make all".
#
# Changelog:
# - 17. Feb. 2005  - added thumb-interwork support (mth)
# - 28. Apr. 2005  - added C++ support (mth)
# - 29. Arp. 2005  - changed handling for lst-Filename (mth)
#

# MCU name and submodel
#MCU = arm7tdmi
#SUBMDL = LPC2119
#THUMB    = -mthumb
#THUMB_IW = -mthumb-interwork


## Create ROM-Image (final)
#RUN_MODE=ROM_RUN
## Create RAM-Image (debugging)
#RUN_MODE=RAM_RUN

# Target file name (without extension).
#TARGET = test

# List C source files here. (C dependencies are automatically generated.)
# use file-extension c for "c-only"-files
#SRC = $(TARGET).c
#SRC += clocks.c

# List C source files here which must be compiled in ARM-Mode.
# use file-extension c for "c-only"-files
SRCARM = 

# List C++ source files here.
# use file-extension cpp for C++-files
CPPSRC = 

# List C++ source files here which must be compiled in ARM-Mode.
# use file-extension cpp for C++-files
# CPPSRCARM = $(TARGET).cpp
CPPSRCARM = 

# List Assembler source files here.
# Make them always end in a capital .S.  Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC = 

# List Assembler source files here which must be assembled in ARM-Mode..
ASRCARM = 

# Optimization level, can be [0, 1, 2, 3, s]. 
# 0 = turn off optimization. s = optimize for size.
# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
OPT = s

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
#DEBUG = stabs
DEBUG = 

# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
#EXTRAINCDIRS = ./include
EXTRAINCDIRS = 

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Place -D or -U options for C here
CDEFS =  

# Place -I options here
CINCS = -I../SIM/ -I../$(HOME)/

# Place -D or -U options for ASM here
ADEFS =  
# XXX ERORR IN STARTUP.S!!!
ADEFS =  s= 

# Compiler flags.
#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
#
# Flags for C and C++ (arm-elf-gcc/arm-elf-g++)
#CFLAGS = -g$(DEBUG)
CFLAGS += $(CDEFS) $(CINCS)
CFLAGS += -g 
#CFLAGS += -O$(OPT) -g 
#CFLAGS += -Wall -Wcast-align -Wcast-qual -Wimplicit 
#CFLAGS += -Wpointer-arith -Wswitch
#CFLAGS += -Wredundant-decls -Wreturn-type -Wshadow -Wunused
##CFLAGS += -Wa,-adhlns=$(<:.c=.lst) 
#CFLAGS += -Wa,-adhlns=$(subst $(suffix $<),.lst,$<) 
#CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += -fno-common -fno-strict-aliasing -Wall -Wextra -Wno-pointer-sign -Wunreachable-code -Wno-sign-compare -Wno-unreachable-code -fno-stack-protector
# -Wno-comment -Wlarger-than-2200
#-Wdeclaration-after-statement -Wstrict-prototypes -pedantic

# flags only for C
CONLYFLAGS  = -Wstrict-prototypes -Wmissing-declarations 
CONLYFLAGS += -Wmissing-prototypes -Wnested-externs 
CONLYFLAGS += $(CSTANDARD)

# flags only for C++ (arm-elf-g++)
CPPFLAGS =

# Assembler flags.
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -ahlms:    create listing
#  -gstabs:   have the assembler create line number information; note that
#             for use in COFF files, additional information about filenames
#             and function names needs to be present in the assembler source
#             files -- see avr-libc docs [FIXME: not yet described there]
##ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs 
#ASFLAGS = $(ADEFS) -Wa,-adhlns=$(<:.S=.lst),-g$(DEBUG)
ASFLAGS = $(ADEFS) -ahls -mapcs-32

#Additional libraries.

#Support for newlibc-lpc (file: libnewlibc-lpc.a)
#NEWLIBLPC = -lnewlib-lpc
NEWLIBCLPC =

MATH_LIB = -lm

# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS += $(MATH_LIB)

LINKER_SCRIPT = unknown

# ---------------------------------------------------------------------------
# Flash-Programming support using lpc21isp by Martin Maurer 

# Settings and variables:
#LPC21ISP = lpc21isp
LPC21ISP = lpc21isp_beta
LPC21ISP_PORT = com1s= 
LPC21ISP_BAUD = 115200
LPC21ISP_XTAL = 14746
LPC21ISP_FLASHFILE = $(TARGET).hex
# verbose output:
## LPC21ISP_DEBUG = -debug
# enter bootloader via RS232 DTR/RTS (only if hardware supports this
# feature - see Philips AppNote):
#LPC21ISP_CONTROL = -control


# ---------------------------------------------------------------------------

# Define directories, if needed.
## DIRARM = c:/WinARM/
## DIRARMBIN = $(DIRAVR)/bin/
## DIRAVRUTILS = $(DIRAVR)/utils/bin/

# Define programs and commands.
SHELL = sh
CC = gcc
AS = as
CPP = g++
OBJCOPY = objcopy
OBJDUMP = objdump
SIZE = size
NM = nm
REMOVE = rm -f
COPY = cp


# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before: 
MSG_SIZE_AFTER = Size after:
MSG_FLASH = Creating load file for Flash:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_COMPILING_ARM = "Compiling C :"
MSG_COMPILINGCPP = Compiling C++:
MSG_COMPILINGCPP_ARM = "Compiling C++ :"
MSG_ASSEMBLING = Assembling:
MSG_ASSEMBLING_ARM = "Assembling :"
MSG_CLEANING = Cleaning project:
MSG_LPC21_RESETREMINDER = You may have to bring the target in bootloader-mode now.

#SRC  = peripherals/nand.c
#SRC += peripherals/simulator-test.c

# Define all object files.
COBJ      = $(SRC:.c=.o) 
AOBJ      = $(ASRC:.S=.o)
COBJARM   = $(SRCARM:.c=.o)
AOBJARM   = $(ASRCARM:.S=.o)
CPPOBJ    = $(CPPSRC:.cpp=.o) 
CPPOBJARM = $(CPPSRCARM:.cpp=.o)

# Define all listing files.
LST = $(ASRC:.S=.lst) $(ASRCARM:.S=.lst) $(SRC:.c=.lst) $(SRCARM:.c=.lst)
LST += $(CPPSRC:.cpp=.lst) $(CPPSRCARM:.cpp=.lst)

#aviad - new all, clean etc. for simulation
.c.o:
	$(CC) -c $(CFLAGS) $<

all:	$(TARGET)

$(TARGET) : $(COBJ)	
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $(notdir $(COBJ))

clean:	
	$(REMOVE) $(TARGET) $(notdir $(COBJ)) *~

rebuild: clean all

depend:
	$(CC) -M $(SRC) > makefile.depend

-include makefile.depend
