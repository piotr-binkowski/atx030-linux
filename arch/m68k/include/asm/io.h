/* SPDX-License-Identifier: GPL-2.0 */
/*
 * linux/include/asm-m68k/io.h
 *
 * 4/1/00 RZ: - rewritten to avoid clashes between ISA/PCI and other
 *              IO access
 *            - added Q40 support
 *            - added skeleton for GG-II and Amiga PCMCIA
 * 2/3/01 RZ: - moved a few more defs into raw_io.h
 *
 * inX/outX should not be used by any driver unless it does
 * ISA access. Other drivers should use function defined in raw_io.h
 * or define its own macros on top of these.
 *
 *    inX(),outX()              are for ISA I/O
 *    isa_readX(),isa_writeX()  are for ISA memory
 */

#ifndef _M68K_IO_MM_H
#define _M68K_IO_MM_H

#include <linux/compiler.h>
#include <asm/raw_io.h>
#include <asm/virtconvert.h>
#include <asm/kmap.h>

#include <asm-generic/iomap.h>

/*
 * IO/MEM definitions for various ISA bridges
 */

#define readl(addr)      in_le32(addr)
#define writel(val,addr) out_le32((addr),(val))

#define readsb(port, buf, nr)     raw_insb((port), (u8 *)(buf), (nr))
#define readsw(port, buf, nr)     raw_insw((port), (u16 *)(buf), (nr))
#define readsl(port, buf, nr)     raw_insl((port), (u32 *)(buf), (nr))
#define writesb(port, buf, nr)    raw_outsb((port), (u8 *)(buf), (nr))
#define writesw(port, buf, nr)    raw_outsw((port), (u16 *)(buf), (nr))
#define writesl(port, buf, nr)    raw_outsl((port), (u32 *)(buf), (nr))

#define mmiowb()

#define IO_SPACE_LIMIT 0xffff

#define __ARCH_HAS_NO_PAGE_ZERO_MAPPED		1

/*
 * Convert a physical pointer to a virtual kernel pointer for /dev/mem
 * access
 */
#define xlate_dev_mem_ptr(p)	__va(p)

/*
 * Convert a virtual cached pointer to an uncached pointer
 */
#define xlate_dev_kmem_ptr(p)	p

#define readb_relaxed(addr)	readb(addr)
#define readw_relaxed(addr)	readw(addr)
#define readl_relaxed(addr)	readl(addr)

#define writeb_relaxed(b, addr)	writeb(b, addr)
#define writew_relaxed(b, addr)	writew(b, addr)
#define writel_relaxed(b, addr)	writel(b, addr)

#include <asm-generic/io.h>

#endif /* _M68K_IO_MM_H */
