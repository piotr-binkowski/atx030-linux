#include <linux/init.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

static int c_show(struct seq_file *m, void *v)
{
	const char *cpu, *mmu, *fpu;
	unsigned long clockfreq, clockfactor;

#define LOOP_CYCLES_68030	(8)
#define LOOP_CYCLES_68040	(3)
#define LOOP_CYCLES_68060	(1)

	if (CPU_IS_030) {
		cpu = "68030";
		clockfactor = LOOP_CYCLES_68030;
	} else if (CPU_IS_040) {
		cpu = "68040";
		clockfactor = LOOP_CYCLES_68040;
	} else if (CPU_IS_060) {
		cpu = "68060";
		clockfactor = LOOP_CYCLES_68060;
	} else {
		cpu = "680x0";
		clockfactor = 0;
	}

	if (m68k_fputype & FPU_68881)
		fpu = "68881";
	else if (m68k_fputype & FPU_68882)
		fpu = "68882";
	else if (m68k_fputype & FPU_68040)
		fpu = "68040";
	else if (m68k_fputype & FPU_68060)
		fpu = "68060";
	else
		fpu = "none";

	if (m68k_mmutype & MMU_68030)
		mmu = "68030";
	else if (m68k_mmutype & MMU_68040)
		mmu = "68040";
	else if (m68k_mmutype & MMU_68060)
		mmu = "68060";
	else
		mmu = "unknown";

	clockfreq = loops_per_jiffy * HZ * clockfactor;

	seq_printf(m, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "Clocking:\t%lu.%1luMHz\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   clockfreq/1000000,(clockfreq/100000)%10,
		   loops_per_jiffy/(500000/HZ),(loops_per_jiffy/(5000/HZ))%100,
		   loops_per_jiffy);
	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
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
