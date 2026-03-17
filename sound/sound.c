#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uio.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>

#define CLASS_NAME "dummy_sound_class"
#define DRV_NAME "dummy_sound"

#define DUMMY_SOUND_BUFFER_SIZE (48 * 1024)

struct dummy_sound {
	dev_t dev_num;

	struct device *dev;
	struct class *class;

	struct snd_card *card;
	struct snd_pcm *pcm;

	unsigned int sample_rate;
	unsigned int channels;
	unsigned int sample_bits;

	void *playback_buffer;
	void *capture_buffer;
};

static struct dummy_sound dummy_sound;

static struct snd_pcm_hardware dummy_playback_hw = {
	.info = SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_INTERLEAVED,
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
	.rate_min = 44100,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 2,
	.buffer_bytes_max = DUMMY_SOUND_BUFFER_SIZE,
	.period_bytes_min = 4096,
	.period_bytes_max = 4096,
	.periods_min = 12,
	.periods_max = 12,
};

static struct snd_pcm_hardware dummy_capture_hw = {
	.info = SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_INTERLEAVED,
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
	.rate_min = 44100,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 2,
	.buffer_bytes_max = DUMMY_SOUND_BUFFER_SIZE,
	.period_bytes_min = 4096,
	.period_bytes_max = 4096,
	.periods_min = 12,
	.periods_max = 12,
};

static int dummy_pcm_open(struct snd_pcm_substream *substream)
{
	struct dummy_sound *sound = snd_pcm_substream_chip(substream);

	dev_info(sound->dev, "%s: direction=%d\n", __func__, substream->stream);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	else
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	snd_pcm_hw_constraint_integer(substream->runtime,
				      SNDRV_PCM_HW_PARAM_PERIODS);

	return 0;
}

static int dummy_pcm_close(struct snd_pcm_substream *substream)
{
	struct dummy_sound *sound = snd_pcm_substream_chip(substream);

	dev_info(sound->dev, "%s\n", __func__);
	return 0;
}

static int dummy_pcm_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params)
{
	struct dummy_sound *sound = snd_pcm_substream_chip(substream);

	dev_info(sound->dev, "%s: rate=%d, channels=%d, format=%d\n", __func__,
		 params_rate(params), params_channels(params),
		 params_format(params));

	sound->sample_rate = params_rate(params);
	sound->channels = params_channels(params);
	sound->sample_bits =
		snd_pcm_format_physical_width(params_format(params));

	return 0;
}

static int dummy_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct dummy_sound *sound = snd_pcm_substream_chip(substream);

	dev_info(sound->dev, "%s\n", __func__);
	return 0;
}

static snd_pcm_uframes_t dummy_pcm_pointer(struct snd_pcm_substream *substream)
{
	return 0;
}

static int dummy_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct dummy_sound *sound = snd_pcm_substream_chip(substream);

	dev_info(sound->dev, "%s: cmd=%d\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_STOP:
		return 0;
	default:
		return -EINVAL;
	}
}

static int dummy_pcm_copy(struct snd_pcm_substream *substream, int channel,
			  snd_pcm_uframes_t pos, struct iov_iter *iter,
			  snd_pcm_uframes_t count)
{
	struct dummy_sound *sound = snd_pcm_substream_chip(substream);
	size_t frame_size = snd_pcm_format_width(substream->runtime->format) *
			    substream->runtime->channels / 8;
	size_t bytes = count * frame_size;
	void *buf;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		buf = (void *)(unsigned long)substream->dma_buffer.addr +
		      pos * frame_size;
		bytes = copy_from_iter(buf, bytes, iter);
		dev_info(sound->dev, "%s: playback copied %zu bytes\n",
			 __func__, bytes);
	} else {
		buf = (void *)(unsigned long)substream->dma_buffer.addr +
		      pos * frame_size;
		bytes = copy_to_iter(buf, bytes, iter);
		dev_info(sound->dev, "%s: capture copied %zu bytes\n", __func__,
			 bytes);
	}

	return 0;
}

