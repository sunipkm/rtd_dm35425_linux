#
#	FILE NAME: Makefile
#
#	FILE DESCRIPTION: Make description file for building example programs
#
#	PROJECT NAME: Linux Software (DM35425)
#
#	PROJECT VERSION: (Defined in README.TXT)
#
#	This file and its contents are copyright (C) RTD Embedded Technologies,
#	Inc.  All Rights Reserved.
#
#	This software is licensed as described in the RTD End-User Software
#	License Agreement.  For a copy of this agreement, refer to the file
#	LICENSE.TXT (which should be included with this software) or contact RTD
#	Embedded Technologies, Inc.
#

CC=gcc
DEBUG_FLAGS=-g
INCLUDE_FLAGS=-I../include
LIBRARY_FLAGS=-L../lib -lrtd-dm35425 -lpthread -lm
OPTIMIZE_FLAGS=-O2
WARNING_FLAGS=-Wall
CFLAGS:=$(DEBUG_FLAGS) $(INCLUDE_FLAGS) $(OPTIMIZE_FLAGS) $(WARNING_FLAGS)

EXAMPLES = \
	dm35425_list_fb \
	dm35425_adc \
	dm35425_adc_continuous_dma \
	dm35425_adc_all_dma \
	dm35425_adc_fifo \
	dm35425_dac \
	dm35425_adio \
	dm35425_adio_adv_int \
	dm35425_adio_dma \
	dm35425_ext_clocking \
	dm35425_dac_dma \
	dm35425_adio_parallel_bus

all:	$(EXAMPLES)

%:	%.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBRARY_FLAGS)

clean:
	rm -f *.o *.dat *~ $(EXAMPLES) $(TESTS)
