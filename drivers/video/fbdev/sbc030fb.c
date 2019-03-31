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

#define INFO() pr_err("%s:%d\n", __func__, __LINE__)
#define INFO()

static struct fb_fix_screeninfo sbc030fb_fix = {
	.id = "SBC030",
	.smem_len = 640*200/8,
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_MONO10,
	.line_length = 640/8,
	.accel = FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo sbc030fb_var = {
	.xres = 640,
	.yres = 200,
	.xres_virtual = 640,
	.yres_virtual = 200,
	.bits_per_pixel = 1,
	.red = {0, 1, 0},
	.green = {0, 1, 0},
	.blue = {0, 1, 0},
	.activate = FB_ACTIVATE_NOW,
	.height = 230,
	.width = 300,
	.vmode = FB_VMODE_NONINTERLACED,
};

static int sbc030fb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue, unsigned transp, struct fb_info *info)
{
	INFO();
	return 0;
}

static struct fb_ops sbc030fb_ops = {
	.owner = THIS_MODULE,
	.fb_setcolreg = sbc030fb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

static int sbc030fb_probe(struct platform_device *pdev)
{
	struct fb_info *info;

	INFO();

	info = framebuffer_alloc(0, &pdev->dev);
	if(!info)
		return -ENOMEM;
	
	sbc030fb_fix.smem_start = 0xE0080000;

	info->var = sbc030fb_var;
	info->fix = sbc030fb_fix;

	info->fbops = &sbc030fb_ops;
	info->flags = FBINFO_DEFAULT;
	info->pseudo_palette = NULL;
	info->par = NULL;
	info->screen_base = (char*) sbc030fb_fix.smem_start;

	iowrite8(0x03, (void*)0xE0000003);

	if(register_framebuffer(info) < 0){
		framebuffer_release(info);
		return -EINVAL;
	}

	fb_info(info, "SBC030 frame buffer ready\n");

	return 0;
}

static struct platform_driver sbc030fb_driver = {
	.probe = sbc030fb_probe,
	.driver = {
		.name = "sbc030fb",
	},
};

static struct platform_device sbc030fb_device = {
	.name = "sbc030fb",
};

static int __init sbc030fb_init(void)
{
	int ret = 0;

	INFO();
	
	platform_driver_register(&sbc030fb_driver);

	platform_device_register(&sbc030fb_device);

	return 0;
}

module_init(sbc030fb_init);
MODULE_LICENSE(GPL);
