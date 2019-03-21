#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/io.h>

#define DRIVER_NAME "sbc030,spi"

#define SBC030_SPI_CR 0
#define SBC030_SPI_SR 0
#define SBC030_SPI_DR 1

struct sbc030_spi {
	struct spi_bitbang bitbang;
	void __iomem *base;
};

static inline struct sbc030_spi *sbc030_spi_to_hw(struct spi_device *sdev)
{
	return spi_master_get_devdata(sdev->master);
}

static void sbc030_spi_chipsel(struct spi_device *spi, int value)
{
	struct sbc030_spi *hw = sbc030_spi_to_hw(spi);

	if(spi->mode & SPI_CS_HIGH){
		switch(value){
			case BITBANG_CS_INACTIVE:
				writeb(0, hw->base + SBC030_SPI_CR);
				break;
			case BITBANG_CS_ACTIVE:
				writeb(1, hw->base + SBC030_SPI_CR);
				break;
		}
	} else {
		switch(value){
			case BITBANG_CS_INACTIVE:
				writeb(1, hw->base + SBC030_SPI_CR);
				break;
			case BITBANG_CS_ACTIVE:
				writeb(0, hw->base + SBC030_SPI_CR);
				break;
		}
	}
}

static int sbc030_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct sbc030_spi *hw = sbc030_spi_to_hw(spi);
	int count;
	int len = t->len;
	uint8_t *tx, *rx;

	tx = (uint8_t*)t->tx_buf;
	rx = (uint8_t*)t->rx_buf;

	for(count = 0; count < len; count++){

		if(tx)
			writeb(tx[count], hw->base + SBC030_SPI_DR);
		else
			writeb(0, hw->base + SBC030_SPI_DR);

		while(!(readb(hw->base + SBC030_SPI_SR) & 0x02));

		if(rx)
			rx[count] = readb(hw->base + SBC030_SPI_DR);
	}

	return count;
}

static int sbc030_spi_probe(struct platform_device *pdev)
{
	struct sbc030_spi *hw;
	struct spi_master *master;
	struct resource *res;

	dev_info(&pdev->dev, "SBC030 SPI Driver\n");

	master = spi_alloc_master(&pdev->dev, sizeof(struct sbc030_spi));
	if(!master)
		return -ENODEV;

	master->bus_num = pdev->id;
	master->num_chipselect = 1;
	master->mode_bits = SPI_CS_HIGH;
	master->bits_per_word_mask = SPI_BPW_MASK(8);

	hw = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, hw);
	
	hw->bitbang.master = master;
	hw->bitbang.chipselect = sbc030_spi_chipsel;
	hw->bitbang.txrx_bufs = sbc030_spi_txrx;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hw->base = devm_ioremap_resource(&pdev->dev, res);

	spi_bitbang_start(&hw->bitbang);
	
	return 0;
}

static int sbc030_spi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver sbc030_spi_driver = {
	.probe = sbc030_spi_probe,
	.remove = sbc030_spi_remove,
	.driver = {
		.name = DRIVER_NAME,
	},
};

module_platform_driver(sbc030_spi_driver);
