/* -*- mode: asm -*-
**
** head.S -- This file contains the initial boot code for the
**	     Linux/68k kernel.
**
** Copyright 1993 by Hamish Macdonald
**
** 68040 fixes by Michael Rausch
** 68060 fixes by Roman Hodek
** MMU cleanup by Randy Thelen
** Final MMU cleanup by Roman Zippel
**
** Atari support by Andreas Schwab, using ideas of Robert de Vries
** and Bjoern Brauel
** VME Support by Richard Hirst
**
** 94/11/14 Andreas Schwab: put kernel at PAGESIZE
** 94/11/18 Andreas Schwab: remove identity mapping of STRAM for Atari
** ++ Bjoern & Roman: ATARI-68040 support for the Medusa
** 95/11/18 Richard Hirst: Added MVME166 support
** 96/04/26 Guenther Kelleter: fixed identity mapping for Falcon with
**			      Magnum- and FX-alternate ram
** 98/04/25 Phil Blundell: added HP300 support
** 1998/08/30 David Kilzer: Added support for font_desc structures
**            for linux-2.1.115
** 1999/02/11  Richard Zidlicky: added Q40 support (initial version 99/01/01)
** 2004/05/13 Kars de Jong: Finalised HP300 support
**
** This file is subject to the terms and conditions of the GNU General Public
** License. See the file README.legal in the main directory of this archive
** for more details.
**
*/

/*
 * Linux startup code.
 *
 * At this point, the boot loader has:
 * Disabled interrupts
 * Disabled caches
 * Put us in supervisor state.
 *
 * The kernel setup code takes the following steps:
 * .  Raise interrupt level
 * .  Set up initial kernel memory mapping.
 *    .  This sets up a mapping of the 4M of memory the kernel is located in.
 *    .  It also does a mapping of any initial machine specific areas.
 * .  Enable the MMU
 * .  Enable cache memories
 * .  Jump to kernel startup
 *
 * Much of the file restructuring was to accomplish:
 * 1) Remove register dependency through-out the file.
 * 2) Increase use of subroutines to perform functions
 * 3) Increase readability of the code
 *
 * Of course, readability is a subjective issue, so it will never be
 * argued that that goal was accomplished.  It was merely a goal.
 * A key way to help make code more readable is to give good
 * documentation.  So, the first thing you will find is exaustive
 * write-ups on the structure of the file, and the features of the
 * functional subroutines.
 *
 * General Structure:
 * ------------------
 *	Without a doubt the single largest chunk of head.S is spent
 * mapping the kernel and I/O physical space into the logical range
 * for the kernel.
 *	There are new subroutines and data structures to make MMU
 * support cleaner and easier to understand.
 *	First, you will find a routine call "mmu_map" which maps
 * a logical to a physical region for some length given a cache
 * type on behalf of the caller.  This routine makes writing the
 * actual per-machine specific code very simple.
 *	A central part of the code, but not a subroutine in itself,
 * is the mmu_init code which is broken down into mapping the kernel
 * (the same for all machines) and mapping machine-specific I/O
 * regions.
 *	Also, there will be a description of engaging the MMU and
 * caches.
 *	You will notice that there is a chunk of code which
 * can emit the entire MMU mapping of the machine.  This is present
 * only in debug modes and can be very helpful.
 *	Further, there is a new console driver in head.S that is
 * also only engaged in debug mode.  Currently, it's only supported
 * on the Macintosh class of machines.  However, it is hoped that
 * others will plug-in support for specific machines.
 *
 * ######################################################################
 *
 * mmu_map
 * -------
 *	mmu_map was written for two key reasons.  First, it was clear
 * that it was very difficult to read the previous code for mapping
 * regions of memory.  Second, the Macintosh required such extensive
 * memory allocations that it didn't make sense to propagate the
 * existing code any further.
 *	mmu_map requires some parameters:
 *
 *	mmu_map (logical, physical, length, cache_type)
 *
 *	While this essentially describes the function in the abstract, you'll
 * find more indepth description of other parameters at the implementation site.
 *
 * mmu_get_root_table_entry
 * ------------------------
 * mmu_get_ptr_table_entry
 * -----------------------
 * mmu_get_page_table_entry
 * ------------------------
 *
 *	These routines are used by other mmu routines to get a pointer into
 * a table, if necessary a new table is allocated. These routines are working
 * basically like pmd_alloc() and pte_alloc() in <asm/pgtable.h>. The root
 * table needs of course only to be allocated once in mmu_get_root_table_entry,
 * so that here also some mmu specific initialization is done. The second page
 * at the start of the kernel (the first page is unmapped later) is used for
 * the kernel_pg_dir. It must be at a position known at link time (as it's used
 * to initialize the init task struct) and since it needs special cache
 * settings, it's the easiest to use this page, the rest of the page is used
 * for further pointer tables.
 * mmu_get_page_table_entry allocates always a whole page for page tables, this
 * means 1024 pages and so 4MB of memory can be mapped. It doesn't make sense
 * to manage page tables in smaller pieces as nearly all mappings have that
 * size.
 *
 * ######################################################################
 *
 *
 * ######################################################################
 *
 * mmu_engage
 * ----------
 *	Thanks to a small helping routine enabling the mmu got quite simple
 * and there is only one way left. mmu_engage makes a complete a new mapping
 * that only includes the absolute necessary to be able to jump to the final
 * position and to restore the original mapping.
 * As this code doesn't need a transparent translation register anymore this
 * means all registers are free to be used by machines that needs them for
 * other purposes.
 *
 * ######################################################################
 *
 *	Register usage has greatly simplified within head.S. Every subroutine
 * saves and restores all registers that it modifies (except it returns a
 * value in there of course). So the only register that needs to be initialized
 * is the stack pointer.
 * All other init code and data is now placed in the init section, so it will
 * be automatically freed at the end of the kernel initialization.
 *
 * ######################################################################
 *
 * options
 * -------
 *	There are many options available in a build of this file.  I've
 * taken the time to describe them here to save you the time of searching
 * for them and trying to understand what they mean.
 *
 * CONFIG_xxx:	These are the obvious machine configuration defines created
 * during configuration.  These are defined in autoconf.h.
 *
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/bootinfo.h>
#include <asm/bootinfo-atx040.h>
#include <asm/setup.h>
#include <asm/entry.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/asm-offsets.h>
#ifdef CONFIG_ATX040
#  include <asm/atx040hw.h>
#endif

.globl kernel_pg_dir
.globl availmem
.globl m68k_init_mapped_size
.globl m68k_pgtable_cachemode
.globl m68k_supervisor_cachemode

CPUTYPE_040	= 1	/* indicates an 040 */
CPUTYPE_060	= 2	/* indicates an 060 */
CPUTYPE_0460	= 3	/* if either above are set, this is set */
CPUTYPE_020	= 4	/* indicates an 020 */

