/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __M68K_HARDIRQ_H
#define __M68K_HARDIRQ_H

#include <linux/threads.h>
#include <linux/cache.h>
#include <asm/irq.h>

static inline void ack_bad_irq(unsigned int irq)
{
	pr_crit("unexpected IRQ trap at vector %02x\n", irq);
}

/* entry.S is sensitive to the offsets of these fields */
typedef struct {
	unsigned int __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;

#include <linux/irq_cpustat.h>	/* Standard mappings for irq_cpustat_t above */

#endif
