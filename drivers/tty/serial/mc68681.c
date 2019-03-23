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

#define DRIVER_NAME "mc,68681"

#define UART_TB 7
#define UART_SR 3
#define UART_ISR 11
#define PORT_MC68681 114

//#define INFO() pr_notice("%s:%d", __func__, __LINE__)
#define INFO()

static struct uart_port ports[1];

static struct uart_port *console_port;

static void mc68681_stop_tx(struct uart_port *port)
{
	INFO();
}

static void mc68681_stop_rx(struct uart_port *port)
{
	INFO();
}

static void mc68681_enable_ms(struct uart_port *port)
{
	INFO();
}

static void mc68681_putchar(struct uart_port *port, int ch)
{
	while(!(ioread8(port->mapbase + UART_SR) & 0x04));
	iowrite8(ch, port->mapbase + UART_TB);
}

static void mc68681_start_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;

	while(!uart_circ_empty(xmit)){
		mc68681_putchar(port, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}
}

static unsigned int mc68681_tx_empty(struct uart_port *port)
{
	INFO();
	return 1;
}

static irqreturn_t mc68681_irq_handler(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	int isr = ioread8(port->mapbase + UART_ISR);

	if(isr & 0x02){
		char c = ioread8(port->mapbase + UART_TB);
		port->icount.rx++;
		uart_insert_char(port, 0, 0, c, TTY_NORMAL);

		spin_unlock(&port->lock);
		tty_flip_buffer_push(&port->state->port);
		spin_lock(&port->lock);

		return IRQ_HANDLED;
	} else {
		return IRQ_NONE;
	}
}

static unsigned int mc68681_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void mc68681_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	INFO();
}

static void mc68681_break_ctl(struct uart_port *port, int break_state)
{
	INFO();
}

static int mc68681_startup(struct uart_port *port)
{
	int ret;
	INFO();

	ret = request_irq(port->irq, mc68681_irq_handler, IRQF_SHARED, DRIVER_NAME, port);

	iowrite8(0x02, port->mapbase + UART_ISR);

	return ret;
}

static void mc68681_shutdown(struct uart_port *port)
{
	INFO();
	free_irq(port->irq, port);
}

static void mc68681_set_termios(struct uart_port *port, struct ktermios *temios, struct ktermios *old)
{
	INFO();

}

static const char * mc68681_type(struct uart_port *port)
{
	INFO();
	return port->type == PORT_MC68681 ? "MC68681" : NULL;
}

static void mc68681_release_port(struct uart_port *port)
{
	INFO();

}

static int mc68681_request_port(struct uart_port *port)
{
	INFO();
	return 0;
}

static void mc68681_config_port(struct uart_port *port, int flags)
{
	INFO();
	if(flags & UART_CONFIG_TYPE)
		port->type = PORT_MC68681;
}

static int mc68681_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	INFO();

	if(ser->type != PORT_UNKNOWN && ser->type != PORT_MC68681)
		return -EINVAL;

	return ret;
}

static struct uart_ops mc68681_ops = {
	.tx_empty = mc68681_tx_empty,
	.set_mctrl = mc68681_set_mctrl,
	.get_mctrl = mc68681_get_mctrl,
	.stop_tx = mc68681_stop_tx,
	.start_tx = mc68681_start_tx,
	.stop_rx = mc68681_stop_rx,
	.enable_ms = mc68681_enable_ms,
	.break_ctl = mc68681_break_ctl,
	.startup = mc68681_startup,
	.shutdown = mc68681_shutdown,
	.set_termios = mc68681_set_termios,
	.type = mc68681_type,
	.release_port = mc68681_release_port,
	.request_port = mc68681_request_port,
	.config_port = mc68681_config_port,
	.verify_port = mc68681_verify_port,
};

static void mc68681_console_write(struct console *co, const char *s, unsigned int count)
{
	unsigned long flags;
	spin_lock_irqsave(&console_port->lock, flags);
	uart_console_write(console_port, s, count, mc68681_putchar);
	spin_unlock_irqrestore(&console_port->lock, flags);
}

static int __init mc68681_console_setup(struct console *co, char *options)
{
	INFO();

	if(!console_port){
		INFO();
		return -ENODEV;
	}
	return 0;
}

static struct uart_driver mc68681_drv;
static struct console mc68681_console = {
	.name = "ttyS",
	.write = mc68681_console_write,
	.device = uart_console_device,
	.setup = mc68681_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &mc68681_drv,
};

static int __init mc68681_console_init(void)
{
	register_console(&mc68681_console);
	return 0;
}

console_initcall(mc68681_console_init);

static struct uart_driver mc68681_drv = {
	.owner = THIS_MODULE,
	.driver_name = DRIVER_NAME,
	.dev_name = "ttyS",
	.major = TTY_MAJOR,
	.minor = 64,
	.nr = 2,
	.cons = &mc68681_console,
};

static int mc68681_suspend(struct platform_device *pdev, pm_message_t state)
{
	INFO();
	uart_suspend_port(&mc68681_drv, &ports[0]);
	return 0;
}

static int mc68681_resume(struct platform_device *pdev)
{
	uart_resume_port(&mc68681_drv, &ports[0]);
	INFO();
	return 0;
}

static int mc68681_probe(struct platform_device *pdev)
{
	struct resource *r_mem, *r_irq;

	INFO();

	struct uart_port *port = &ports[0];
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	port->mapbase = r_mem->start;
	port->membase = ioremap(r_mem->start, 32);
	port->irq = r_irq->start;
	port->uartclk = 1843200;
	port->iotype = UPIO_MEM;
	port->flags = UPF_BOOT_AUTOCONF;
	port->line = 0;
	port->ops = &mc68681_ops;
	port->fifosize = 1;

	console_port = port;

	uart_add_one_port(&mc68681_drv, port);

	return 0;
}

static int mc68681_remove(struct platform_device *pdev)
{
	INFO();
	return 0;
}

static struct platform_driver mc68681_plat = {
	.probe = mc68681_probe,
	.remove = mc68681_remove,
	.suspend = mc68681_suspend,
	.resume = mc68681_resume,
	.driver = {
		.name = DRIVER_NAME,
	},
};

static int __init mc68681_init(void)
{
	int ret;

	INFO();
	
	ret = uart_register_driver(&mc68681_drv);

	if(ret)
		return ret;
	
	ret = platform_driver_register(&mc68681_plat);
	if(ret)
		uart_unregister_driver(&mc68681_drv);

	return ret;
}

static void __exit mc68681_exit(void)
{
	platform_driver_unregister(&mc68681_plat);
	uart_unregister_driver(&mc68681_drv);
}

module_init(mc68681_init);
module_exit(mc68681_exit);

MODULE_AUTHOR("Piotr Binkowski");
MODULE_DESCRIPTION("MC68681 serial driver");
MODULE_LICENSE("MIT");
