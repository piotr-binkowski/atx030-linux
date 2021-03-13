#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/traps.h>
#include <asm/atx040hw.h>

void __init atx040_init_IRQ(void)
{
	void __iomem *irqc_base;

	m68k_setup_user_interrupt(VEC_USER, 32);

	irqc_base = ioremap(ATX040_IRQC_BASE, 0x100);
	
	writel(0xFFFFFFFF, irqc_base);
}