/* Translation control register */
TC_ENABLE = 0x8000
TC_PAGE8K = 0x4000
TC_PAGE4K = 0x0000

/* Transparent translation registers */
TTR_ENABLE	= 0x8000	/* enable transparent translation */
TTR_ANYMODE	= 0x4000	/* user and kernel mode access */
TTR_KERNELMODE	= 0x2000	/* only kernel mode access */
TTR_USERMODE	= 0x0000	/* only user mode access */
TTR_CI		= 0x0400	/* inhibit cache */
TTR_RW		= 0x0200	/* read/write mode */
TTR_RWM		= 0x0100	/* read/write mask */
TTR_FCB2	= 0x0040	/* function code base bit 2 */
TTR_FCB1	= 0x0020	/* function code base bit 1 */
TTR_FCB0	= 0x0010	/* function code base bit 0 */
TTR_FCM2	= 0x0004	/* function code mask bit 2 */
TTR_FCM1	= 0x0002	/* function code mask bit 1 */
TTR_FCM0	= 0x0001	/* function code mask bit 0 */

/* Cache Control registers */
CC6_ENABLE_D	= 0x80000000	/* enable data cache (680[46]0) */
CC6_FREEZE_D	= 0x40000000	/* freeze data cache (68060) */
CC6_ENABLE_SB	= 0x20000000	/* enable store buffer (68060) */
CC6_PUSH_DPI	= 0x10000000	/* disable CPUSH invalidation (68060) */
CC6_HALF_D	= 0x08000000	/* half-cache mode for data cache (68060) */
CC6_ENABLE_B	= 0x00800000	/* enable branch cache (68060) */
CC6_CLRA_B	= 0x00400000	/* clear all entries in branch cache (68060) */
CC6_CLRU_B	= 0x00200000	/* clear user entries in branch cache (68060) */
CC6_ENABLE_I	= 0x00008000	/* enable instruction cache (680[46]0) */
CC6_FREEZE_I	= 0x00004000	/* freeze instruction cache (68060) */
CC6_HALF_I	= 0x00002000	/* half-cache mode for instruction cache (68060) */
CC3_ALLOC_WRITE	= 0x00002000	/* write allocate mode(68030) */
CC3_ENABLE_DB	= 0x00001000	/* enable data burst (68030) */
CC3_CLR_D	= 0x00000800	/* clear data cache (68030) */
CC3_CLRE_D	= 0x00000400	/* clear entry in data cache (68030) */
CC3_FREEZE_D	= 0x00000200	/* freeze data cache (68030) */
CC3_ENABLE_D	= 0x00000100	/* enable data cache (68030) */
CC3_ENABLE_IB	= 0x00000010	/* enable instruction burst (68030) */
CC3_CLR_I	= 0x00000008	/* clear instruction cache (68030) */
CC3_CLRE_I	= 0x00000004	/* clear entry in instruction cache (68030) */
CC3_FREEZE_I	= 0x00000002	/* freeze instruction cache (68030) */
CC3_ENABLE_I	= 0x00000001	/* enable instruction cache (68030) */

/* Miscellaneous definitions */
PAGESIZE	= 4096
PAGESHIFT	= 12

ROOT_TABLE_SIZE	= 128
PTR_TABLE_SIZE	= 128
PAGE_TABLE_SIZE	= 64
ROOT_INDEX_SHIFT = 25
PTR_INDEX_SHIFT  = 18
PAGE_INDEX_SHIFT = 12

#define L(name) .L##name

/* The __INITDATA stuff is a no-op when ftrace or kgdb are turned on */
#ifndef __INITDATA
#define __INITDATA	.data
#define __FINIT		.previous
#endif

/* Several macros to make the writing of subroutines easier:
 * - func_start marks the beginning of the routine which setups the frame
 *   register and saves the registers, it also defines another macro
 *   to automatically restore the registers again.
 * - func_return marks the end of the routine and simply calls the prepared
 *   macro to restore registers and jump back to the caller.
 * - func_define generates another macro to automatically put arguments
 *   onto the stack call the subroutine and cleanup the stack again.
 */

/* Within subroutines these macros can be used to access the arguments
 * on the stack. With STACK some allocated memory on the stack can be
 * accessed and ARG0 points to the return address (used by mmu_engage).
 */
#define	STACK	%a6@(stackstart)
#define ARG0	%a6@(4)
#define ARG1	%a6@(8)
#define ARG2	%a6@(12)
#define ARG3	%a6@(16)
#define ARG4	%a6@(20)

.macro	func_start	name,saveregs,stack=0
L(\name):
	linkw	%a6,#-\stack
	moveml	\saveregs,%sp@-
.set	stackstart,-\stack

.macro	func_return_\name
	moveml	%sp@+,\saveregs
	unlk	%a6
	rts
.endm
.endm

.macro	func_return	name
	func_return_\name
.endm

.macro	func_call	name
	jbsr	L(\name)
.endm

.macro	move_stack	nr,arg1,arg2,arg3,arg4
.if	\nr
	move_stack	"(\nr-1)",\arg2,\arg3,\arg4
	movel	\arg1,%sp@-
