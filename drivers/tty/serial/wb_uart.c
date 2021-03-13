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

#define DRIVER_NAME	"wb_uart"

#define WB_UART_DATA	0x0
#define WB_UART_STATUS	0x4

#define WB_UART_IE	0x10
#define WB_UART_EMPTY	0x01
#define WB_UART_FULL	0x08

#define PORT_WB_UART	115

#define INFO() //pr_notice("%s:%d", __func__, __LINE__)

static struct uart_port ports[1];

static struct uart_port *console_port;

static void wb_uart_stop_tx(struct uart_port *port)
{
	INFO();
}

static void wb_uart_stop_rx(struct uart_port *port)
{
	INFO();
}

static void wb_uart_enable_ms(struct uart_port *port)
{
	INFO();
}

static void wb_uart_putchar(struct uart_port *port, int ch)
{
	while(ioread8(port->membase + WB_UART_STATUS) & WB_UART_FULL);
	iowrite8(ch, port->membase + WB_UART_DATA);
}

static void wb_uart_start_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;

	while(!uart_circ_empty(xmit)){
		wb_uart_putchar(port, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}
}

static unsigned int wb_uart_tx_empty(struct uart_port *port)
{
	INFO();
	return 1;
}

static irqreturn_t wb_uart_irq_handler(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;

	if(!(ioread8(port->membase + WB_UART_STATUS) & WB_UART_EMPTY)) {
		while(!(ioread8(port->membase + WB_UART_STATUS) & WB_UART_EMPTY)) {
			char c = ioread8(port->membase + WB_UART_DATA);
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

static unsigned int wb_uart_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void wb_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	INFO();
}

static void wb_uart_break_ctl(struct uart_port *port, int break_state)
{
	INFO();
}

static int wb_uart_startup(struct uart_port *port)
{
	int ret;
	INFO();

	ret = request_irq(port->irq, wb_uart_irq_handler, IRQF_SHARED, DRIVER_NAME, port);
	iowrite8(WB_UART_IE, port->membase + WB_UART_STATUS);

	return ret;
}

static void wb_uart_shutdown(struct uart_port *port)
{
	INFO();

	iowrite8(0x00, port->membase + WB_UART_STATUS);
	free_irq(port->irq, port);
}

static void wb_uart_set_termios(struct uart_port *port, struct ktermios *temios, struct ktermios *old)
{
	INFO();
}

static const char * wb_uart_type(struct uart_port *port)
{
	INFO();
	return port->type == PORT_WB_UART ? "WB UART" : NULL;
}

static void wb_uart_release_port(struct uart_port *port)
{
	INFO();
}

static int wb_uart_request_port(struct uart_port *port)
{
	INFO();
	return 0;
}

static void wb_uart_config_port(struct uart_port *port, int flags)
{
	INFO();
	if(flags & UART_CONFIG_TYPE)
		port->type = PORT_WB_UART;
}

static int wb_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	INFO();

	if(ser->type != PORT_UNKNOWN && ser->type != PORT_WB_UART)
		return -EINVAL;

	return ret;
}

static struct uart_ops wb_uart_ops = {
	.tx_empty = wb_uart_tx_empty,
	.set_mctrl = wb_uart_set_mctrl,
	.get_mctrl = wb_uart_get_mctrl,
	.stop_tx = wb_uart_stop_tx,
	.start_tx = wb_uart_start_tx,
	.stop_rx = wb_uart_stop_rx,
	.enable_ms = wb_uart_enable_ms,
	.break_ctl = wb_uart_break_ctl,
	.startup = wb_uart_startup,
	.shutdown = wb_uart_shutdown,
	.set_termios = wb_uart_set_termios,
	.type = wb_uart_type,
	.release_port = wb_uart_release_port,
	.request_port = wb_uart_request_port,
	.config_port = wb_uart_config_port,
	.verify_port = wb_uart_verify_port,
};

static void wb_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	unsigned long flags;
	spin_lock_irqsave(&console_port->lock, flags);
	uart_console_write(console_port, s, count, wb_uart_putchar);
	spin_unlock_irqrestore(&console_port->lock, flags);
}

static int __init wb_uart_console_setup(struct console *co, char *options)
{
	INFO();

	if(!console_port){
		INFO();
		return -ENODEV;
	}
	return 0;
}

static struct uart_driver wb_uart_drv;
static struct console wb_uart_console = {
	.name = "ttyS",
	.write = wb_uart_console_write,
	.device = uart_console_device,
	.setup = wb_uart_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &wb_uart_drv,
};

static int __init wb_uart_console_init(void)
{
	register_console(&wb_uart_console);
	return 0;
}

console_initcall(wb_uart_console_init);

static struct uart_driver wb_uart_drv = {
	.owner = THIS_MODULE,
	.driver_name = DRIVER_NAME,
	.dev_name = "ttyS",
	.major = TTY_MAJOR,
	.minor = 64,
	.nr = 2,
	.cons = &wb_uart_console,
};

static int wb_uart_suspend(struct platform_device *pdev, pm_message_t state)
{
	INFO();
	uart_suspend_port(&wb_uart_drv, &ports[0]);
	return 0;
}

static int wb_uart_resume(struct platform_device *pdev)
{
	uart_resume_port(&wb_uart_drv, &ports[0]);
	INFO();
	return 0;
}

static int wb_uart_probe(struct platform_device *pdev)
{
	struct uart_port *port;
	struct resource *res;

	INFO();

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	console_port = &ports[0];

	port = &ports[0];
	port->mapbase = res->start;
	port->membase = ioremap(port->mapbase, 0x100);
	port->irq = platform_get_irq(pdev, 0);
	port->uartclk = 1843200;
	port->iotype = UPIO_MEM;
	port->flags = UPF_BOOT_AUTOCONF;
	port->line = 0;
	port->ops = &wb_uart_ops;
	port->fifosize = 64;

	uart_add_one_port(&wb_uart_drv, port);

	return 0;
}

static int wb_uart_remove(struct platform_device *pdev)
{
	INFO();
	return 0;
}

static struct platform_driver wb_uart_plat = {
	.probe = wb_uart_probe,
	.remove = wb_uart_remove,
	.suspend = wb_uart_suspend,
	.resume = wb_uart_resume,
	.driver = {
		.name = DRIVER_NAME,
	},
};

static int __init wb_uart_init(void)
{
	int ret;

	INFO();
	
	ret = uart_register_driver(&wb_uart_drv);
	if(ret)
		return ret;
	
	ret = platform_driver_register(&wb_uart_plat);
	if(ret)
		uart_unregister_driver(&wb_uart_drv);

	return ret;
}

static void __exit wb_uart_exit(void)
{
	platform_driver_unregister(&wb_uart_plat);
	uart_unregister_driver(&wb_uart_drv);
}

module_init(wb_uart_init);
module_exit(wb_uart_exit);

MODULE_AUTHOR("Piotr Binkowski");
MODULE_DESCRIPTION("WB UART driver");
MODULE_LICENSE("GPL");
