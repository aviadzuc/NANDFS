 #
 # Copyright (C) 2009 Aviad Zuck & Sivan Toledo
 # This file is part of NANDFS.
 #  
 # To license NANDFS under a different license, please
 # contact the authors.
 #
 # NANDFS is free software: you can redistribute it and/or modify
 # it under the terms of the GNU Lesser General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # NANDFS is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU Lesser General Public License for more details.
 # You should have received a copy of the GNU Lesser General Public License
 # along with NANDFS.  If not, see <http://www.gnu.org/licenses/>.
 #

#SIM_MODE = ARM
SIM_MODE = SIM
HOME     = NANDFS
CHIP     = LARGE_PAGE
#CHIP     = SMALL_PAGE
#ifeq ($(SIM_MODE), SIM) 
CFLAGS += -D$(SIM_MODE) -DDO_ECC -DDebug 
#CFLAGS += -DNAND_2048
#endif
CFLAGS += -D$(SIM_MODE) -D$(CHIP) -DCONFIG_NAND_FS -DSYSTEM_TYPES 
#-DDO_ECC 
#-DDebug
# special nightly tests
#CFLAGS += -DNightlyTests 

#SUBMDL = LPC2214
SUBMDL = LPC2119
PLATFORM = OLIMEX_LPC_H2214
PLATFORM = ETT_ARM_STAMP
RTOS     = NONE
#RTOS     = UCOS-II

#DOWNLOAD_TO=DOWNLOAD_TO_RAM
DOWNLOAD_TO=DOWNLOAD_TO_FLASH

TARGET = testsAll
#TARGET = temp_main

# List C source files here. (C dependencies are automatically generated.)
# use file-extension c for "c-only"-files
SRC  = $(TARGET).c
SRC += globals.c

#for busyWaitInit()
#SRC += ../ARM/lpc2000/adc.c
#SRC += ../ARM/lpc2000/spi.c
#SRC += ../ARM/lpc2000/i2c.c
#SRC += ../ARM/lpc2000/timers.c
SRC += ../$(SIM_MODE)/lpc2000/busywait.c
SRC += ../$(SIM_MODE)/lpc2000/clocks.c

SRC += ../$(SIM_MODE)/peripherals/nand.c
SRC += src/sequencing/sequencing.c
SRC += src/sequencing/sequencingUtils.c
SRC += src/sequencing/lfsr.c
SRC += ../$(SIM_MODE)/utils/memlib.c
SRC += ../$(SIM_MODE)/utils/string_lib.c
SRC += ../$(SIM_MODE)/utils/yaffs_ecc.c
SRC += src/fs/insertionsort.c

SRC += src/fs/fs.c
SRC += src/fs/fsUtils.c
SRC += src/fs/transactions.c
SRC += src/fs/cache.c

#tests source files
SRC += test/testUtils.c
SRC += ../$(SIM_MODE)/lpc2000/uart0.c
SRC += ../$(SIM_MODE)/utils/print.c

#SRC += test/fs/testFsUtils.c
#SRC += test/fs/testFsSeperate.c

#SRC += test/fs/lruTests.c
#SRC += test/fs/auxiliaryFuncsTests.c
#SRC += test/fs/openTests.c
#SRC += test/fs/creatTests.c
#SRC += test/fs/writeTests.c
#SRC += test/fs/closeTests.c
#SRC += test/fs/readTests.c
#SRC += test/fs/unlinkTests.c
#SRC += test/fs/mkdirTests.c
#SRC += test/fs/rmdirTests.c
#SRC += test/fs/fsyncTests.c
#SRC += test/fs/opendirTests.c
#SRC += test/fs/readdirTests.c
#SRC += test/fs/variousFuncsTests.c
#SRC += test/fs/scandirTests.c
#SRC += test/fs/tearTests.c

SRC += test/sequencing/testSequencingUtils.c
SRC += test/sequencing/testSequencingSeperate.c

SRC += test/sequencing/logicalAddressToReservePhysicalTests.c
#SRC += test/sequencing/logicalAddressToPhysicalTests.c
#SRC += test/sequencing/increment_counterTests.c
#SRC += test/sequencing/performWearLevelingTests.c
#SRC += test/sequencing/allocAndWriteBlockTests.c
#SRC += test/sequencing/readBlockTests.c
#SRC += test/sequencing/commitTests.c
#SRC += test/sequencing/findCheckpointAndTruncateTests.c
#SRC += test/sequencing/readVOTsAndPrevTests.c
#SRC += test/sequencing/variousTests.c
#SRC += test/sequencing/sequencingBootingTests.c
#SRC += test/sequencing/verifyECCTests.c
#SRC += test/sequencing/markAsObsoleteTests.c
#SRC += test/sequencing/bit_generator_mechanismTests.c

#SRC += test/fs/nightlyFsTests.c
#SRC += test/sequencing/nightlySequencingTests.c

ASRC =
ASRCARM =

include ../$(SIM_MODE)/makefile

