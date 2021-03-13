/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/asm-m68k/processor.h
 *
 * Copyright (C) 1995 Hamish Macdonald
 */

#ifndef __ASM_M68K_PROCESSOR_H
#define __ASM_M68K_PROCESSOR_H

#include <linux/thread_info.h>
#include <asm/segment.h>
#include <asm/fpu.h>
#include <asm/ptrace.h>

static inline unsigned long rdusp(void)
{
	register unsigned long usp __asm__("a0");
	/* move %usp,%a0 */
	__asm__ __volatile__(".word 0x4e68" : "=a" (usp));
	return usp;
}

static inline void wrusp(unsigned long usp)
{
	register unsigned long a0 __asm__("a0") = usp;
	/* move %a0,%usp */
	__asm__ __volatile__(".word 0x4e60" : : "a" (a0) );
}

/*
 * User space process size: 3.75GB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.
 */
#define TASK_SIZE	(0xF0000000UL)

#ifdef __KERNEL__
#define STACK_TOP	TASK_SIZE
#define STACK_TOP_MAX	STACK_TOP
#endif

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	0xC0000000UL
#define TASK_UNMAPPED_ALIGN(addr, off)	PAGE_ALIGN(addr)

struct thread_struct {
	unsigned long  ksp;		/* kernel stack pointer */
	unsigned long  usp;		/* user stack pointer */
	unsigned short sr;		/* saved status register */
	unsigned short fs;		/* saved fs (sfc, dfc) */
	unsigned long  crp[2];		/* cpu root pointer */
	unsigned long  esp0;		/* points to SR of stack frame */
	unsigned long  faddr;		/* info about last fault */
	int            signo, code;
	unsigned long  fp[8*3];
	unsigned long  fpcntl[3];	/* fp control regs */
	unsigned char  fpstate[FPSTATESIZE];  /* floating point state */
};

#define INIT_THREAD  {							\
	.ksp	= sizeof(init_stack) + (unsigned long) init_stack,	\
	.sr	= PS_S,							\
	.fs	= __KERNEL_DS,						\
}

#define setframeformat(_regs)	do { } while (0)

/*
 * Do necessary setup to start up a newly executed thread.
 */
static inline void start_thread(struct pt_regs * regs, unsigned long pc,
				unsigned long usp)
{
	regs->pc = pc;
	regs->sr &= ~0x2000;
	setframeformat(regs);
	wrusp(usp);
}

/* Forward declaration, a strange C thing */
struct task_struct;

/* Free all resources held by a thread. */
static inline void release_thread(struct task_struct *dead_task)
{
}

unsigned long get_wchan(struct task_struct *p);

#define	KSTK_EIP(tsk)	\
    ({			\
	unsigned long eip = 0;	 \
	if ((tsk)->thread.esp0 > PAGE_SIZE && \
	    (virt_addr_valid((tsk)->thread.esp0))) \
	      eip = ((struct pt_regs *) (tsk)->thread.esp0)->pc; \
	eip; })
#define	KSTK_ESP(tsk)	((tsk) == current ? rdusp() : (tsk)->thread.usp)

#define task_pt_regs(tsk)	((struct pt_regs *) ((tsk)->thread.esp0))

#define cpu_relax()	barrier()

#endif
