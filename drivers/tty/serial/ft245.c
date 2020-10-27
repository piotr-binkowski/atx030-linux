#include <linux/device.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/irq.h>

#define DRIVER_NAME	"ft245"

#define FT_TXE		0
#define FT_RXF		1
#define	FT_DATA		2
#define	FT_IRQEN	3
#define PORT_FT245	114

#define INFO() //pr_notice("%s:%d", __func__, __LINE__)

static struct uart_port ports[1];

static struct uart_port *console_port;

static void ft245_stop_tx(struct uart_port *port)
{
	INFO();
}

static void ft245_stop_rx(struct uart_port *port)
{
	INFO();
}

static void ft245_enable_ms(struct uart_port *port)
{
	INFO();
}

static void ft245_putchar(struct uart_port *port, int ch)
{
	while(ioread8(port->mapbase + FT_TXE));
	iowrite8(ch, port->mapbase + FT_DATA);
}

static void ft245_start_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;

	while(!uart_circ_empty(xmit)){
		ft245_putchar(port, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}
}

static unsigned int ft245_tx_empty(struct uart_port *port)
{
	INFO();
	return 1;
}

static irqreturn_t ft245_irq_handler(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;

	if(!ioread8(port->mapbase + FT_RXF)) {
		while(!ioread8(port->mapbase + FT_RXF)) {
			char c = ioread8(port->mapbase + FT_DATA);
			port->icount.rx++;
			uart_insert_char(port, 0, 0, c, TTY_NORMAL);
		}

		spin_unlock(&port->lock);
		tty_flip_buffer_push(&port->state->port);
		spin_lock(&port->lock);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static unsigned int ft245_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void ft245_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	INFO();
}

static void ft245_break_ctl(struct uart_port *port, int break_state)
{
	INFO();
}

static int ft245_startup(struct uart_port *port)
{
	int ret;
	INFO();

	ret = request_irq(port->irq, ft245_irq_handler, IRQF_SHARED, DRIVER_NAME, port);
	iowrite8(0x01, port->mapbase + FT_IRQEN);

	return ret;
}

static void ft245_shutdown(struct uart_port *port)
{
	INFO();

	iowrite8(0x00, port->mapbase + FT_IRQEN);
	free_irq(port->irq, port);
}

static void ft245_set_termios(struct uart_port *port, struct ktermios *temios, struct ktermios *old)
{
	INFO();
}

static const char * ft245_type(struct uart_port *port)
{
	INFO();
	return port->type == PORT_FT245 ? "FT245" : NULL;
}

static void ft245_release_port(struct uart_port *port)
{
	INFO();
}

static int ft245_request_port(struct uart_port *port)
{
	INFO();
	return 0;
}

static void ft245_config_port(struct uart_port *port, int flags)
{
	INFO();
	if(flags & UART_CONFIG_TYPE)
		port->type = PORT_FT245;
}

static int ft245_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	INFO();

	if(ser->type != PORT_UNKNOWN && ser->type != PORT_FT245)
		return -EINVAL;

	return ret;
}

static struct uart_ops ft245_ops = {
	.tx_empty = ft245_tx_empty,
	.set_mctrl = ft245_set_mctrl,
	.get_mctrl = ft245_get_mctrl,
	.stop_tx = ft245_stop_tx,
	.start_tx = ft245_start_tx,
	.stop_rx = ft245_stop_rx,
	.enable_ms = ft245_enable_ms,
	.break_ctl = ft245_break_ctl,
	.startup = ft245_startup,
	.shutdown = ft245_shutdown,
	.set_termios = ft245_set_termios,
	.type = ft245_type,
	.release_port = ft245_release_port,
	.request_port = ft245_request_port,
	.config_port = ft245_config_port,
	.verify_port = ft245_verify_port,
};

static void ft245_console_write(struct console *co, const char *s, unsigned int count)
{
	unsigned long flags;
	spin_lock_irqsave(&console_port->lock, flags);
	uart_console_write(console_port, s, count, ft245_putchar);
	spin_unlock_irqrestore(&console_port->lock, flags);
}

static int __init ft245_console_setup(struct console *co, char *options)
{
	INFO();

	if(!console_port){
		INFO();
		return -ENODEV;
	}
	return 0;
}

static struct uart_driver ft245_drv;
static struct console ft245_console = {
	.name = "ttyS",
	.write = ft245_console_write,
	.device = uart_console_device,
	.setup = ft245_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &ft245_drv,
};

static int __init ft245_console_init(void)
{
	register_console(&ft245_console);
	return 0;
}

console_initcall(ft245_console_init);

static struct uart_driver ft245_drv = {
	.owner = THIS_MODULE,
	.driver_name = DRIVER_NAME,
	.dev_name = "ttyS",
	.major = TTY_MAJOR,
	.minor = 64,
	.nr = 2,
	.cons = &ft245_console,
};

static int ft245_suspend(struct platform_device *pdev, pm_message_t state)
{
	INFO();
	uart_suspend_port(&ft245_drv, &ports[0]);
	return 0;
}

static int ft245_resume(struct platform_device *pdev)
{
	uart_resume_port(&ft245_drv, &ports[0]);
	INFO();
	return 0;
}

static int ft245_probe(struct platform_device *pdev)
{
	struct resource *r_mem, *r_irq;
	struct uart_port *port;
	int i;

	INFO();

	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	console_port = &ports[0];

	port = &ports[0];
	port->mapbase = r_mem->start;
	port->irq = r_irq->start;
	port->uartclk = 1843200;
	port->iotype = UPIO_MEM;
	port->flags = UPF_BOOT_AUTOCONF;
	port->line = 0;
	port->ops = &ft245_ops;
	port->fifosize = 64;

	uart_add_one_port(&ft245_drv, port);

	return 0;
}

static int ft245_remove(struct platform_device *pdev)
{
	INFO();
	return 0;
}

static struct platform_driver ft245_plat = {
	.probe = ft245_probe,
	.remove = ft245_remove,
	.suspend = ft245_suspend,
	.resume = ft245_resume,
	.driver = {
		.name = DRIVER_NAME,
	},
};

static int __init ft245_init(void)
{
	int ret;

	INFO();
	
	ret = uart_register_driver(&ft245_drv);
	if(ret)
		return ret;
	
	ret = platform_driver_register(&ft245_plat);
	if(ret)
		uart_unregister_driver(&ft245_drv);

	return ret;
}

static void __exit ft245_exit(void)
{
	platform_driver_unregister(&ft245_plat);
	uart_unregister_driver(&ft245_drv);
}

module_init(ft245_init);
module_exit(ft245_exit);

MODULE_AUTHOR("Piotr Binkowski");
MODULE_DESCRIPTION("FT245 serial driver");
MODULE_LICENSE("GPL");
