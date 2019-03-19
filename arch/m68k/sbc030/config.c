#include <linux/console.h>
#include <asm/bootinfo.h>
#include <asm/machdep.h>
#include <asm/sbc030hw.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

extern void __init sbc030_init_IRQ(void);

static struct resource duart_res[] = {
	{
		.start = SBC030_DUART_BASE,
		.end = SBC030_DUART_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_AUTO_1,
		.end = IRQ_AUTO_1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device duart_device = {
	.name = "mc,68681",
	.id = 0,
	.num_resources = ARRAY_SIZE(duart_res),
	.resource = duart_res,
};

static struct resource spi0_res[] = {
	{
		.start = SBC030_SPI0_BASE,
		.end = SBC030_SPI0_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_AUTO_3,
		.end = IRQ_AUTO_3,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device spi0_device = {
	.name = "sbc030,spi",
	.id = 0,
	.num_resources = ARRAY_SIZE(spi0_res),
	.resource = spi0_res,
};

static struct resource spi1_res[] = {
	{
		.start = SBC030_SPI1_BASE,
		.end = SBC030_SPI1_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_AUTO_3,
		.end = IRQ_AUTO_3,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device spi1_device = {
	.name = "sbc030,spi",
	.id = 1,
	.num_resources = ARRAY_SIZE(spi1_res),
	.resource = spi1_res,
};

#ifdef CONFIG_EARLY_PRINTK

static void sbc030_serial_putc(const char c)
{
	while(!(ioread8((void*)SBC030_DUART_SR) & 0x04));
	iowrite8(c, (void*)SBC030_DUART_TB);
}

static void sbc030_console_write(struct console *co, const char *s, unsigned int count)
{
	while(count--){
		if(*s == '\n')
			sbc030_serial_putc('\r');
		sbc030_serial_putc(*s++);
	}
}

static struct console sbc030_console_driver = {
	.name = "sbc030serial",
	.flags = CON_PRINTBUFFER | CON_BOOT,
	.index = -1,
	.write = sbc030_console_write,
};

#endif

static void sbc030_get_model(char *model)
{
	sprintf(model, "SBC030");
}

int __init sbc030_parse_bootinfo(const struct bi_record *rec)
{
	return 1;
}

static irq_handler_t sbc030_timer_routine;

static irqreturn_t sbc030_timer_irq(int irq, void * dev)
{
	iowrite8(0x01, (void*)SBC030_PIT_TSR);
	return sbc030_timer_routine(irq, dev);
}

void sbc030_sched_init(irq_handler_t timer_routine)
{
	sbc030_timer_routine = timer_routine;
	iowrite8(0x00, (void*)SBC030_PIT_TCR);
	iowrite8(0x00, (void*)SBC030_PIT_CPRH);
	iowrite8(0x04, (void*)SBC030_PIT_CPRM);
	iowrite8(0x80, (void*)SBC030_PIT_CPRL);
	iowrite8(0xe1, (void*)SBC030_PIT_TCR);
	if(request_irq(IRQ_AUTO_2, sbc030_timer_irq, 0, "timer", NULL))
		panic("could not register timer irq");
}

void __init config_sbc030(void)
{
#ifdef CONFIG_EARLY_PRINTK
	register_console(&sbc030_console_driver);
#endif
	mach_init_IRQ = sbc030_init_IRQ;
	mach_sched_init = sbc030_sched_init;
	mach_get_model = sbc030_get_model;
	mach_max_dma_address = 0x10000000;
}

int __init sbc030_platform_init(void)
{
	platform_device_register(&duart_device);

	platform_device_register(&spi0_device);

	platform_device_register(&spi1_device);

	return 0;
}

arch_initcall(sbc030_platform_init);
