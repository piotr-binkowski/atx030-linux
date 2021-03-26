#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/dma-mapping.h>
#include <asm/atx040hw.h>

#define XRES 640
#define YRES 400
#define PIXW 2

static struct fb_fix_screeninfo atx040fb_fix = {
	.id          = "ATX040",
	.smem_len    = XRES*YRES*PIXW,
	.type        = FB_TYPE_PACKED_PIXELS,
	.visual      = FB_VISUAL_TRUECOLOR,
	.line_length = XRES*PIXW,
	.accel       = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo atx040fb_var = {
	.xres           = XRES,
	.yres           = YRES,
	.xres_virtual   = XRES,
	.yres_virtual   = YRES,
	.bits_per_pixel = 16,
	.red            = {0, 5, 0},
	.green          = {5, 6, 0},
	.blue           = {11, 5, 0},
	.activate       = FB_ACTIVATE_NOW,
	.height         = 230,
	.width          = 300,
	.vmode          = FB_VMODE_NONINTERLACED,
};

static int atx040fb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue, unsigned transp, struct fb_info *info)
{
	if(regno >= 16)
		return 1;

	red>>=11;
	green>>=10;
	blue>>=11;

	if(regno < 16)
		((u32*)info->pseudo_palette)[regno] = (red & 31) | ((green & 63) << 5) | ((blue & 31) << 11);

	return 0;
}

static struct fb_ops atx040fb_ops = {
	.owner        = THIS_MODULE,
	.fb_setcolreg = atx040fb_setcolreg,
	.fb_fillrect  = cfb_fillrect,
	.fb_copyarea  = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

static int atx040fb_enable(dma_addr_t fb_base)
{
	void *base = ioremap(ATX040_VGA_BASE, 16);

	if(!base)
		return -ENOMEM;

	__raw_writel(fb_base, base + 4);
	__raw_writel(1, base);

	iounmap(base);

	return 0;
}

static int atx040fb_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	dma_addr_t fb_phys;
	void *fb_virt;
	int ret;


	info = framebuffer_alloc(sizeof(u32)*16, &pdev->dev);
	if (!info)
		return -ENOMEM;

	fb_virt = dma_alloc_wc(&pdev->dev, XRES*YRES*PIXW, &fb_phys, GFP_KERNEL);

	if(!fb_virt) {
		ret = -ENOMEM;
		goto exit0;
	}
	
	info->var = atx040fb_var;
	info->fix = atx040fb_fix;
	info->fbops = &atx040fb_ops;
	info->flags = FBINFO_DEFAULT;
	info->pseudo_palette = info->par;
	info->par = NULL;

	info->screen_base = fb_virt;
	atx040fb_fix.smem_start = fb_phys;

	ret = fb_alloc_cmap(&info->cmap, 256, 0);

	if (ret < 0)
		goto exit1;

	ret = atx040fb_enable(fb_phys);

	if (ret < 0)
		goto exit2;

	ret = register_framebuffer(info);

	if (ret < 0)
		goto exit2;

	fb_info(info, "ATX040 frame buffer ready\n");

	return 0;

exit2:
	fb_dealloc_cmap(&info->cmap);
exit1:
	dma_free_wc(&pdev->dev, XRES*YRES*PIXW, fb_virt, fb_phys);
exit0:
	framebuffer_release(info);

	printk(KERN_ERR "Failed to probe ATX040 frame buffer\n");

	return ret;
}

static struct platform_driver atx040fb_driver = {
	.probe = atx040fb_probe,
	.driver = {
		.name = "atx040fb",
	},
};

static struct platform_device atx040fb_device = {
	.name = "atx040fb",
};

static int __init atx040fb_init(void)
{
	platform_driver_register(&atx040fb_driver);

	platform_device_register(&atx040fb_device);

	return 0;
}

module_init(atx040fb_init);
MODULE_LICENSE(GPL);
