#include <linux/console.h>
#include <asm/bootinfo.h>
#include <asm/machdep.h>
#include <asm/atx030hw.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>

extern void __init atx030_init_IRQ(void);

static struct resource ft245_res[] = {
	{
		.start = ATX030_FT245_BASE,
		.end = ATX030_FT245_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_AUTO_2,
		.end = IRQ_AUTO_2,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device ft245_device = {
	.name = "ft245",
	.id = 0,
	.num_resources = ARRAY_SIZE(ft245_res),
	.resource = ft245_res,
};

static struct resource sst_flash_res = {
	.start = ATX030_FLASH_BASE,
	.end = ATX030_FLASH_BASE + ATX030_FLASH_SIZE - 1,
	.flags = IORESOURCE_MEM,
};

static struct mtd_partition sst_flash_parts[] = {
	{
		.name = "u-boot",
		.size = 0x20000,
		.offset = 0x0,
	},
	{
		.name = "jffs2",
		.size = 0x60000,
		.offset = 0x20000,
	},
};

static struct physmap_flash_data sst_flash_data = {
	.width = 1,
	.parts = sst_flash_parts,
	.nr_parts = 2,
};

static struct platform_device sst_flash_device = {
	.name = "physmap-flash",
	.id = 0,
	.dev = {
		.platform_data = &sst_flash_data,
	},
	.num_resources = 1,
	.resource = &sst_flash_res,
};

#ifdef CONFIG_EARLY_PRINTK

static void atx030_serial_putc(const char c)
{
	while(ioread8((void*)ATX030_FT245_TXE));
	iowrite8(c, (void*)ATX030_FT245_DATA);
}

static void atx030_console_write(struct console *co, const char *s, unsigned int count)
{
	while(count--){
		if(*s == '\n')
			atx030_serial_putc('\r');
		atx030_serial_putc(*s++);
	}
}

static struct console atx030_console_driver = {
	.name = "atx030serial",
	.flags = CON_PRINTBUFFER | CON_BOOT,
	.index = -1,
	.write = atx030_console_write,
};

#endif

static void atx030_get_model(char *model)
{
	sprintf(model, "ATX030");
}

int __init atx030_parse_bootinfo(const struct bi_record *rec)
{
	return 1;
}

static irq_handler_t atx030_timer_routine;

static irqreturn_t atx030_timer_irq(int irq, void * dev)
{
	return atx030_timer_routine(irq, dev);
}

void atx030_sched_init(irq_handler_t timer_routine)
{
	atx030_timer_routine = timer_routine;
	if(request_irq(IRQ_AUTO_1, atx030_timer_irq, 0, "timer", NULL))
		panic("could not register timer irq");
}

static void atx030_reset(void)
{
	for(;;)
		asm volatile("reset");
}

void __init config_atx030(void)
{
#ifdef CONFIG_EARLY_PRINTK
	register_console(&atx030_console_driver);
#endif
	mach_init_IRQ = atx030_init_IRQ;
	mach_sched_init = atx030_sched_init;
	mach_get_model = atx030_get_model;
	mach_max_dma_address = 0xffffffff;
	mach_reset = atx030_reset;
}

int __init atx030_platform_init(void)
{
	platform_device_register(&ft245_device);

	platform_device_register(&sst_flash_device);

	return 0;
}

arch_initcall(atx030_platform_init);