static struct snd_pcm_ops dummy_pcm_ops = {
	.open = dummy_pcm_open,
	.close = dummy_pcm_close,
	.hw_params = dummy_pcm_hw_params,
	.prepare = dummy_pcm_prepare,
	.trigger = dummy_pcm_trigger,
	.pointer = dummy_pcm_pointer,
	.copy = dummy_pcm_copy,
};

static int dummy_sound_card_create(struct dummy_sound *sound)
{
	int err;

	err = snd_card_new(sound->dev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1,
			   THIS_MODULE, 0, &sound->card);
	if (err < 0)
		return err;

	strscpy(sound->card->driver, DRV_NAME, sizeof(sound->card->driver));
	strscpy(sound->card->shortname, "Dummy Sound",
		sizeof(sound->card->shortname));
	strscpy(sound->card->longname, "Dummy Sound Card",
		sizeof(sound->card->longname));

	return 0;
}

static int dummy_sound_pcm_create(struct dummy_sound *sound)
{
	int err;

	err = snd_pcm_new(sound->card, DRV_NAME " PCM", 0, 1, 1, &sound->pcm);
	if (err < 0)
		return err;

	sound->pcm->private_data = sound;
	snd_pcm_set_ops(sound->pcm, SNDRV_PCM_STREAM_PLAYBACK, &dummy_pcm_ops);
	snd_pcm_set_ops(sound->pcm, SNDRV_PCM_STREAM_CAPTURE, &dummy_pcm_ops);
	snd_pcm_set_managed_buffer_all(sound->pcm, SNDRV_DMA_TYPE_DEV,
				       sound->dev, DUMMY_SOUND_BUFFER_SIZE,
				       DUMMY_SOUND_BUFFER_SIZE);

	return 0;
}

static int dummy_sound_register(struct dummy_sound *sound)
{
	struct device *dev;
	int err;

	err = alloc_chrdev_region(&sound->dev_num, 0, 1, DRV_NAME);
	if (err) {
		pr_err("failed to allocate device number\n");
		return err;
	}

	sound->class = class_create(CLASS_NAME);
	if (IS_ERR(sound->class)) {
		pr_err("failed to create class\n");
		goto err_free_dev_num;
	}

	sound->dev = device_create(sound->class, NULL, sound->dev_num, NULL,
				   DRV_NAME);
	if (IS_ERR(sound->dev)) {
		pr_err("failed to create device\n");
		goto err_free_class;
	}
	dev = sound->dev;

	dev->coherent_dma_mask = DMA_BIT_MASK(32);
	dev->dma_mask = &dev->coherent_dma_mask;

	dev_set_drvdata(dev, sound);

	err = dummy_sound_card_create(sound);
	if (err < 0) {
		dev_err(dev, "failed to create sound card: %d\n", err);
		goto err_free_dev;
	}

	err = dummy_sound_pcm_create(sound);
	if (err < 0) {
		dev_err(dev, "failed to create PCM: %d\n", err);
		goto err_free_card;
	}

	err = snd_card_register(sound->card);
	if (err < 0) {
		dev_err(dev, "failed to register sound card: %d\n", err);
		goto err_free_card;
	}

	dev_info(dev, "dummy sound card registered - ready");

	return 0;

err_free_card:
	snd_card_free(sound->card);
err_free_dev:
	device_destroy(sound->class, sound->dev_num);
err_free_class:
	class_destroy(sound->class);
err_free_dev_num:
	unregister_chrdev_region(sound->dev_num, 1);

	return -ENODEV;
}

static int dummy_sound_unregister(struct dummy_sound *sound)
{
	snd_card_free(sound->card);
	device_destroy(sound->class, sound->dev_num);
	class_destroy(sound->class);
	unregister_chrdev_region(sound->dev_num, 1);

	return 0;
}

module_driver(dummy_sound, dummy_sound_register, dummy_sound_unregister);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy Sound Card driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("dummy-sound");
