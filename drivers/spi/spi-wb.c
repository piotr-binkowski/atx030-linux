#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/io.h>

#define DRIVER_NAME "wb_spi"

#define WB_SPI_DATA_WR		0x00
#define WB_SPI_DATA_RD		0x00
#define WB_SPI_DATA_RD_B	0x03
#define WB_SPI_STATUS		0x04

#define WB_SPI_RX_EMPTY		0x02
#define WB_SPI_TX_FULL		0x04
#define WB_SPI_FIFO_DEPTH	16

struct wb_spi {
	struct spi_bitbang bitbang;
	void __iomem *base;
};

static inline struct wb_spi *wb_spi_to_hw(struct spi_device *sdev)
{
	return spi_master_get_devdata(sdev->master);
}

static void wb_spi_chipsel(struct spi_device *spi, int value)
{
	struct wb_spi *hw = wb_spi_to_hw(spi);

	if(spi->mode & SPI_CS_HIGH){
		switch(value){
			case BITBANG_CS_INACTIVE:
				writeb(0, hw->base + WB_SPI_STATUS);
				break;
			case BITBANG_CS_ACTIVE:
				writeb(1, hw->base + WB_SPI_STATUS);
				break;
		}
	} else {
		switch(value){
			case BITBANG_CS_INACTIVE:
				writeb(1, hw->base + WB_SPI_STATUS);
				break;
			case BITBANG_CS_ACTIVE:
				writeb(0, hw->base + WB_SPI_STATUS);
				break;
		}
	}
}

static int wb_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct wb_spi *hw = wb_spi_to_hw(spi);
	void __iomem *base = hw->base;
	const u8 *txd = t->tx_buf;
	u8 *rxd = t->rx_buf;
	u8 leftover;
	u32 len;
	u16 i;

	leftover = t->len % 4;

	for(len = (t->len - leftover) / 4; len > 0;) {
		const u16 tgt_len = (len > WB_SPI_FIFO_DEPTH) ? WB_SPI_FIFO_DEPTH : len;
		u32 tmp = 0;
		for(i = 0; i < tgt_len; i++) {
			tmp = txd ? *((u32*)txd) : tmp;
			__raw_writel(tmp, base + WB_SPI_DATA_WR);
			if(txd)
				txd += 4;
		}
		for(i = 0; i < tgt_len; i++) {
			while(readb(base + WB_SPI_STATUS) & WB_SPI_RX_EMPTY);
			tmp = __raw_readl(base + WB_SPI_DATA_RD);
			if(rxd) {
				*((u32*)rxd) = tmp;
				rxd += 4;
			}
		}
		len -= tgt_len;
	}

	for(len = leftover; len > 0; len--) {
		u8 tmp = txd ? *(txd++) : 0;
		writeb(tmp, base + WB_SPI_DATA_WR);
		while(readb(base + WB_SPI_STATUS) & WB_SPI_RX_EMPTY);
		tmp = readb(base + WB_SPI_DATA_RD_B);
		if(rxd)
			*(rxd++) = tmp;
	}

	return t->len;
}

static int wb_spi_probe(struct platform_device *pdev)
{
	struct wb_spi *hw;
	struct spi_master *master;
	struct resource *res;

	master = spi_alloc_master(&pdev->dev, sizeof(struct wb_spi));
	if(!master)
		return -ENODEV;

	master->bus_num = pdev->id;
	master->num_chipselect = 1;
	master->mode_bits = SPI_CS_HIGH;
	master->bits_per_word_mask = SPI_BPW_MASK(8);

	hw = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, hw);
	
	hw->bitbang.master = master;
	hw->bitbang.chipselect = wb_spi_chipsel;
	hw->bitbang.txrx_bufs = wb_spi_txrx;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hw->base = devm_ioremap_resource(&pdev->dev, res);

	spi_bitbang_start(&hw->bitbang);

	printk(KERN_INFO "Wishbone SPI driver probed\n"); 
	
	return 0;
}

static int wb_spi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver wb_spi_driver = {
	.probe = wb_spi_probe,
	.remove = wb_spi_remove,
	.driver = {
		.name = DRIVER_NAME,
	},
};

module_platform_driver(wb_spi_driver);
