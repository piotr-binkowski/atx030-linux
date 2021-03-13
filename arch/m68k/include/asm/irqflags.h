/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _M68K_IRQFLAGS_H
#define _M68K_IRQFLAGS_H

#include <linux/types.h>
#include <linux/preempt.h>
#include <asm/thread_info.h>
#include <asm/entry.h>

static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;
	asm volatile ("movew %%sr,%0" : "=d" (flags) : : "memory");
	return flags;
}

static inline void arch_local_irq_disable(void)
{
	asm volatile ("oriw  #0x0700,%%sr" : : : "memory");
}

static inline void arch_local_irq_enable(void)
{
	if (!hardirq_count())
		asm volatile (
			"andiw %0,%%sr"
			:
			: "i" (ALLOWINT)
			: "memory");
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags = arch_local_save_flags();
	arch_local_irq_disable();
	return flags;
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	asm volatile ("movew %0,%%sr" : : "d" (flags) : "memory");
}

static inline bool arch_irqs_disabled_flags(unsigned long flags)
{
	return (flags & ~ALLOWINT) != 0;
}

static inline bool arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

#endif /* _M68K_IRQFLAGS_H */