.endif
.endm

.macro	func_define	name,nr=0
.macro	\name	arg1,arg2,arg3,arg4
	move_stack	\nr,\arg1,\arg2,\arg3,\arg4
	func_call	\name
.if	\nr
	lea	%sp@(\nr*4),%sp
.endif
.endm
.endm

func_define	mmu_map,4
func_define	mmu_map_tt,4
func_define	mmu_fixup_page_mmu_cache,1
func_define	mmu_temp_map,2
func_define	mmu_engage
func_define	mmu_get_root_table_entry,1
func_define	mmu_get_ptr_table_entry,2
func_define	mmu_get_page_table_entry,2
func_define	get_new_page

.macro	mmu_map_eq	arg1,arg2,arg3
	mmu_map	\arg1,\arg1,\arg2,\arg3
.endm

.macro	get_bi_record	record
	pea	\record
	func_call	get_bi_record
	addql	#4,%sp
.endm

#define is_not_atx040(lab) cmpl &MACH_ATX040,%pc@(m68k_machtype); jne lab

#define is_040_or_060(lab)	btst &CPUTYPE_0460,%pc@(L(cputype)+3); jne lab
#define is_not_040_or_060(lab)	btst &CPUTYPE_0460,%pc@(L(cputype)+3); jeq lab
#define is_040(lab)		btst &CPUTYPE_040,%pc@(L(cputype)+3); jne lab
#define is_060(lab)		btst &CPUTYPE_060,%pc@(L(cputype)+3); jne lab
#define is_not_060(lab)		btst &CPUTYPE_060,%pc@(L(cputype)+3); jeq lab
#define is_020(lab)		btst &CPUTYPE_020,%pc@(L(cputype)+3); jne lab
#define is_not_020(lab)		btst &CPUTYPE_020,%pc@(L(cputype)+3); jeq lab

__HEAD
ENTRY(_stext)
/*
 * Version numbers of the bootinfo interface
 * The area from _stext to _start will later be used as kernel pointer table
 */
	bras	1f	/* Jump over bootinfo version numbers */

	.long	BOOTINFOV_MAGIC
	.long	MACH_ATX040, ATX040_BOOTI_VERSION
	.long	0
1:	jra	__start

.equ	kernel_pg_dir,_stext

.equ	.,_stext+PAGESIZE

ENTRY(_start)
	jra	__start
__INIT
ENTRY(__start)

	movew	#0x2700,%sr

/*
 * Setup initial stack pointer
 */
	lea	%pc@(_stext),%sp

/*
 * Record the CPU and machine type.
 */
	get_bi_record	BI_MACHTYPE
	lea	%pc@(m68k_machtype),%a1
	movel	%a0@,%a1@

	get_bi_record	BI_FPUTYPE
	lea	%pc@(m68k_fputype),%a1
	movel	%a0@,%a1@

	get_bi_record	BI_MMUTYPE
	lea	%pc@(m68k_mmutype),%a1
	movel	%a0@,%a1@

	get_bi_record	BI_CPUTYPE
	lea	%pc@(m68k_cputype),%a1
	movel	%a0@,%a1@

/*
 * There are ultimately two pieces of information we want for all kinds of
 * processors CpuType and CacheBits.  The CPUTYPE was passed in from booter
 * and is converted here from a booter type definition to a separate bit
 * number which allows for the standard is_0x0 macro tests.
 */
	movel	%pc@(m68k_cputype),%d0
	/*
	 * Assume it's an 030
	 */
	clrl	%d1

	/*
	 * Test the BootInfo cputype for 060
	 */
	btst	#CPUB_68060,%d0
	jeq	1f
	bset	#CPUTYPE_060,%d1
	bset	#CPUTYPE_0460,%d1
	jra	3f
1:
	/*
	 * Test the BootInfo cputype for 040
	 */
	btst	#CPUB_68040,%d0
	jeq	2f
	bset	#CPUTYPE_040,%d1
	bset	#CPUTYPE_0460,%d1
	jra	3f
2:
	/*
	 * Test the BootInfo cputype for 020
	 */
	btst	#CPUB_68020,%d0
	jeq	3f
	bset	#CPUTYPE_020,%d1
	jra	3f
3:
	/*
	 * Record the cpu type
	 */
	lea	%pc@(L(cputype)),%a0
	movel	%d1,%a0@

	/*
	 * NOTE:
	 *
	 * Now the macros are valid:
	 *	is_040_or_060
	 *	is_not_040_or_060
	 *	is_040
	 *	is_060
	 *	is_not_060
	 */

	/*
	 * Determine the cache mode for pages holding MMU tables
	 * and for supervisor mode, unused for '020 and '030
	 */
	clrl	%d0
	clrl	%d1

	is_not_040_or_060(L(save_cachetype))

	/*
	 * '040 or '060
	 * d1 := cacheable write-through
	 * NOTE: The 68040 manual strongly recommends non-cached for MMU tables,
	 * but we have been using write-through since at least 2.0.29 so I
	 * guess it is OK.
	 */
#ifdef CONFIG_060_WRITETHROUGH
	/*
	 * If this is a 68060 board using drivers with cache coherency
	 * problems, then supervisor memory accesses need to be write-through
	 * also; otherwise, we want copyback.
	 */

	is_not_060(1f)
	movel	#_PAGE_CACHE040W,%d0
	jra	L(save_cachetype)
#endif /* CONFIG_060_WRITETHROUGH */
1:
	movew	#_PAGE_CACHE040,%d0

	movel	#_PAGE_CACHE040W,%d1

L(save_cachetype):
	/* Save cache mode for supervisor mode and page tables
	 */
	lea	%pc@(m68k_supervisor_cachemode),%a0
	movel	%d0,%a0@
	lea	%pc@(m68k_pgtable_cachemode),%a0
	movel	%d1,%a0@

/*
 * raise interrupt level
 */
	movew	#0x2700,%sr

