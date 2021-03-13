/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _M68K_PGTABLE_H
#define _M68K_PGTABLE_H

#include <asm-generic/4level-fixup.h>

#include <asm/setup.h>

#ifndef __ASSEMBLY__
#include <asm/processor.h>
#include <linux/sched.h>
#include <linux/threads.h>

/*
 * This file contains the functions and defines necessary to modify and use
 * the m68k page table tree.
 */

#include <asm/virtconvert.h>

/* Certain architectures need to do special things when pte's
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
#define set_pte(pteptr, pteval)					\
	do{							\
		*(pteptr) = (pteval);				\
	} while(0)
#define set_pte_at(mm,addr,ptep,pteval) set_pte(ptep,pteval)


/* PMD_SHIFT determines the size of the area a second-level page table can map */
#define PMD_SHIFT	22
#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE-1))

/* PGDIR_SHIFT determines what a third-level page table entry can map */
#define PGDIR_SHIFT	25
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

/*
 * entries per page directory level: the m68k is configured as three-level,
 * so we do have PMD level physically.
 */
#define PTRS_PER_PTE	1024
#define PTRS_PER_PMD	8
#define PTRS_PER_PGD	128
#define USER_PTRS_PER_PGD	(TASK_SIZE/PGDIR_SIZE)
#define FIRST_USER_ADDRESS	0UL

/* Virtual address region for use by kernel_map() */
#define	KMAP_START	0xd0000000
#define	KMAP_END	0xf0000000

/* Just any arbitrary offset to the start of the vmalloc VM area: the
 * current 8MB value just means that there will be a 8MB "hole" after the
 * physical memory until the kernel virtual memory starts.  That means that
 * any out-of-bounds memory accesses will hopefully be caught.
 * The vmalloc() routines leaves a hole of 4kB between each vmalloced
 * area for the same reason. ;)
 */
#define VMALLOC_OFFSET	(8*1024*1024)
#define VMALLOC_START (((unsigned long) high_memory + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1))
#define VMALLOC_END KMAP_START

/* zero page used for uninitialized stuff */
extern void *empty_zero_page;

/*
 * ZERO_PAGE is a global shared page that is always zero: used
 * for zero-mapped memory areas etc..
 */
#define ZERO_PAGE(vaddr)	(virt_to_page(empty_zero_page))

/* number of bits that fit into a memory pointer */
#define BITS_PER_PTR			(8*sizeof(unsigned long))

/* to align the pointer to a pointer address */
#define PTR_MASK			(~(sizeof(void*)-1))

/* sizeof(void*)==1<<SIZEOF_PTR_LOG2 */
/* 64-bit machines, beware!  SRB. */
#define SIZEOF_PTR_LOG2			       2

extern void kernel_set_cachemode(void *addr, unsigned long size, int cmode);

/*
 * The m68k doesn't have any external MMU info: the kernel page
 * tables contain all the necessary information.  The Sun3 does, but
 * they are updated on demand.
 */
static inline void update_mmu_cache(struct vm_area_struct *vma,
				    unsigned long address, pte_t *ptep)
{
}

#endif /* !__ASSEMBLY__ */

#define kern_addr_valid(addr)	(1)

/* MMU-specific headers */

#include <asm/motorola_pgtable.h>

#ifndef __ASSEMBLY__
/*
 * Macro to mark a page protection value as "uncacheable".
 */
#define pgprot_noncached(prot)							\
	 ((MMU_IS_851 || MMU_IS_030)						\
	    ? (__pgprot(pgprot_val(prot) | _PAGE_NOCACHE030))			\
	    : (MMU_IS_040 || MMU_IS_060)					\
	    ? (__pgprot((pgprot_val(prot) & _CACHEMASK040) | _PAGE_NOCACHE_S))	\
	    : (prot))

#include <asm-generic/pgtable.h>
#endif /* !__ASSEMBLY__ */

/*
 * No page table caches to initialise
 */
#define pgtable_cache_init()	do { } while (0)

#define check_pgt_cache()	do { } while (0)

#endif /* _M68K_PGTABLE_H */
