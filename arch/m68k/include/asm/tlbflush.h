/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _M68K_TLBFLUSH_H
#define _M68K_TLBFLUSH_H

#ifdef CONFIG_MMU

#include <asm/current.h>

static inline void flush_tlb_kernel_page(void *addr)
{
	if (CPU_IS_040_OR_060) {
		mm_segment_t old_fs = get_fs();
		set_fs(KERNEL_DS);
		__asm__ __volatile__(".chip 68040\n\t"
				     "pflush (%0)\n\t"
				     ".chip 68k"
				     : : "a" (addr));
		set_fs(old_fs);
	} else if (CPU_IS_020_OR_030)
		__asm__ __volatile__("pflush #4,#4,(%0)" : : "a" (addr));
}

/*
 * flush all user-space atc entries.
 */
static inline void __flush_tlb(void)
{
	if (CPU_IS_040_OR_060) {
		__asm__ __volatile__(".chip 68040\n\t"
				     "pflushan\n\t"
				     ".chip 68k");
	} else if (CPU_IS_020_OR_030) {
		__asm__ __volatile__("pflush #0,#4");
	}
}

static inline void __flush_tlb040_one(unsigned long addr)
{
	__asm__ __volatile__(".chip 68040\n\t"
			     "pflush (%0)\n\t"
			     ".chip 68k"
			     : : "a" (addr));
}

static inline void __flush_tlb_one(unsigned long addr)
{
	if (CPU_IS_040_OR_060)
		__flush_tlb040_one(addr);
	else if (CPU_IS_020_OR_030)
		__asm__ __volatile__("pflush #0,#4,(%0)" : : "a" (addr));
}

#define flush_tlb() __flush_tlb()

/*
 * flush all atc entries (both kernel and user-space entries).
 */
static inline void flush_tlb_all(void)
{
	if (CPU_IS_040_OR_060) {
		__asm__ __volatile__(".chip 68040\n\t"
				     "pflusha\n\t"
				     ".chip 68k");
	} else if (CPU_IS_020_OR_030) {
		__asm__ __volatile__("pflusha");
	}
}

static inline void flush_tlb_mm(struct mm_struct *mm)
{
	if (mm == current->active_mm)
		__flush_tlb();
}

static inline void flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
{
	if (vma->vm_mm == current->active_mm) {
		mm_segment_t old_fs = get_fs();
		set_fs(USER_DS);
		__flush_tlb_one(addr);
		set_fs(old_fs);
	}
}

static inline void flush_tlb_range(struct vm_area_struct *vma,
				   unsigned long start, unsigned long end)
{
	if (vma->vm_mm == current->active_mm)
		__flush_tlb();
}

static inline void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	flush_tlb_all();
}

#else /* !CONFIG_MMU */

/*
 * flush all user-space atc entries.
 */
static inline void __flush_tlb(void)
{
	BUG();
}

static inline void __flush_tlb_one(unsigned long addr)
{
	BUG();
}

#define flush_tlb() __flush_tlb()

/*
 * flush all atc entries (both kernel and user-space entries).
 */
static inline void flush_tlb_all(void)
{
	BUG();
}

static inline void flush_tlb_mm(struct mm_struct *mm)
{
	BUG();
}

static inline void flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
{
	BUG();
}

static inline void flush_tlb_range(struct mm_struct *mm,
				   unsigned long start, unsigned long end)
{
	BUG();
}

static inline void flush_tlb_kernel_page(unsigned long addr)
{
	BUG();
}

#endif /* CONFIG_MMU */

#endif /* _M68K_TLBFLUSH_H */
