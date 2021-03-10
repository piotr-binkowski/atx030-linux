#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/sysrq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/irq.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#define DRIVER_NAME "atx040,i2s"

#define ATX040_I2S_CTRL		0x00
#define ATX040_I2S_DATA		0x04

#define ATX040_I2S_TXEN		0x01
#define ATX040_I2S_MONO		0x02
#define ATX040_I2S_BE		0x04
#define ATX040_I2S_IRQEN	0x08

#define FRAME_BYTES  (2)
#define PERIOD_BYTES (4096)
#define PERIODS_MAX  (512)
#define BUF_BYTES    (PERIODS_MAX*PERIOD_BYTES)

struct atx040_i2s_priv {
	struct platform_device *pdev;

	void __iomem *base;
	int irq;

	struct snd_card *card;

	unsigned int be_mode;
	unsigned int buf_pos;
	unsigned int buf_siz;

	struct snd_pcm *pcm;
	struct snd_pcm_hardware pcm_hw;
	struct snd_pcm_substream *substream;
};

static struct snd_pcm_hardware atx040_i2s_hw = {
	.info             = (SNDRV_PCM_INFO_INTERLEAVED |
			     SNDRV_PCM_INFO_NONINTERLEAVED |
			     SNDRV_PCM_INFO_MMAP |
			     SNDRV_PCM_INFO_BLOCK_TRANSFER |
			     SNDRV_PCM_INFO_MMAP_VALID),
	.formats          = (SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_S16_LE),
	.rates            = SNDRV_PCM_RATE_44100,
	.rate_min         = 44100,
	.rate_max         = 44100,
	.channels_min     = 1,
	.channels_max     = 1,
	.buffer_bytes_max = BUF_BYTES,
	.period_bytes_min = PERIOD_BYTES,
	.period_bytes_max = PERIOD_BYTES,
	.periods_min      = 1,
	.periods_max      = PERIODS_MAX,
};

static int atx040_i2s_fill_fifo(struct atx040_i2s_priv *priv, const u16 *buf, const int len) {
	int i;
	for (i = 0; i < len; i++) {
		__raw_writew(*buf++, priv->base + ATX040_I2S_DATA);
	}
	return len;
}

