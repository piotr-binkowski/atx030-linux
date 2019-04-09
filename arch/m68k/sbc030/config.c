#include <linux/console.h>
#include <asm/bootinfo.h>
#include <asm/machdep.h>
#include <asm/sbc030hw.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>

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
	.name = "mc68681",
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

static struct resource sst_flash_res = {
	.start = SBC030_FLASH_BASE,
	.end = SBC030_FLASH_BASE + SBC030_FLASH_SIZE - 1,
	.flags = IORESOURCE_MEM,
};

static struct mtd_partition sst_flash_part = {
	.name = "u-boot",
	.size = 0x80000,
	.offset = 0x0,
};

static struct physmap_flash_data sst_flash_data = {
	.width = 1,
	.parts = &sst_flash_part,
	.nr_parts = 1,
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

static struct mtd_partition spi_flash_parts[] = {
		{
			.name = "kernel",
			.size = 0x200000,
			.offset = 0x0,
		},
		{
			.name = "rootfs",
			.size = 0xe00000,
			.offset = 0x200000,
		},
	};

static struct flash_platform_data spi_flash_data = {
		.name = "w25q128",
		.parts = spi_flash_parts,
		.nr_parts = ARRAY_SIZE(spi_flash_parts),
		.type = "w25q128",
	};

static struct spi_board_info sbc030_spi_info[] = {
	{
		.modalias = "enc28j60",
		.mode = SPI_MODE_0,
		.irq = IRQ_AUTO_3,
		.max_speed_hz = 20000000,
		.bus_num = 1,
		.chip_select = 0,
	},
	{
		.modalias = "mmc_spi",
		.mode = SPI_MODE_0,
		.max_speed_hz = 20000000,
		.bus_num = 0,
		.chip_select = 0,
	},
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

static void sbc030_reset(void)
{
	asm volatile("reset");
}

void __init config_sbc030(void)
{
#ifdef CONFIG_EARLY_PRINTK
	register_console(&sbc030_console_driver);
#endif
	mach_init_IRQ = sbc030_init_IRQ;
	mach_sched_init = sbc030_sched_init;
	mach_get_model = sbc030_get_model;
	mach_max_dma_address = 0xffffffff;
	mach_reset = sbc030_reset;

#ifdef CONFIG_VT
#if defined(CONFIG_SBC_CONSOLE)
	conswitchp = &sbc_con;
#endif
#endif
}

int __init sbc030_platform_init(void)
{
	platform_device_register(&duart_device);

	platform_device_register(&spi0_device);

	platform_device_register(&spi1_device);

	platform_device_register(&sst_flash_device);

	spi_register_board_info(sbc030_spi_info, ARRAY_SIZE(sbc030_spi_info));

	return 0;
}

arch_initcall(sbc030_platform_init);
