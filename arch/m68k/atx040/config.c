#include <linux/console.h>
#include <asm/bootinfo.h>
#include <asm/machdep.h>
#include <asm/atx040hw.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

extern void __init atx040_init_IRQ(void);

static struct resource wb_uart_res[] = {
	{
		.start = ATX040_WB_UART_BASE,
		.end = ATX040_WB_UART_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = ATX040_WB_UART_IRQ,
		.end = ATX040_WB_UART_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device wb_uart_device = {
	.name = "wb_uart",
	.id = 0,
	.num_resources = ARRAY_SIZE(wb_uart_res),
	.resource = wb_uart_res,
};

static struct resource spi0_res[] = {
	{
		.start = ATX040_SPI0_BASE,
		.end = ATX040_SPI0_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device spi0_device = {
	.name = "wb_spi",
	.id = 0,
	.num_resources = ARRAY_SIZE(spi0_res),
	.resource = spi0_res,
};

static struct resource spi1_res[] = {
	{
		.start = ATX040_SPI1_BASE,
		.end = ATX040_SPI1_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device spi1_device = {
	.name = "wb_spi",
	.id = 1,
	.num_resources = ARRAY_SIZE(spi1_res),
	.resource = spi1_res,
};

static struct resource spi2_res[] = {
	{
		.start = ATX040_SPI2_BASE,
		.end = ATX040_SPI2_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device spi2_device = {
	.name = "wb_spi",
	.id = 2,
	.num_resources = ARRAY_SIZE(spi2_res),
	.resource = spi2_res,
};

static struct spi_board_info spi_info[] = {
	{
		.modalias = "enc28j60",
		.mode = SPI_MODE_0,
		.irq = ATX040_SPI_ETH_IRQ,
		.max_speed_hz = 12000000,
		.bus_num = 2,
		.chip_select = 0,
	},
	{
		.modalias = "mmc_spi",
		.mode = SPI_MODE_0,
		.max_speed_hz = 12000000,
		.bus_num = 1,
		.chip_select = 0,
	},
};

static struct resource i2s_res[] = {
	{
		.start = ATX040_I2S_BASE,
		.end = ATX040_I2S_BASE + 16,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = ATX040_I2S_SND_IRQ,
		.end = ATX040_I2S_SND_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device i2s_device = {
	.name = "atx040,i2s",
	.id = 0,
	.num_resources = ARRAY_SIZE(i2s_res),
	.resource = i2s_res,
};

static void atx040_get_model(char *model)
{
	sprintf(model, "ATX040");
}

int __init atx040_parse_bootinfo(const struct bi_record *rec)
{
	return 1;
}

static irq_handler_t atx040_timer_routine;

static irqreturn_t atx040_timer_irq(int irq, void * dev)
{
	return atx040_timer_routine(irq, dev);
}

void atx040_sched_init(irq_handler_t timer_routine)
{
	atx040_timer_routine = timer_routine;
	if(request_irq(ATX040_SYSTICK_IRQ, atx040_timer_irq, 0, "timer", NULL))
		panic("could not register timer irq");
}

static void atx040_reset(void)
{
	for(;;)
		asm volatile("reset");
}

void __init config_atx040(void)
{
	mach_init_IRQ = atx040_init_IRQ;
	mach_sched_init = atx040_sched_init;
	mach_get_model = atx040_get_model;
	mach_max_dma_address = 0xffffffff;
	mach_reset = atx040_reset;
}

int __init atx040_platform_init(void)
{
	platform_device_register(&wb_uart_device);
	platform_device_register(&spi0_device);
	platform_device_register(&spi1_device);
	platform_device_register(&spi2_device);
	platform_device_register(&i2s_device);

	spi_register_board_info(spi_info, ARRAY_SIZE(spi_info));
	return 0;
}

arch_initcall(atx040_platform_init);
