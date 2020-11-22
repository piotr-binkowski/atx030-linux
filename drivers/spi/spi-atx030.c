#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/io.h>

#define DRIVER_NAME "atx030,spi"

#define ATX030_SPI_SR	1
#define ATX030_SPI_CR	1
#define ATX030_SPI_DRWB	2
#define ATX030_SPI_DRRB	3
#define ATX030_SPI_DRW	4

#define ATX030_SPI_DONE	0x02

struct atx030_spi {
	struct spi_bitbang bitbang;
	void __iomem *base;
};

static inline struct atx030_spi *atx030_spi_to_hw(struct spi_device *sdev)
{
	return spi_master_get_devdata(sdev->master);
}

static void atx030_spi_chipsel(struct spi_device *spi, int value)
{
	struct atx030_spi *hw = atx030_spi_to_hw(spi);

	if(spi->mode & SPI_CS_HIGH){
		switch(value){
			case BITBANG_CS_INACTIVE:
				writeb(0, hw->base + ATX030_SPI_CR);
				break;
			case BITBANG_CS_ACTIVE:
				writeb(1, hw->base + ATX030_SPI_CR);
				break;
		}
	} else {
		switch(value){
			case BITBANG_CS_INACTIVE:
				writeb(1, hw->base + ATX030_SPI_CR);
				break;
			case BITBANG_CS_ACTIVE:
				writeb(0, hw->base + ATX030_SPI_CR);
				break;
		}
	}
}

static int atx030_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct atx030_spi *hw = atx030_spi_to_hw(spi);
	int len;
	uint8_t *txd, *rxd;

	txd = (uint8_t*)t->tx_buf;
	rxd = (uint8_t*)t->rx_buf;

	for(len = t->len; len > 0; len--){
		uint8_t rx, tx = txd ? *txd++ : 0xff;

		writeb(tx, hw->base + ATX030_SPI_DRWB);

		while(!(readb(hw->base + ATX030_SPI_SR) & ATX030_SPI_DONE));

		rx = readb(hw->base + ATX030_SPI_DRRB);

		if(rxd)
			*rxd++ = rx;
	}

	return t->len;
}

static int atx030_spi_probe(struct platform_device *pdev)
{
	struct atx030_spi *hw;
	struct spi_master *master;
	struct resource *res;

	master = spi_alloc_master(&pdev->dev, sizeof(struct atx030_spi));
	if(!master)
		return -ENODEV;

	master->bus_num = pdev->id;
	master->num_chipselect = 1;
	master->mode_bits = SPI_CS_HIGH;
	master->bits_per_word_mask = SPI_BPW_MASK(8);

	hw = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, hw);
	
	hw->bitbang.master = master;
	hw->bitbang.chipselect = atx030_spi_chipsel;
	hw->bitbang.txrx_bufs = atx030_spi_txrx;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hw->base = devm_ioremap_resource(&pdev->dev, res);

	spi_bitbang_start(&hw->bitbang);
	
	return 0;
}

static int atx030_spi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver atx030_spi_driver = {
	.probe = atx030_spi_probe,
	.remove = atx030_spi_remove,
	.driver = {
		.name = DRIVER_NAME,
	},
};

module_platform_driver(atx030_spi_driver);
