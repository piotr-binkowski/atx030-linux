// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/arch/m68k/kernel/setup.c
 *
 *  Copyright (C) 1995  Hamish Macdonald
 */

/*
 * This file handles the architecture-dependent parts of system setup
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/console.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/initrd.h>

#include <linux/of_fdt.h>

#include <asm/bootinfo.h>
#include <asm/byteorder.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/fpu.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/machdep.h>

#if !FPSTATESIZE || !NR_IRQS
#warning No CPU/platform type selected, your kernel will not work!
#warning Are you building an allnoconfig kernel?
#endif

unsigned long m68k_machtype;
EXPORT_SYMBOL(m68k_machtype);
unsigned long m68k_cputype;
EXPORT_SYMBOL(m68k_cputype);
unsigned long m68k_fputype;
unsigned long m68k_mmutype;
EXPORT_SYMBOL(m68k_mmutype);

int m68k_is040or060;
EXPORT_SYMBOL(m68k_is040or060);

extern unsigned long availmem;

int m68k_num_memory;
EXPORT_SYMBOL(m68k_num_memory);
int m68k_realnum_memory;
EXPORT_SYMBOL(m68k_realnum_memory);
unsigned long m68k_memoffset;
struct m68k_mem_info m68k_memory[NUM_MEMINFO];
EXPORT_SYMBOL(m68k_memory);

static struct m68k_mem_info m68k_ramdisk __initdata;

static struct m68k_mem_info m68k_device_tree __initdata;

static char m68k_command_line[CL_SIZE] __initdata;

void (*mach_sched_init) (irq_handler_t handler) __initdata = NULL;
/* machine dependent irq functions */
void (*mach_init_IRQ) (void) __initdata = NULL;
void (*mach_get_model) (char *model);
void (*mach_get_hardware_list) (struct seq_file *m);
/* machine dependent timer functions */
int (*mach_hwclk) (int, struct rtc_time*);
EXPORT_SYMBOL(mach_hwclk);
unsigned int (*mach_get_ss)(void);
int (*mach_get_rtc_pll)(struct rtc_pll_info *);
int (*mach_set_rtc_pll)(struct rtc_pll_info *);
EXPORT_SYMBOL(mach_get_ss);
EXPORT_SYMBOL(mach_get_rtc_pll);
EXPORT_SYMBOL(mach_set_rtc_pll);
void (*mach_reset)( void );
void (*mach_halt)( void );
void (*mach_power_off)( void );
long mach_max_dma_address = 0x00ffffff; /* default set to the lower 16MB */

extern int atx040_parse_bootinfo(const struct bi_record *);

extern void config_atx040(void);

#define MASK_256K 0xfffc0000

extern void paging_init(void);

unsigned long fdt_start = 0;
unsigned long fdt_end = 0;

static void __init m68k_parse_bootinfo(const struct bi_record *record)
{
	uint16_t tag;

	while ((tag = be16_to_cpu(record->tag)) != BI_LAST) {
		int unknown = 0;
		const void *data = record->data;
		uint16_t size = be16_to_cpu(record->size);

		switch (tag) {
		case BI_MACHTYPE:
		case BI_CPUTYPE:
		case BI_FPUTYPE:
		case BI_MMUTYPE:
			/* Already set up by head.S */
			break;

		case BI_MEMCHUNK:
			if (m68k_num_memory < NUM_MEMINFO) {
				const struct mem_info *m = data;
				m68k_memory[m68k_num_memory].addr =
					be32_to_cpu(m->addr);
				m68k_memory[m68k_num_memory].size =
					be32_to_cpu(m->size);
				m68k_num_memory++;
			} else
				pr_warn("%s: too many memory chunks\n",
					__func__);
			break;

		case BI_RAMDISK:
			{
				const struct mem_info *m = data;
				m68k_ramdisk.addr = be32_to_cpu(m->addr);
				m68k_ramdisk.size = be32_to_cpu(m->size);
			}
			break;

		case BI_COMMAND_LINE:
			strlcpy(m68k_command_line, data,
				sizeof(m68k_command_line));
			break;

		case BI_DEVICE_TREE:
			{
				const struct mem_info *m = data;
				m68k_device_tree.addr = be32_to_cpu(m->addr);
				m68k_device_tree.size = be32_to_cpu(m->size);
			}
			break;

		default:
			if (MACH_IS_ATX040)
				unknown = atx040_parse_bootinfo(record);
			else
				unknown = 1;
		}
		if (unknown)
			pr_warn("%s: unknown tag 0x%04x ignored\n", __func__,
				tag);
		record = (struct bi_record *)((unsigned long)record + size);
	}

	m68k_realnum_memory = m68k_num_memory;
#ifdef CONFIG_SINGLE_MEMORY_CHUNK
	if (m68k_num_memory > 1) {
		pr_warn("%s: ignoring last %i chunks of physical memory\n",
			__func__, (m68k_num_memory - 1));
		m68k_num_memory = 1;
	}
#endif
}

