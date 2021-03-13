/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _M68K_BUG_H
#define _M68K_BUG_H

#ifdef CONFIG_BUG

#ifdef CONFIG_DEBUG_BUGVERBOSE
#define BUG() do { \
	pr_crit("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
	barrier_before_unreachable(); \
	__builtin_trap(); \
} while (0)
#else
#define BUG() do { \
	barrier_before_unreachable(); \
	__builtin_trap(); \
} while (0)
#endif

#define HAVE_ARCH_BUG
#endif

#include <asm-generic/bug.h>

#endif
