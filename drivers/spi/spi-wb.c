#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/io.h>

#define DRIVER_NAME "wb_spi"

#define WB_SPI_DATA	0x00
#define WB_SPI_STATUS	0x04

#define WB_SPI_RX_EMPTY	0x02
#define WB_SPI_TX_FULL	0x04

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
	int tx_len = 0, rx_len = 0;
	uint8_t *txd, *rxd;
	int len;

	txd = (uint8_t*)t->tx_buf;
	rxd = (uint8_t*)t->rx_buf;


	for(len = t->len; rx_len < len;) {
		while((tx_len < len) && !(readb(hw->base + WB_SPI_STATUS) & WB_SPI_TX_FULL)) {
			uint8_t tmp = txd ? txd[tx_len] : 0xff;

			writeb(tmp, hw->base + WB_SPI_DATA);

			tx_len++;
		}
		while((rx_len < len) && !(readb(hw->base + WB_SPI_STATUS) & WB_SPI_RX_EMPTY)) {
			uint8_t tmp = readb(hw->base + WB_SPI_DATA);

			if(rxd)
				rxd[rx_len] = tmp;

			rx_len++;
		}
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
