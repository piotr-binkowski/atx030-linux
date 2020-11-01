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

#define INFO() //pr_err("%s:%d\n", __func__, __LINE__)

#define XRES 640
#define YRES 400

static struct fb_fix_screeninfo atx030fb_fix = {
	.id = "ATX030",
	.smem_len = XRES*YRES/8,
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_MONO10,
	.line_length = XRES/8,
	.accel = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo atx030fb_var = {
	.xres = XRES,
	.yres = YRES,
	.xres_virtual = XRES,
	.yres_virtual = YRES,
	.bits_per_pixel = 1,
	.red = {0, 1, 0},
	.green = {0, 1, 0},
	.blue = {0, 1, 0},
	.activate = FB_ACTIVATE_NOW,
	.height = 230,
	.width = 300,
	.vmode = FB_VMODE_NONINTERLACED,
};

static int atx030fb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue, unsigned transp, struct fb_info *info)
{
	INFO();
	return 0;
}

static struct fb_ops atx030fb_ops = {
	.owner = THIS_MODULE,
	.fb_setcolreg = atx030fb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

static int atx030fb_probe(struct platform_device *pdev)
{
	struct fb_info *info;

	INFO();

	info = framebuffer_alloc(0, &pdev->dev);
	if(!info)
		return -ENOMEM;
	
	atx030fb_fix.smem_start = 0xE0010000;

	info->var = atx030fb_var;
	info->fix = atx030fb_fix;

	info->fbops = &atx030fb_ops;
	info->flags = FBINFO_DEFAULT;
	info->pseudo_palette = NULL;
	info->par = NULL;
	info->screen_base = (char*) atx030fb_fix.smem_start;

	if(register_framebuffer(info) < 0){
		framebuffer_release(info);
		return -EINVAL;
	}

	fb_info(info, "ATX030 frame buffer ready\n");

	return 0;
}

static struct platform_driver atx030fb_driver = {
	.probe = atx030fb_probe,
	.driver = {
		.name = "atx030fb",
	},
};

static struct platform_device atx030fb_device = {
	.name = "atx030fb",
};

static int __init atx030fb_init(void)
{
	INFO();
	
	platform_driver_register(&atx030fb_driver);

	platform_device_register(&atx030fb_device);

	return 0;
}

module_init(atx030fb_init);
MODULE_LICENSE(GPL);
