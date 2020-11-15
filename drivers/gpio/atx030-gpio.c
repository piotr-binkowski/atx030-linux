/*
 * 74xx MMIO GPIO driver
 *
 *  Copyright (C) 2014 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/gpio/driver.h>
#include <linux/platform_device.h>

struct atx030_gpio_priv {
	struct gpio_chip gc;
};

static int atx030_gpio_probe(struct platform_device *pdev)
{
	struct atx030_gpio_priv *priv;
	struct resource *res;
	void __iomem *dat;
	void __iomem *oe;
	int err;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dat = res->start;
	oe  = res->start + 1;

	err = bgpio_init(&priv->gc, &pdev->dev, 1, dat, dat, NULL, oe, NULL, 0);
	if (err)
		return err;

	priv->gc.ngpio = 2;
	priv->gc.owner = THIS_MODULE;

	platform_set_drvdata(pdev, priv);

	return devm_gpiochip_add_data(&pdev->dev, &priv->gc, priv);
}

static struct platform_driver atx030_gpio_driver = {
	.driver	= {
		.name		= "atx030-gpio",
	},
	.probe	= atx030_gpio_probe,
};
module_platform_driver(atx030_gpio_driver);

MODULE_AUTHOR("Piotr Binkowski");
MODULE_DESCRIPTION("ATX030 GPIO driver");
MODULE_LICENSE("GPL");