/*
 * Save physical start address of kernel
 */
	lea	%pc@(L(phys_kernel_start)),%a0
	lea	%pc@(_stext),%a1
	subl	#_stext,%a1
	addl	#PAGE_OFFSET,%a1
	movel	%a1,%a0@

/*
 *	mmu_init
 *
 *	This block of code does what's necessary to map in the various kinds
 *	of machines for execution of Linux.
 *	First map the first 4, 8, or 16 MB of kernel code & data
 */

	get_bi_record BI_MEMCHUNK
	movel	%a0@(4),%d0
	movel	#16*1024*1024,%d1
	cmpl	%d0,%d1
	jls	1f
	lsrl	#1,%d1
	cmpl	%d0,%d1
	jls	1f
	lsrl	#1,%d1
1:
	lea	%pc@(m68k_init_mapped_size),%a0
	movel	%d1,%a0@
	mmu_map	#PAGE_OFFSET,%pc@(L(phys_kernel_start)),%d1,\
		%pc@(m68k_supervisor_cachemode)

#ifdef CONFIG_ATX040
	is_not_atx040(L(notatx040))

	move.l #0, %d0
	movec.l %d0, %dtt0
	movec.l %d0, %dtt1
	movec.l %d0, %itt0
	movec.l %d0, %itt1

	jbra	L(mmu_init_done)

L(notatx040):
#endif

L(mmu_init_done):

/*
 * mmu_fixup
 *
 * On the 040 class machines, all pages that are used for the
 * mmu have to be fixed up. According to Motorola, pages holding mmu
 * tables should be non-cacheable on a '040 and write-through on a
 * '060. But analysis of the reasons for this, and practical
 * experience, showed that write-through also works on a '040.
 *
 * Allocated memory so far goes from kernel_end to memory_start that
 * is used for all kind of tables, for that the cache attributes
 * are now fixed.
 */
L(mmu_fixup):

	is_not_040_or_060(L(mmu_fixup_done))

	/* first fix the page at the start of the kernel, that
	 * contains also kernel_pg_dir.
	 */
	movel	%pc@(L(phys_kernel_start)),%d0
	subl	#PAGE_OFFSET,%d0
	lea	%pc@(_stext),%a0
	subl	%d0,%a0
	mmu_fixup_page_mmu_cache	%a0

	movel	%pc@(L(kernel_end)),%a0
	subl	%d0,%a0
	movel	%pc@(L(memory_start)),%a1
	subl	%d0,%a1
	bra	2f
1:
	mmu_fixup_page_mmu_cache	%a0
	addw	#PAGESIZE,%a0
2:
	cmpl	%a0,%a1
	jgt	1b

L(mmu_fixup_done):

/*
 * mmu_engage
 *
 * This chunk of code performs the gruesome task of engaging the MMU.
 * The reason its gruesome is because when the MMU becomes engaged it
 * maps logical addresses to physical addresses.  The Program Counter
 * register is then passed through the MMU before the next instruction
 * is fetched (the instruction following the engage MMU instruction).
 * This may mean one of two things:
 * 1. The Program Counter falls within the logical address space of
 *    the kernel of which there are two sub-possibilities:
 *    A. The PC maps to the correct instruction (logical PC == physical
 *       code location), or
 *    B. The PC does not map through and the processor will read some
 *       data (or instruction) which is not the logically next instr.
 *    As you can imagine, A is good and B is bad.
 * Alternatively,
 * 2. The Program Counter does not map through the MMU.  The processor
 *    will take a Bus Error.
 * Clearly, 2 is bad.
 * It doesn't take a wiz kid to figure you want 1.A.
 * This code creates that possibility.
 * There are two possible 1.A. states (we now ignore the other above states):
 * A. The kernel is located at physical memory addressed the same as
 *    the logical memory for the kernel, i.e., 0x01000.
 * B. The kernel is located some where else.  e.g., 0x0400.0000
 *
 *    Under some conditions the Macintosh can look like A or B.
 * [A friend and I once noted that Apple hardware engineers should be
 * wacked twice each day: once when they show up at work (as in, Whack!,
 * "This is for the screwy hardware we know you're going to design today."),
 * and also at the end of the day (as in, Whack! "I don't know what
 * you designed today, but I'm sure it wasn't good."). -- rst]
 *
 * This code works on the following premise:
 * If the kernel start (%d5) is within the first 16 Meg of RAM,
 * then create a mapping for the kernel at logical 0x8000.0000 to
 * the physical location of the pc.  And, create a transparent
 * translation register for the first 16 Meg.  Then, after the MMU
 * is engaged, the PC can be moved up into the 0x8000.0000 range
 * and then the transparent translation can be turned off and then
 * the PC can jump to the correct logical location and it will be
 * home (finally).  This is essentially the code that the Amiga used
 * to use.  Now, it's generalized for all processors.  Which means
 * that a fresh (but temporary) mapping has to be created.  The mapping
 * is made in page 0 (an as of yet unused location -- except for the
 * stack!).  This temporary mapping will only require 1 pointer table
 * and a single page table (it can map 256K).
 *
 * OK, alternatively, imagine that the Program Counter is not within
 * the first 16 Meg.  Then, just use Transparent Translation registers
 * to do the right thing.
 *
 * Last, if _start is already at 0x01000, then there's nothing special
 * to do (in other words, in a degenerate case of the first case above,
 * do nothing).
 *
 * Let's do it.
 *
 *
 */

	mmu_engage

/*
 * After this point no new memory is allocated and
 * the start of available memory is stored in availmem.
 * (The bootmem allocator requires now the physicall address.)
 */

	movel	L(memory_start),availmem

/*
 * Enable caches
 */

	is_not_040_or_060(L(cache_not_680460))

L(cache680460):
	.chip	68040
	nop
	cpusha	%bc
	nop

	is_060(L(cache68060))

	movel	#CC6_ENABLE_D+CC6_ENABLE_I,%d0
	/* MMU stuff works in copyback mode now, so enable the cache */
	movec	%d0,%cacr
	jra	L(cache_done)

