// SPDX-License-Identifier: GPL-2.0
#include <linux/clocksource.h>
#include <linux/sched_clock.h>
#include <linux/of_address.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/clk.h>

#define WB_TIMER_HI 4
#define WB_TIMER_LO 0

static void __iomem *wb_timer_base;

struct wb_timer_priv {
	void __iomem *reg;
	struct clocksource clksrc;
};

static inline struct wb_timer_priv *to_wb_timer_priv(struct clocksource *c)
{
	return container_of(c, struct wb_timer_priv, clksrc);
}

static u64 notrace read_wb_timer(void __iomem *base)
{
	u32 tmp1, tmp2, tmp3;
	do {
		tmp1 = __raw_readl(base + WB_TIMER_HI);
		tmp2 = __raw_readl(base + WB_TIMER_LO);
		tmp3 = __raw_readl(base + WB_TIMER_HI);
	} while (tmp1 != tmp3);

	return ((u64)tmp1 << 32) | tmp2;
}

static u64 read_clocksource_clock(struct clocksource *c)
{
	return read_wb_timer(to_wb_timer_priv(c)->reg);
}

static u64 notrace read_sched_clock(void)
{
	return read_wb_timer(wb_timer_base);
}

static int __init atx_clocksource_init(struct device_node *np)
{
	struct wb_timer_priv *priv;
	void __iomem *base;
	int tim_freq, ret;
	struct clk *clk;

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("%pOF: invalid address\n", np);
		return -ENXIO;
	}

	clk = of_clk_get(np, 0);
	if (IS_ERR(clk)) {
		pr_err("%pOF: invalid clock\n", np);
		return PTR_ERR(clk);
	}

	tim_freq = clk_get_rate(clk);

	priv = kzalloc(sizeof(struct wb_timer_priv), GFP_KERNEL);
	if(!priv)
		return -ENOMEM;

	priv->reg = base;
	priv->clksrc.name = "wb-timer";
	priv->clksrc.rating = 350;
	priv->clksrc.read = read_clocksource_clock;
	priv->clksrc.mask = CLOCKSOURCE_MASK(64);
	priv->clksrc.flags = CLOCK_SOURCE_IS_CONTINUOUS;

	ret = clocksource_register_hz(&priv->clksrc, tim_freq);
	ret  = 0;

	if (ret) {
		pr_err("%pOF: registration failed\n", np);
		return ret;
	}

	if(of_property_read_bool(np, "atx,sched-clock")) {
		wb_timer_base = base;
		sched_clock_register(read_sched_clock, 64, tim_freq);
	};

	return 0;
}

TIMER_OF_DECLARE(wb_timer, "atx,wb-timer", atx_clocksource_init);
