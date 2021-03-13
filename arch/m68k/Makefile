#
# m68k/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Hamish Macdonald
# Copyright (C) 2002,2011 Greg Ungerer <gerg@snapgear.com>
#

KBUILD_DEFCONFIG := defconfig

ifneq ($(SUBARCH),$(ARCH))
	ifeq ($(CROSS_COMPILE),)
		CROSS_COMPILE := $(call cc-cross-prefix, \
			m68k-linux-gnu- m68k-linux- m68k-unknown-linux-gnu-)
	endif
endif

KBUILD_AFLAGS += -m68040
KBUILD_CFLAGS += -m68040 -pipe

KBUILD_CFLAGS += -ffixed-a2

KBUILD_LDFLAGS := -m m68kelf
KBUILD_LDFLAGS_MODULE += -T $(srctree)/arch/m68k/kernel/module.lds

CHECKFLAGS += -D__mc68000__

#
# Select the assembler head startup code. Order is important. The default
# head code is first, processor specific selections can override it after.
#
head-y				:= arch/m68k/kernel/head.o

core-y				+= arch/m68k/kernel/	arch/m68k/mm/
libs-y				+= arch/m68k/lib/

core-$(CONFIG_ATX040)		+= arch/m68k/atx040/
core-$(CONFIG_M68040)		+= arch/m68k/fpsp040/
core-$(CONFIG_M68060)		+= arch/m68k/ifpsp060/

all:	vmlinux

