/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _M68K_IRQ_H_
#define _M68K_IRQ_H_

#define NR_IRQS 256

/*
 * Interrupt source definitions
 * General interrupt sources are the level 1-7.
 * Adding an interrupt service routine for one of these sources
 * results in the addition of that routine to a chain of routines.
 * Each one is called in succession.  Each individual interrupt
 * service routine should determine if the device associated with
 * that routine requires service.
 */

#define IRQ_SPURIOUS	0

#define IRQ_AUTO_1	1	/* level 1 interrupt */
#define IRQ_AUTO_2	2	/* level 2 interrupt */
#define IRQ_AUTO_3	3	/* level 3 interrupt */
#define IRQ_AUTO_4	4	/* level 4 interrupt */
#define IRQ_AUTO_5	5	/* level 5 interrupt */
#define IRQ_AUTO_6	6	/* level 6 interrupt */
#define IRQ_AUTO_7	7	/* level 7 interrupt (non-maskable) */

#define IRQ_USER	8

struct irq_data;
struct irq_chip;
struct irq_desc;
extern unsigned int m68k_irq_startup(struct irq_data *data);
extern unsigned int m68k_irq_startup_irq(unsigned int irq);
extern void m68k_irq_shutdown(struct irq_data *data);
extern void m68k_setup_auto_interrupt(void (*handler)(unsigned int,
						      struct pt_regs *));
extern void m68k_setup_user_interrupt(unsigned int vec, unsigned int cnt);
extern void m68k_setup_irq_controller(struct irq_chip *,
				      void (*handle)(struct irq_desc *desc),
				      unsigned int irq, unsigned int cnt);

asmlinkage void do_IRQ(int irq, struct pt_regs *regs);
extern atomic_t irq_err_count;

#include <asm-generic/irq.h>

#endif /* _M68K_IRQ_H_ */