L(cache68060):
	movel	#CC6_ENABLE_D+CC6_ENABLE_I+CC6_ENABLE_SB+CC6_PUSH_DPI+CC6_ENABLE_B+CC6_CLRA_B,%d0
	/* MMU stuff works in copyback mode now, so enable the cache */
	movec	%d0,%cacr
	/* enable superscalar dispatch in PCR */
	moveq	#1,%d0
	.chip	68060
	movec	%d0,%pcr

	jbra	L(cache_done)
L(cache_not_680460):
L(cache68030):
	.chip	68030
	movel	#CC3_ENABLE_DB+CC3_CLR_D+CC3_ENABLE_D+CC3_ENABLE_IB+CC3_CLR_I+CC3_ENABLE_I,%d0
	movec	%d0,%cacr

	jra	L(cache_done)
	.chip	68k
L(cache_done):

/*
 * Setup initial stack pointer
 */
	lea	init_task,%curptr
	lea	init_thread_union+THREAD_SIZE,%sp

	subl	%a6,%a6		/* clear a6 for gdb */

/*
 * The new 64bit printf support requires an early exception initialization.
 */
	jbsr	base_trap_init

/* jump to the kernel start */

	jbsr	start_kernel

/*
 * Find a tag record in the bootinfo structure
 * The bootinfo structure is located right after the kernel
 * Returns: d0: size (-1 if not found)
 *          a0: data pointer (end-of-records if not found)
 */
func_start	get_bi_record,%d1

	movel	ARG1,%d0
	lea	%pc@(_end),%a0
1:	tstw	%a0@(BIR_TAG)
	jeq	3f
	cmpw	%a0@(BIR_TAG),%d0
	jeq	2f
	addw	%a0@(BIR_SIZE),%a0
	jra	1b
2:	moveq	#0,%d0
	movew	%a0@(BIR_SIZE),%d0
	lea	%a0@(BIR_DATA),%a0
	jra	4f
3:	moveq	#-1,%d0
	lea	%a0@(BIR_SIZE),%a0
4:
func_return	get_bi_record


/*
 *	MMU Initialization Begins Here
 *
 *	The structure of the MMU tables on the 68k machines
 *	is thus:
 *	Root Table
 *		Logical addresses are translated through
 *	a hierarchical translation mechanism where the high-order
 *	seven bits of the logical address (LA) are used as an
 *	index into the "root table."  Each entry in the root
 *	table has a bit which specifies if it's a valid pointer to a
 *	pointer table.  Each entry defines a 32KMeg range of memory.
 *	If an entry is invalid then that logical range of 32M is
 *	invalid and references to that range of memory (when the MMU
 *	is enabled) will fault.  If the entry is valid, then it does
 *	one of two things.  On 040/060 class machines, it points to
 *	a pointer table which then describes more finely the memory
 *	within that 32M range.  On 020/030 class machines, a technique
 *	called "early terminating descriptors" are used.  This technique
 *	allows an entire 32Meg to be described by a single entry in the
 *	root table.  Thus, this entry in the root table, contains the
 *	physical address of the memory or I/O at the logical address
 *	which the entry represents and it also contains the necessary
 *	cache bits for this region.
 *
 *	Pointer Tables
 *		Per the Root Table, there will be one or more
 *	pointer tables.  Each pointer table defines a 32M range.
 *	Not all of the 32M range need be defined.  Again, the next
 *	seven bits of the logical address are used an index into
 *	the pointer table to point to page tables (if the pointer
 *	is valid).  There will undoubtedly be more than one
 *	pointer table for the kernel because each pointer table
 *	defines a range of only 32M.  Valid pointer table entries
 *	point to page tables, or are early terminating entries
 *	themselves.
 *
 *	Page Tables
 *		Per the Pointer Tables, each page table entry points
 *	to the physical page in memory that supports the logical
 *	address that translates to the particular index.
 *
 *	In short, the Logical Address gets translated as follows:
 *		bits 31..26 - index into the Root Table
 *		bits 25..18 - index into the Pointer Table
 *		bits 17..12 - index into the Page Table
 *		bits 11..0  - offset into a particular 4K page
 *
 *	The algorithms which follows do one thing: they abstract
 *	the MMU hardware.  For example, there are three kinds of
 *	cache settings that are relevant.  Either, memory is
 *	being mapped in which case it is either Kernel Code (or
 *	the RamDisk) or it is MMU data.  On the 030, the MMU data
 *	option also describes the kernel.  Or, I/O is being mapped
 *	in which case it has its own kind of cache bits.  There
 *	are constants which abstract these notions from the code that
 *	actually makes the call to map some range of memory.
 *
 *
 *
 */

/*
 * mmu_map_tt
 *
 * This is a specific function which works on all 680x0 machines.
 * On 030, 040 & 060 it will attempt to use Transparent Translation
 * registers (tt1).
 * On 020 it will call the standard mmu_map which will use early
 * terminating descriptors.
 */
