/*
** asm/setup.h -- Definition of the Linux/m68k setup information
**
** Copyright 1992 by Greg Harp
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
** Created 09/29/92 by Greg Harp
**
** 5/2/94 Roman Hodek:
**   Added bi_atari part of the machine dependent union bi_un; for now it
**   contains just a model field to distinguish between TT and Falcon.
** 26/7/96 Roman Zippel:
**   Renamed to setup.h; added some useful macros to allow gcc some
**   optimizations if possible.
** 5/10/96 Geert Uytterhoeven:
**   Redesign of the boot information structure; moved boot information
**   structure to bootinfo.h
*/
#ifndef _M68K_SETUP_H
#define _M68K_SETUP_H

#include <uapi/asm/bootinfo.h>
#include <uapi/asm/setup.h>


#define CL_SIZE COMMAND_LINE_SIZE

#ifndef __ASSEMBLY__
extern unsigned long m68k_machtype;
#endif /* !__ASSEMBLY__ */

#define MACH_ATX040_ONLY
#define MACH_IS_ATX040 (1)
#define MACH_TYPE (MACH_ATX040)

#ifndef __ASSEMBLY__
extern unsigned long m68k_cputype;
extern unsigned long m68k_fputype;
extern unsigned long m68k_mmutype;

    /*
     *  m68k_is040or060 is != 0 for a '040 or higher;
     *  used numbers are 4 for 68040 and 6 for 68060.
     */

extern int m68k_is040or060;
#endif /* !__ASSEMBLY__ */

#define CPU_IS_020 (0)
#define MMU_IS_851 (0)

#if !defined(CONFIG_M68030)
#  define CPU_IS_030 (0)
#  define MMU_IS_030 (0)
#elif defined(CONFIG_M68040) || defined(CONFIG_M68060)
#  define CPU_IS_030 (m68k_cputype & CPU_68030)
#  define MMU_IS_030 (m68k_mmutype & MMU_68030)
#else
#  define CPU_M68030_ONLY
#  define CPU_IS_030 (1)
#  define MMU_IS_030 (1)
#endif

#if !defined(CONFIG_M68040)
#  define CPU_IS_040 (0)
#  define MMU_IS_040 (0)
#elif defined(CONFIG_M68030) || defined(CONFIG_M68060)
#  define CPU_IS_040 (m68k_cputype & CPU_68040)
#  define MMU_IS_040 (m68k_mmutype & MMU_68040)
#else
#  define CPU_M68040_ONLY
#  define CPU_IS_040 (1)
#  define MMU_IS_040 (1)
#endif

#if !defined(CONFIG_M68060)
#  define CPU_IS_060 (0)
#  define MMU_IS_060 (0)
#elif defined(CONFIG_M68030) || defined(CONFIG_M68040)
#  define CPU_IS_060 (m68k_cputype & CPU_68060)
#  define MMU_IS_060 (m68k_mmutype & MMU_68060)
#else
#  define CPU_M68060_ONLY
#  define CPU_IS_060 (1)
#  define MMU_IS_060 (1)
#endif

#if !defined(CONFIG_M68020) && !defined(CONFIG_M68030)
#  define CPU_IS_020_OR_030 (0)
#else
#  define CPU_M68020_OR_M68030
#  if defined(CONFIG_M68040) || defined(CONFIG_M68060)
#    define CPU_IS_020_OR_030 (!m68k_is040or060)
#  else
#    define CPU_M68020_OR_M68030_ONLY
#    define CPU_IS_020_OR_030 (1)
#  endif
#endif

#if !defined(CONFIG_M68040) && !defined(CONFIG_M68060)
#  define CPU_IS_040_OR_060 (0)
#else
#  define CPU_M68040_OR_M68060
#  if defined(CONFIG_M68020) || defined(CONFIG_M68030)
#    define CPU_IS_040_OR_060 (m68k_is040or060)
#  else
#    define CPU_M68040_OR_M68060_ONLY
#    define CPU_IS_040_OR_060 (1)
#  endif
#endif

#define CPU_TYPE (m68k_cputype)

#define FPU_IS_EMU (0)


    /*
     *  Miscellaneous
     */

#define NUM_MEMINFO	4

#ifndef __ASSEMBLY__
struct m68k_mem_info {
	unsigned long addr;		/* physical address of memory chunk */
	unsigned long size;		/* length of memory chunk (in bytes) */
};

extern int m68k_num_memory;		/* # of memory blocks found (and used) */
extern int m68k_realnum_memory;		/* real # of memory blocks found */
extern struct m68k_mem_info m68k_memory[NUM_MEMINFO];/* memory description */
#endif

#endif /* _M68K_SETUP_H */