static irqreturn_t atx040_i2s_irq_handler(int irq, void *ptr)
{
	struct atx040_i2s_priv *priv = ptr;
	u16 *buf;
	int len;
	if (priv->substream && priv->substream->runtime && priv->substream->runtime->dma_area) {
		buf = (u16*)priv->substream->runtime->dma_area;

		len = (PERIOD_BYTES/FRAME_BYTES);

		if((priv->buf_pos + len) > priv->buf_siz)
			len = priv->buf_siz - priv->buf_pos;

		priv->buf_pos += atx040_i2s_fill_fifo(priv, buf + priv->buf_pos, len);
		if(priv->buf_pos == priv->buf_siz)
		       priv->buf_pos = 0;

		if(priv->buf_pos == 0) {
			len = (PERIOD_BYTES/FRAME_BYTES) - len;
			priv->buf_pos += atx040_i2s_fill_fifo(priv, buf, len);
		}

		snd_pcm_period_elapsed(priv->substream);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int atx040_i2s_open(struct snd_pcm_substream *substream)
{
	struct atx040_i2s_priv *priv = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;

	if(!priv) {
		pr_info("Failed to retrieve chip\n");
		return -1;
	}

	runtime->hw     = atx040_i2s_hw;
	priv->pcm_hw    = runtime->hw;
	priv->substream = substream;
	return 0;
}

static int atx040_i2s_close(struct snd_pcm_substream *substream)
{
	return 0;
}

static int atx040_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
}

static int atx040_i2s_hw_free(struct snd_pcm_substream *substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int atx040_i2s_prepare(struct snd_pcm_substream *substream)
{
	struct atx040_i2s_priv *priv = snd_pcm_substream_chip(substream);

	priv->buf_pos = 0;
	priv->buf_siz = substream->runtime->buffer_size;

	priv->be_mode = substream->runtime->format & SNDRV_PCM_FMTBIT_S16_BE;

	writeb(0, priv->base + ATX040_I2S_CTRL);

	return 0;
}

static int atx040_i2s_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct atx040_i2s_priv *priv = snd_pcm_substream_chip(substream);
	u16 *buf;
	u32 reg;
	int len;

	switch(cmd) {
		case SNDRV_PCM_TRIGGER_START:
			len = (PERIOD_BYTES/FRAME_BYTES);
			buf = (u16*)priv->substream->runtime->dma_area;

			priv->buf_pos += atx040_i2s_fill_fifo(priv, buf + priv->buf_pos, len);
			priv->buf_pos += atx040_i2s_fill_fifo(priv, buf + priv->buf_pos, len);

			reg = ATX040_I2S_IRQEN | ATX040_I2S_TXEN | ATX040_I2S_MONO;

			if(priv->be_mode)
				reg |= ATX040_I2S_BE;

			__raw_writel(reg , priv->base + ATX040_I2S_CTRL);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t atx040_i2s_pointer(struct snd_pcm_substream *substream)
{
	struct atx040_i2s_priv *priv = snd_pcm_substream_chip(substream);

	return priv->buf_pos;
}

static struct snd_pcm_ops atx040_i2s_ops = {
	.open      = atx040_i2s_open,
	.close     = atx040_i2s_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = atx040_i2s_hw_params,
	.hw_free   = atx040_i2s_hw_free,
	.prepare   = atx040_i2s_prepare,
	.trigger   = atx040_i2s_trigger,
	.pointer   = atx040_i2s_pointer,
};

static int atx040_i2s_new_pcm(struct atx040_i2s_priv *priv)
{
	struct snd_pcm *pcm;
	int ret;

	ret = snd_pcm_new(priv->card, "ATX040 PCM", 0, 1, 0, &pcm);
	if (ret) {
		dev_err(&priv->pdev->dev, "Failed to create new PCM device\n");
		return ret;
	}

	pcm->private_data = priv;
	strcpy(pcm->name, "ATX040 PCM");
	priv->pcm = pcm;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &atx040_i2s_ops);

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
					      snd_dma_continuous_data(GFP_KERNEL),
					      BUF_BYTES, BUF_BYTES);

	return 0;
}

static int atx040_i2s_probe(struct platform_device *pdev)
{
	struct resource *r_mem, *r_irq;
	struct atx040_i2s_priv *priv;
	struct snd_card *card;
	int ret;

	ret = snd_card_new(&pdev->dev, 0, NULL, THIS_MODULE, sizeof(*priv), &card);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create new soundcard\n");
		return ret;
	}

	priv = card->private_data;

	priv->card = card;
	priv->pdev = pdev;

	strcpy(card->driver, "ATX040 I2S");
	strcpy(card->shortname, "ATX040 I2S");
	strcpy(card->longname, "ATX040 I2S Module");

	atx040_i2s_new_pcm(priv);

	ret = snd_card_register(card);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register soundcard\n");
		snd_card_free(card);
		return ret;
	}

	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	priv->base = (void*)r_mem->start;
	priv->irq  = r_irq->start;
	ret = request_irq(priv->irq, atx040_i2s_irq_handler, IRQF_SHARED, DRIVER_NAME, priv);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request interrupt\n");
		return ret;
	}

	platform_set_drvdata(pdev, card);

	return 0;
}

static int atx040_i2s_remove(struct platform_device *pdev)
{
	snd_card_free(platform_get_drvdata(pdev));	

	return 0;
}

static struct platform_driver atx040_i2s_driver = {
	.probe = atx040_i2s_probe,
	.remove = atx040_i2s_remove,
	.driver = {
		.name = DRIVER_NAME,
	},
};

module_platform_driver(atx040_i2s_driver);

MODULE_LICENSE("GPL");