func_start	mmu_map_tt,%d0/%d1/%a0,4

	is_020(L(do_map))

	/* Extract the highest bit set
	 */
	bfffo	ARG3{#0,#32},%d1
	cmpw	#8,%d1
	jcc	L(do_map)

	/* And get the mask
	 */
	moveq	#-1,%d0
	lsrl	%d1,%d0
	lsrl	#1,%d0

	/* Mask the address
	 */
	movel	%d0,%d1
	notl	%d1
	andl	ARG2,%d1

	/* Generate the upper 16bit of the tt register
	 */
	lsrl	#8,%d0
	orl	%d0,%d1
	clrw	%d1

	is_040_or_060(L(mmu_map_tt_040))

	/* set 030 specific bits (read/write access for supervisor mode
	 * (highest function code set, lower two bits masked))
	 */
	orw	#TTR_ENABLE+TTR_RWM+TTR_FCB2+TTR_FCM1+TTR_FCM0,%d1
	movel	ARG4,%d0
	btst	#6,%d0
	jeq	1f
	orw	#TTR_CI,%d1

1:	lea	STACK,%a0
	movel	%d1,%a0@
	.chip	68030
	tstl	ARG1
	jne	1f
	pmove	%a0@,%tt0
	jra	2f
1:	pmove	%a0@,%tt1
2:	.chip	68k
	jra	L(mmu_map_tt_done)

	/* set 040 specific bits
	 */
L(mmu_map_tt_040):
	orw	#TTR_ENABLE+TTR_KERNELMODE,%d1
	orl	ARG4,%d1

	.chip	68040
	tstl	ARG1
	jne	1f
	movec	%d1,%itt0
	movec	%d1,%dtt0
	jra	2f
1:	movec	%d1,%itt1
	movec	%d1,%dtt1
2:	.chip	68k

	jra	L(mmu_map_tt_done)

L(do_map):
	mmu_map_eq	ARG2,ARG3,ARG4

L(mmu_map_tt_done):

func_return	mmu_map_tt

/*
 *	mmu_map
 *
 *	This routine will map a range of memory using a pointer
 *	table and allocating the pages on the fly from the kernel.
 *	The pointer table does not have to be already linked into
 *	the root table, this routine will do that if necessary.
 *
 *	NOTE
 *	This routine will assert failure and use the serial_putc
 *	routines in the case of a run-time error.  For example,
 *	if the address is already mapped.
 *
 *	NOTE-2
 *	This routine will use early terminating descriptors
 *	where possible for the 68020+68851 and 68030 type
 *	processors.
 */
func_start	mmu_map,%d0-%d4/%a0-%a4

	/* Get logical address and round it down to 256KB
	 */
	movel	ARG1,%d0
	andl	#-(PAGESIZE*PAGE_TABLE_SIZE),%d0
	movel	%d0,%a3

	/* Get the end address
	 */
	movel	ARG1,%a4
	addl	ARG3,%a4
	subql	#1,%a4

	/* Get physical address and round it down to 256KB
	 */
	movel	ARG2,%d0
	andl	#-(PAGESIZE*PAGE_TABLE_SIZE),%d0
	movel	%d0,%a2

	/* Add page attributes to the physical address
	 */
	movel	ARG4,%d0
	orw	#_PAGE_PRESENT+_PAGE_ACCESSED+_PAGE_DIRTY,%d0
	addw	%d0,%a2

	is_not_040_or_060(L(mmu_map_030))

	addw	#_PAGE_GLOBAL040,%a2
/*
 *	MMU 040 & 060 Support
 *
 *	The MMU usage for the 040 and 060 is different enough from
 *	the 030 and 68851 that there is separate code.  This comment
 *	block describes the data structures and algorithms built by
 *	this code.
 *
 *	The 040 does not support early terminating descriptors, as
 *	the 030 does.  Therefore, a third level of table is needed
 *	for the 040, and that would be the page table.  In Linux,
 *	page tables are allocated directly from the memory above the
 *	kernel.
 *
 */

L(mmu_map_040):
	/* Calculate the offset into the root table
	 */
	movel	%a3,%d0
	moveq	#ROOT_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	mmu_get_root_table_entry	%d0

	/* Calculate the offset into the pointer table
	 */
	movel	%a3,%d0
	moveq	#PTR_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	andl	#PTR_TABLE_SIZE-1,%d0
	mmu_get_ptr_table_entry		%a0,%d0

	/* Calculate the offset into the page table
	 */
	movel	%a3,%d0
	moveq	#PAGE_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	andl	#PAGE_TABLE_SIZE-1,%d0
	mmu_get_page_table_entry	%a0,%d0

	/* The page table entry must not no be busy
	 */
	tstl	%a0@
	jne	L(mmu_map_error)

	/* Do the mapping and advance the pointers
	 */
	movel	%a2,%a0@
2:
	addw	#PAGESIZE,%a2
	addw	#PAGESIZE,%a3

	/* Ready with mapping?
	 */
	lea	%a3@(-1),%a0
	cmpl	%a0,%a4
	jhi	L(mmu_map_040)
	jra	L(mmu_map_done)

L(mmu_map_030):
	/* Calculate the offset into the root table
	 */
	movel	%a3,%d0
	moveq	#ROOT_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	mmu_get_root_table_entry	%d0

	/* Check if logical address 32MB aligned,
	 * so we can try to map it once
	 */
	movel	%a3,%d0
	andl	#(PTR_TABLE_SIZE*PAGE_TABLE_SIZE*PAGESIZE-1)&(-ROOT_TABLE_SIZE),%d0
	jne	1f

	/* Is there enough to map for 32MB at once
	 */
	lea	%a3@(PTR_TABLE_SIZE*PAGE_TABLE_SIZE*PAGESIZE-1),%a1
	cmpl	%a1,%a4
	jcs	1f

	addql	#1,%a1

	/* The root table entry must not no be busy
	 */
	tstl	%a0@
	jne	L(mmu_map_error)

	/* Do the mapping and advance the pointers
	 */
	movel	%a2,%a0@

	movel	%a1,%a3
	lea	%a2@(PTR_TABLE_SIZE*PAGE_TABLE_SIZE*PAGESIZE),%a2
	jra	L(mmu_mapnext_030)
1:
	/* Calculate the offset into the pointer table
	 */
	movel	%a3,%d0
	moveq	#PTR_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	andl	#PTR_TABLE_SIZE-1,%d0
	mmu_get_ptr_table_entry		%a0,%d0

	/* The pointer table entry must not no be busy
	 */
	tstl	%a0@
	jne	L(mmu_map_error)

	/* Do the mapping and advance the pointers
	 */
	movel	%a2,%a0@

	addl	#PAGE_TABLE_SIZE*PAGESIZE,%a2
	addl	#PAGE_TABLE_SIZE*PAGESIZE,%a3

L(mmu_mapnext_030):
	/* Ready with mapping?
	 */
	lea	%a3@(-1),%a0
	cmpl	%a0,%a4
	jhi	L(mmu_map_030)
	jra	L(mmu_map_done)

L(mmu_map_error):

L(mmu_map_done):

func_return	mmu_map

/*
 *	mmu_fixup
 *
 *	On the 040 class machines, all pages that are used for the
 *	mmu have to be fixed up.
 */

func_start	mmu_fixup_page_mmu_cache,%d0/%a0

	/* Calculate the offset into the root table
	 */
	movel	ARG1,%d0
	moveq	#ROOT_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	mmu_get_root_table_entry	%d0

	/* Calculate the offset into the pointer table
	 */
	movel	ARG1,%d0
	moveq	#PTR_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	andl	#PTR_TABLE_SIZE-1,%d0
	mmu_get_ptr_table_entry		%a0,%d0

	/* Calculate the offset into the page table
	 */
	movel	ARG1,%d0
	moveq	#PAGE_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	andl	#PAGE_TABLE_SIZE-1,%d0
	mmu_get_page_table_entry	%a0,%d0

	movel	%a0@,%d0
	andil	#_CACHEMASK040,%d0
	orl	%pc@(m68k_pgtable_cachemode),%d0
	movel	%d0,%a0@

func_return	mmu_fixup_page_mmu_cache

/*
 *	mmu_temp_map
 *
 *	create a temporary mapping to enable the mmu,
 *	this we don't need any transparation translation tricks.
 */

func_start	mmu_temp_map,%d0/%d1/%a0/%a1

	lea	%pc@(L(temp_mmap_mem)),%a1

	/* Calculate the offset in the root table
	 */
	movel	ARG2,%d0
	moveq	#ROOT_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	mmu_get_root_table_entry	%d0

	/* Check if the table is temporary allocated, so we have to reuse it
	 */
	movel	%a0@,%d0
	cmpl	%pc@(L(memory_start)),%d0
	jcc	1f

	/* Temporary allocate a ptr table and insert it into the root table
	 */
	movel	%a1@,%d0
	addl	#PTR_TABLE_SIZE*4,%a1@
	orw	#_PAGE_TABLE+_PAGE_ACCESSED,%d0
	movel	%d0,%a0@
1:
	/* Mask the root table entry for the ptr table
	 */
	andw	#-ROOT_TABLE_SIZE,%d0
	movel	%d0,%a0

	/* Calculate the offset into the pointer table
	 */
	movel	ARG2,%d0
	moveq	#PTR_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	andl	#PTR_TABLE_SIZE-1,%d0
	lea	%a0@(%d0*4),%a0

	/* Check if a temporary page table is already allocated
	 */
	movel	%a0@,%d0
	jne	1f

	/* Temporary allocate a page table and insert it into the ptr table
	 */
	movel	%a1@,%d0
	/* The 512 should be PAGE_TABLE_SIZE*4, but that violates the
	   alignment restriction for pointer tables on the '0[46]0.  */
	addl	#512,%a1@
	orw	#_PAGE_TABLE+_PAGE_ACCESSED,%d0
	movel	%d0,%a0@
1:
	/* Mask the ptr table entry for the page table
	 */
	andw	#-PTR_TABLE_SIZE,%d0
	movel	%d0,%a0

	/* Calculate the offset into the page table
	 */
	movel	ARG2,%d0
	moveq	#PAGE_INDEX_SHIFT,%d1
	lsrl	%d1,%d0
	andl	#PAGE_TABLE_SIZE-1,%d0
	lea	%a0@(%d0*4),%a0

	/* Insert the address into the page table
	 */
	movel	ARG1,%d0
	andw	#-PAGESIZE,%d0
	orw	#_PAGE_PRESENT+_PAGE_ACCESSED+_PAGE_DIRTY,%d0
	movel	%d0,%a0@

func_return	mmu_temp_map

func_start	mmu_engage,%d0-%d2/%a0-%a3

	moveq	#ROOT_TABLE_SIZE-1,%d0
	/* Temporarily use a different root table.  */
	lea	%pc@(L(kernel_pgdir_ptr)),%a0
	movel	%a0@,%a2
	movel	%pc@(L(memory_start)),%a1
	movel	%a1,%a0@
	movel	%a2,%a0
1:
	movel	%a0@+,%a1@+
	dbra	%d0,1b

	lea	%pc@(L(temp_mmap_mem)),%a0
	movel	%a1,%a0@

	movew	#PAGESIZE-1,%d0
1:
	clrl	%a1@+
	dbra	%d0,1b

	lea	%pc@(1b),%a0
	movel	#1b,%a1
	/* Skip temp mappings if phys == virt */
	cmpl	%a0,%a1
	jeq	1f

	mmu_temp_map	%a0,%a0
	mmu_temp_map	%a0,%a1

	addw	#PAGESIZE,%a0
	addw	#PAGESIZE,%a1
	mmu_temp_map	%a0,%a0
	mmu_temp_map	%a0,%a1
1:
	movel	%pc@(L(memory_start)),%a3
	movel	%pc@(L(phys_kernel_start)),%d2

	is_not_040_or_060(L(mmu_engage_030))

L(mmu_engage_040):
	.chip	68040
	nop
	cinva	%bc
	nop
	pflusha
	nop
	movec	%a3,%srp
	movel	#TC_ENABLE+TC_PAGE4K,%d0
	movec	%d0,%tc		/* enable the MMU */
	jmp	1f:l
1:	nop
	movec	%a2,%srp
	nop
	cinva	%bc
	nop
	pflusha
	.chip	68k
	jra	L(mmu_engage_cleanup)

L(mmu_engage_030_temp):
	.space	12
L(mmu_engage_030):
	.chip	68030
	lea	%pc@(L(mmu_engage_030_temp)),%a0
	movel	#0x80000002,%a0@
	movel	%a3,%a0@(4)
	movel	#0x0808,%d0
	movec	%d0,%cacr
	pmove	%a0@,%srp
	pflusha
	/*
	 * enable,super root enable,4096 byte pages,7 bit root index,
	 * 7 bit pointer index, 6 bit page table index.
	 */
	movel	#0x82c07760,%a0@(8)
	pmove	%a0@(8),%tc	/* enable the MMU */
	jmp	1f:l
1:	movel	%a2,%a0@(4)
	movel	#0x0808,%d0
	movec	%d0,%cacr
	pmove	%a0@,%srp
	pflusha
	.chip	68k

L(mmu_engage_cleanup):
	subl	#PAGE_OFFSET,%d2
	subl	%d2,%a2
	movel	%a2,L(kernel_pgdir_ptr)
	subl	%d2,%fp
	subl	%d2,%sp
	subl	%d2,ARG0

func_return	mmu_engage

func_start	mmu_get_root_table_entry,%d0/%a1

	movel	%pc@(L(kernel_pgdir_ptr)),%a0
	tstl	%a0
	jne	2f

	/* Find the start of free memory, get_bi_record does this for us,
	 * as the bootinfo structure is located directly behind the kernel
	 * and and we simply search for the last entry.
	 */
	get_bi_record	BI_LAST
	addw	#PAGESIZE-1,%a0
	movel	%a0,%d0
	andw	#-PAGESIZE,%d0

	lea	%pc@(L(memory_start)),%a0
	movel	%d0,%a0@
	lea	%pc@(L(kernel_end)),%a0
	movel	%d0,%a0@

	/* we have to return the first page at _stext since the init code
	 * in mm/init.c simply expects kernel_pg_dir there, the rest of
	 * page is used for further ptr tables in get_ptr_table.
	 */
	lea	%pc@(_stext),%a0
	lea	%pc@(L(mmu_cached_pointer_tables)),%a1
	movel	%a0,%a1@
	addl	#ROOT_TABLE_SIZE*4,%a1@

	lea	%pc@(L(mmu_num_pointer_tables)),%a1
	addql	#1,%a1@

	/* clear the page
	 */
	movel	%a0,%a1
	movew	#PAGESIZE/4-1,%d0
1:
	clrl	%a1@+
	dbra	%d0,1b

	lea	%pc@(L(kernel_pgdir_ptr)),%a1
	movel	%a0,%a1@

2:
	movel	ARG1,%d0
	lea	%a0@(%d0*4),%a0

func_return	mmu_get_root_table_entry



func_start	mmu_get_ptr_table_entry,%d0/%a1

	movel	ARG1,%a0
	movel	%a0@,%d0
	jne	2f

	/* Keep track of the number of pointer tables we use
	 */
	lea	%pc@(L(mmu_num_pointer_tables)),%a0
	movel	%a0@,%d0
	addql	#1,%a0@

	/* See if there is a free pointer table in our cache of pointer tables
	 */
	lea	%pc@(L(mmu_cached_pointer_tables)),%a1
	andw	#7,%d0
	jne	1f

	/* Get a new pointer table page from above the kernel memory
	 */
	get_new_page
	movel	%a0,%a1@
1:
	/* There is an unused pointer table in our cache... use it
	 */
	movel	%a1@,%d0
	addl	#PTR_TABLE_SIZE*4,%a1@

	/* Insert the new pointer table into the root table
	 */
	movel	ARG1,%a0
	orw	#_PAGE_TABLE+_PAGE_ACCESSED,%d0
	movel	%d0,%a0@
2:
	/* Extract the pointer table entry
	 */
	andw	#-PTR_TABLE_SIZE,%d0
	movel	%d0,%a0
	movel	ARG2,%d0
	lea	%a0@(%d0*4),%a0

func_return	mmu_get_ptr_table_entry


func_start	mmu_get_page_table_entry,%d0/%a1

	movel	ARG1,%a0
	movel	%a0@,%d0
	jne	2f

	/* If the page table entry doesn't exist, we allocate a complete new
	 * page and use it as one continues big page table which can cover
	 * 4MB of memory, nearly almost all mappings have that alignment.
	 */
	get_new_page
	addw	#_PAGE_TABLE+_PAGE_ACCESSED,%a0

	/* align pointer table entry for a page of page tables
	 */
	movel	ARG1,%d0
	andw	#-(PAGESIZE/PAGE_TABLE_SIZE),%d0
	movel	%d0,%a1

	/* Insert the page tables into the pointer entries
	 */
	moveq	#PAGESIZE/PAGE_TABLE_SIZE/4-1,%d0
1:
	movel	%a0,%a1@+
	lea	%a0@(PAGE_TABLE_SIZE*4),%a0
	dbra	%d0,1b

	/* Now we can get the initialized pointer table entry
	 */
	movel	ARG1,%a0
	movel	%a0@,%d0
2:
	/* Extract the page table entry
	 */
	andw	#-PAGE_TABLE_SIZE,%d0
	movel	%d0,%a0
	movel	ARG2,%d0
	lea	%a0@(%d0*4),%a0

func_return	mmu_get_page_table_entry

/*
 *	get_new_page
 *
 *	Return a new page from the memory start and clear it.
 */
func_start	get_new_page,%d0/%a1

	/* allocate the page and adjust memory_start
	 */
	lea	%pc@(L(memory_start)),%a0
	movel	%a0@,%a1
	addl	#PAGESIZE,%a0@

	/* clear the new page
	 */
	movel	%a1,%a0
	movew	#PAGESIZE/4-1,%d0
1:
	clrl	%a1@+
	dbra	%d0,1b

func_return	get_new_page

__INITDATA
	.align	4

m68k_init_mapped_size:
	.long	0

L(cputype):
	.long	0
L(mmu_cached_pointer_tables):
	.long	0
L(mmu_num_pointer_tables):
	.long	0
L(phys_kernel_start):
	.long	0
L(kernel_end):
	.long	0
L(memory_start):
	.long	0
L(kernel_pgdir_ptr):
	.long	0
L(temp_mmap_mem):
	.long	0

__FINIT
	.data
	.align	4

availmem:
	.long	0
m68k_pgtable_cachemode:
	.long	0
m68k_supervisor_cachemode:
	.long	0
