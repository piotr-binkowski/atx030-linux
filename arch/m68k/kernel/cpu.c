#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/of.h>

#define LOOP_CYCLES_68030	(8)
#define LOOP_CYCLES_68040	(3)
#define LOOP_CYCLES_68060	(1)

static int c_show(struct seq_file *m, void *v)
{
	const char *cpu, *mmu, *fpu;
	unsigned long clockfreq, clockfactor;
	struct device_node *cpu_node = of_get_cpu_node(0, NULL);
	struct device_node *mmu_node = of_get_child_by_name(cpu_node, "mmu");
	struct device_node *fpu_node = of_get_child_by_name(cpu_node, "fpu");

	of_property_read_string(cpu_node, "compatible", &cpu);
	of_property_read_string(mmu_node, "compatible", &mmu);
	of_property_read_string(fpu_node, "compatible", &fpu);

	if(!strcmp(cpu, "motorola,68030"))
		clockfactor = LOOP_CYCLES_68030;
	else if(!strcmp(cpu, "motorola,68040"))
		clockfactor = LOOP_CYCLES_68040;
	else if(!strcmp(cpu, "motorola,68060"))
		clockfactor = LOOP_CYCLES_68060;
	else
		clockfactor = 0;

	clockfreq = loops_per_jiffy * HZ * clockfactor;
	clockfreq /= 100000;

	seq_printf(m, "cpu:\t\t%s@%lu.%1luMHz\n"
		   "mmu:\t\t%s\n"
		   "fpu:\t\t%s\n",
		   cpu, clockfreq/10, clockfreq%10,
		   mmu, fpu);

	of_node_put(cpu_node);
	of_node_put(mmu_node);
	of_node_put(fpu_node);

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show,
};
