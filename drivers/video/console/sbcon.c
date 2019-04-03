/*
 * Based on vgacon.c and mdacon.c
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/string.h>
#include <linux/kd.h>
#include <linux/slab.h>
#include <linux/vt_kern.h>
#include <linux/sched.h>
#include <linux/selection.h>
#include <linux/spinlock.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/screen_info.h>
#include <video/vga.h>
#include <asm/io.h>

#define INFO() pr_err("%s:%d\n", __func__, __LINE__)

static struct uni_pagedir *sbc_uni_pagedir;
static int sbc_refcount;

static u16 *sbc_base = (u16*)0xe0080000;
static unsigned sbc_cols = 80;
static unsigned sbc_rows = 25;
static unsigned sbc_font_height = 16;
static unsigned sbc_lines = 400;

static u16 *sbc_addr(int x, int y)
{
	return sbc_base + sbc_cols * y + x;
}

static void sbc_con_putc(struct vc_data *c, int ch, int y, int x)
{
	scr_writew(ch, sbc_addr(x, y));
}

static void sbc_con_putcs(struct vc_data *c, const unsigned short *s, int count, int y, int x)
{
	u16 *dest = sbc_addr(x, y);

	for(; count > 0; count--) {
		scr_writew(scr_readw(s++), dest++);
	}
}

static void sbc_con_invert_region(struct vc_data *c, u16 *p, int count)
{
	for(; count > 0; count--, p++) {
		scr_writew(scr_readw(p) ^ c->vc_complement_mask, p);
	}
}

static void sbc_con_clear(struct vc_data *c, int y, int x, int height, int width)
{
	u16 *dest = sbc_addr(x, y);

	if(x == 0 && width == sbc_cols) {
		scr_memsetw(dest, c->vc_video_erase_char, height * width * 2);
	} else {
		for(; height > 0; height--, dest += sbc_cols) {
			scr_memsetw(dest, c->vc_video_erase_char, width * 2);
		}
	}
}

static void sbc_con_cursor(struct vc_data *c, int mode)
{
	u16 ch = c->vc_screenbuf[c->vc_x + c->vc_y * c->vc_cols];
	switch(mode) {
	case CM_ERASE:
		sbc_con_putc(c, ch, c->vc_y, c->vc_x);
		break;
	case CM_MOVE:
	case CM_DRAW:
		sbc_con_putc(c, ch ^ c->vc_complement_mask, c->vc_y, c->vc_x);
		break;
	}
}

static bool sbc_con_scroll(struct vc_data *c, unsigned int t, unsigned int b, enum con_scroll dir, unsigned int lines)
{
	if(!lines) {
		return false;
	}

	if(lines > c->vc_rows) {
		lines = c->vc_rows;
	}

	switch(dir) {
	case SM_UP:
		scr_memmovew(sbc_addr(0, t), sbc_addr(0, t + lines), (b - t - lines) * sbc_cols * 2);
		scr_memsetw(sbc_addr(0, b - lines), c->vc_video_erase_char, lines * sbc_cols * 2);
		break;
	case SM_DOWN:
		scr_memmovew(sbc_addr(0, t + lines), sbc_addr(0, t), (b - t - lines) * sbc_cols * 2);
		scr_memsetw(sbc_addr(0, t), c->vc_video_erase_char, lines * sbc_cols * 2);
		break;
	}

	return false;
}

static int sbc_con_switch(struct vc_data *c)
{
	return 1;
}

static const char *sbc_con_startup(void)
{
	return "SBC-CON";
}

static int sbc_con_blank(struct vc_data *c, int blank, int mode_switch)
{
	if(blank) {
		sbc_con_clear(c, 0, 0, sbc_rows, sbc_cols);
	}
	return 1;
}

static u8 sbc_con_build_attr(struct vc_data *c, u8 color, u8 intensity, u8 blink, u8 underline, u8 reverse, u8 italic)
{
	u8 attr = color;

	return attr;
}

static void sbc_con_init(struct vc_data *c, int init)
{
	struct uni_pagedir *p;

	c->vc_can_do_color = 1;

	if(init) {
		c->vc_cols = sbc_cols;
		c->vc_rows = sbc_rows;
	} else {
		vc_resize(c, sbc_cols, sbc_rows);
	}

	c->vc_scan_lines = sbc_lines;
	c->vc_font.height = sbc_font_height;
	c->vc_complement_mask = 0x7700;

	p = *c->vc_uni_pagedir_loc;

	if(c->vc_uni_pagedir_loc != &sbc_uni_pagedir) {
		con_free_unimap(c);
		c->vc_uni_pagedir_loc = &sbc_uni_pagedir;
		sbc_refcount++;
	}

	if(!sbc_uni_pagedir && p)
		con_set_default_unimap(c);

}

static void sbc_con_deinit(struct vc_data *c)
{
	if(!--sbc_refcount)
		con_free_unimap(c);

	c->vc_uni_pagedir_loc = &c->vc_uni_pagedir;
	con_set_default_unimap(c);
}

const struct consw sbc_con = {
	.owner = THIS_MODULE,
	.con_startup = sbc_con_startup,
	.con_init = sbc_con_init,
	.con_deinit = sbc_con_deinit,
	.con_clear = sbc_con_clear,
	.con_putc = sbc_con_putc,
	.con_putcs = sbc_con_putcs,
	.con_cursor = sbc_con_cursor,
	.con_scroll = sbc_con_scroll,
	.con_switch = sbc_con_switch,
	.con_blank = sbc_con_blank,
	.con_build_attr = sbc_con_build_attr,
	.con_invert_region = sbc_con_invert_region,
};

EXPORT_SYMBOL(sbc_con);

MODULE_LICENSE("GPL");
