# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#

extra-y := head.o vmlinux.lds
obj-y	:= entry.o irq.o module.o process.o ptrace.o
obj-y	+= setup.o signal.o sys_m68k.o syscalltable.o time.o traps.o
obj-y   += ints.o vectors.o cpu.o

obj-$(CONFIG_HAS_DMA)	+= dma.o