void __init setup_arch(char **cmdline_p)
{
	/* The bootinfo is located right after the kernel */
	m68k_parse_bootinfo((const struct bi_record *)_end);

	if (CPU_IS_040)
		m68k_is040or060 = 4;
	else if (CPU_IS_060)
		m68k_is040or060 = 6;

	/* clear the fpu if we have one */
	if (m68k_fputype & (FPU_68881|FPU_68882|FPU_68040|FPU_68060)) {
		volatile int zero = 0;
		asm volatile ("frestore %0" : : "m" (zero));
	}

	if (CPU_IS_060) {
		u32 pcr;

		asm (".chip 68060; movec %%pcr,%0; .chip 68k"
		     : "=d" (pcr));
		if (((pcr >> 8) & 0xff) <= 5) {
			pr_warn("Enabling workaround for errata I14\n");
			asm (".chip 68060; movec %0,%%pcr; .chip 68k"
			     : : "d" (pcr | 0x20));
		}
	}

	init_mm.start_code = PAGE_OFFSET;
	init_mm.end_code = (unsigned long)_etext;
	init_mm.end_data = (unsigned long)_edata;
	init_mm.brk = (unsigned long)_end;

#if defined(CONFIG_BOOTPARAM)
	strncpy(m68k_command_line, CONFIG_BOOTPARAM_STRING, CL_SIZE);
	m68k_command_line[CL_SIZE - 1] = 0;
#endif /* CONFIG_BOOTPARAM */
	*cmdline_p = m68k_command_line;
	memcpy(boot_command_line, *cmdline_p, CL_SIZE);

	parse_early_param();

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

	switch (m68k_machtype) {
#ifdef CONFIG_ATX040
	case MACH_ATX040:
		config_atx040();
		break;
#endif
	default:
		panic("No configuration setup");
	}

	paging_init();

#ifdef CONFIG_BLK_DEV_INITRD
	if (m68k_ramdisk.size) {
		memblock_reserve(m68k_ramdisk.addr, m68k_ramdisk.size);
		initrd_start = (unsigned long)phys_to_virt(m68k_ramdisk.addr);
		initrd_end = initrd_start + m68k_ramdisk.size;
		pr_info("initrd: %08lx - %08lx\n", initrd_start, initrd_end);
	}
#endif
	if (m68k_device_tree.size) {
		memblock_reserve(m68k_device_tree.addr, m68k_device_tree.size);
		fdt_start = (unsigned long)phys_to_virt(m68k_device_tree.addr);
		fdt_end = fdt_start + m68k_device_tree.size;
		pr_info("fdt: %08lx - %08lx\n", fdt_start, fdt_end);
		if(!early_init_dt_scan(__va(fdt_start)))
			pr_warn("early_init_dt_scan failed\n");
	}

	unflatten_device_tree();
}
