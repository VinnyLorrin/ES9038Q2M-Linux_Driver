/*
 * Driver for hifibunny3-codec
 *
 * Author: Satoru Kawase
 *      Copyright 2018
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include "hifibunny3-codec.h"
#include <linux/timer.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <asm/div64.h>

static int hifibunny3_codec_dac_mute(struct snd_soc_dai *dai, int mute);
/* hifibunny Q2M Codec Private Data */
struct hifibunny3_codec_priv {
	struct regmap *regmap;
	unsigned int fmt;
};
/* hifibunny Q2M Default Register Value */
static const struct reg_default hifibunny3_codec_reg_defaults[] = {
	{ES9038Q2M_INPUT_CONFIG,0xC0},
	{ES9038Q2M_DEEMP_DOP,0x48},
	{ES9038Q2M_GPIO_CONFIG,0xFF},
	{ES9038Q2M_MASTER_MODE,0x80},
	{ES9038Q2M_SOFT_START,0x0C},
	//Disable ASRC
	{ES9038Q2M_GENERAL_CONFIG_0,0x54},
	//Disable amp supply
	{ES9038Q2M_GENERAL_CONFIG_1,0x40},
};


static bool hifibunny3_codec_writeable(struct device *dev, unsigned int reg)
{
	if(reg == 9 || reg == 26 || reg == 28 || reg == 32 || reg >= 53)
		return false;
	else
		return true;
}

static bool hifibunny3_codec_readable(struct device *dev, unsigned int reg)
{
	if(reg <= 102)
		return true;
	else
		return false;
}

static bool hifibunny3_codec_volatile(struct device *dev, unsigned int reg)
{
	return true;
}


/* Volume Scale */
static const DECLARE_TLV_DB_SCALE(volume_tlv, -12750, 50, 1);

/* Filter Type */
static const char * const fir_filter_type_texts[] = {
	"Linear Phase Fast Roll-off Filter",
	"Linear Phase Slow Roll-off Filter",
	"Minimum Phase Fast Roll-off Filter",
	"Minimum Phase Slow Roll-off Filter",
	"Apodizing Fast Roll-off Filter",
	"Corrected Minimum Phase Fast Roll-off Filter",
	"Brick Wall Filter",
};
static const unsigned int fir_filter_type_values[] = {
	0,
	1,
	2,
	3,
	4,
	6,
	7,
};
static SOC_VALUE_ENUM_SINGLE_DECL(hifibunny3_fir_filter_type_enum,
				  ES9038Q2M_FILTER, 5, 0x07,
				  fir_filter_type_texts,
				  fir_filter_type_values);


/* Control */
static const struct snd_kcontrol_new hifibunny3_codec_controls[] = {
SOC_DOUBLE_R_TLV("Master Playback Volume", ES9038Q2M_VOLUME1, ES9038Q2M_VOLUME2,
		 0, 255, 1, volume_tlv),
SOC_DOUBLE_R_TLV("Digital Playback Volume", ES9038Q2M_VOLUME1, ES9038Q2M_VOLUME2,
		 0, 255, 1, volume_tlv),
SOC_ENUM("DSP Program Route", hifibunny3_fir_filter_type_enum),
SOC_SINGLE("DoP Playback Switch", ES9038Q2M_DEEMP_DOP, 3, 1, 0)
};


static const u32 hifibunny3_codec_dai_rates_master[] = {
	8000, 11025, 16000, 22050, 32000,
	44100, 48000, 64000, 88200, 96000, 
	176400, 192000, 352800, 384000, 705600, 768000
};

static const struct snd_pcm_hw_constraint_list constraints_master = {
	.list  = hifibunny3_codec_dai_rates_master,
	.count = ARRAY_SIZE(hifibunny3_codec_dai_rates_master),
};

static int hifibunny3_codec_dai_startup_master(
		struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	int ret;

	ret = snd_pcm_hw_constraint_list(substream->runtime,
			0, SNDRV_PCM_HW_PARAM_RATE, &constraints_master);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to setup rates constraints: %d\n", ret);
	}

	return ret;
}

static int hifibunny3_codec_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	hifibunny3_codec_dac_mute(dai, 1);
	return hifibunny3_codec_dai_startup_master(substream, dai);
}
static int hifibunny3_codec_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params,struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	uint8_t iface = snd_soc_read(codec, ES9038Q2M_INPUT_CONFIG) & 0x3f;
	//Set input bit depth
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x0;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			iface |= 0xC0;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0xC0;
			break;
		default:
			return -EINVAL;
	}
	snd_soc_write(codec, ES9038Q2M_INPUT_CONFIG, iface);
	//Set NCO divier
	switch(params_rate(params))
	{
		case 8000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0xAB);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0xAA);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x0A);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 11025:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x33);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0xB3);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x0E);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 16000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x55);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x55);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x15);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 22050:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x66);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x66);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x1D);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 32000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0xAB);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0xAA);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x2A);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 44100:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0xCD);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0xCC);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x3A);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 48000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x40);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 88200:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x9A);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x99);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x75);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 96000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x80);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 176400:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x33);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x33);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0xEB);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
			break;
		case 192000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x01);
			break;
		case 352800:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x66);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x66);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0xD6);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x01);
			break;
		case 384000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x02);
			break;
		case 705600:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0xCD);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0xCC);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0xAC);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x03);
			break;
		case 768000:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x04);
			break;
		default:
			snd_soc_write(codec, ES9038Q2M_NCO_0, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_1, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_2, 0x00);
			snd_soc_write(codec, ES9038Q2M_NCO_3, 0x00);
	}
	return 0;
}

static int hifibunny3_codec_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec      *codec = dai->codec;
	struct hifibunny3_codec_priv *hifibunny3_codec
					= snd_soc_codec_get_drvdata(codec);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;

	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	default:
		return (-EINVAL);
	}

	/* clock inversion */
	if ((fmt & SND_SOC_DAIFMT_INV_MASK) != SND_SOC_DAIFMT_NB_NF) {
		return (-EINVAL);
	}

	/* Set Audio Data Format */
	hifibunny3_codec->fmt = fmt;

	return 0;
}

static int hifibunny3_codec_dac_mute(struct snd_soc_dai *dai, int mute)
{
	uint8_t genSet = snd_soc_read(dai->codec, ES9038Q2M_FILTER);
	if(mute)
	{
		snd_soc_write(dai->codec, ES9038Q2M_FILTER, genSet | 0x01);
	}
	return 0;
}
static int hifibunny3_codec_dac_unmute(struct snd_soc_dai *dai)
{
	uint8_t genSet = snd_soc_read(dai->codec, ES9038Q2M_FILTER);
	snd_soc_write(dai->codec, ES9038Q2M_FILTER, genSet & 0xFE);
	return 0;
}
static void hifibunny3_codec_dai_shutdown(struct snd_pcm_substream * substream, struct snd_soc_dai *dai)
{
	hifibunny3_codec_dac_mute(dai, 1);
}

static int hifibunny3_codec_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	hifibunny3_codec_dac_unmute(dai);
	return 0;
}

static int hifibunny3_codec_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	switch(cmd)
	{
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			hifibunny3_codec_dac_mute(dai, 1);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}
static const struct snd_soc_dai_ops hifibunny3_codec_dai_ops = {
	.startup      = hifibunny3_codec_dai_startup,
	.hw_params    = hifibunny3_codec_hw_params,
	.set_fmt      = hifibunny3_codec_set_fmt,
	.digital_mute = hifibunny3_codec_dac_mute,
	.shutdown= hifibunny3_codec_dai_shutdown,
	.prepare = hifibunny3_codec_dai_prepare,
	.trigger = hifibunny3_codec_dai_trigger,
};

static struct snd_soc_dai_driver hifibunny3_codec_dai = {
	.name = "hifibunny3-codec-dai",
	.playback = {
		.stream_name  = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min = 8000,
		.rate_max = 768000,
		.formats      = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | \
		    SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &hifibunny3_codec_dai_ops,
};

static const struct snd_soc_dapm_widget hifibunny3_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DACL", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DACR", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_OUTPUT("OUTL"),
	SND_SOC_DAPM_OUTPUT("OUTR"),
};

static const struct snd_soc_dapm_route hifibunny3_dapm_routes[] = {
	{ "DACL", NULL, "Playback" },
	{ "DACR", NULL, "Playback" },
	{ "OUTL", NULL, "DACL" },
	{ "OUTR", NULL, "DACR" },
};
static int es9038q2m_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level)
{
	mdelay(100);//Delay opamp operation, allow power amp to operate first
	switch (level)
	{
		case SND_SOC_BIAS_OFF:
			snd_soc_write(codec, ES9038Q2M_AUTO_CAL,0x04); //Bias low, turn off opamp
			snd_soc_write(codec, ES9038Q2M_GPIO_INV, 0xC0);//GPIO low, turn off pwr
			snd_soc_write(codec, ES9038Q2M_SOFT_START, 0x0C); // ramp DAC output to gnd
			printk("DAC bias level -> OFF!");
			break;
		case SND_SOC_BIAS_STANDBY:
			snd_soc_write(codec, ES9038Q2M_AUTO_CAL,0x04); //Bias low, turn off opamp
			mdelay(50);
			snd_soc_write(codec, ES9038Q2M_GPIO_INV, 0x00);//GPIO high, turn on pwr
			mdelay(50);
			snd_soc_write(codec, ES9038Q2M_SOFT_START, 0x8C); // ramp DAC output to 0.5*AVCC
			printk("DAC bias level -> STANDBY!");
			break;
		case SND_SOC_BIAS_PREPARE:
			snd_soc_write(codec, ES9038Q2M_AUTO_CAL,0x05); //Bias high, turn on opamp
			mdelay(50);
			snd_soc_write(codec, ES9038Q2M_GPIO_INV, 0x00);//GPIO high, turn on pwr
			mdelay(50);
			snd_soc_write(codec, ES9038Q2M_SOFT_START, 0x8C); // ramp DAC output to 0.5*AVCC
			printk("DAC bias level -> PREPARE!");
			break;
		case SND_SOC_BIAS_ON:
			snd_soc_write(codec, ES9038Q2M_AUTO_CAL,0x05); //Bias high, turn on opamp
			snd_soc_write(codec, ES9038Q2M_GPIO_INV, 0x00);//GPIO high, turn on pwr
			snd_soc_write(codec, ES9038Q2M_SOFT_START, 0x8C); // ramp DAC output to 0.5*AVCC
			printk("DAC bias level -> ON!");
			break;
	}
	return 0;
}
static struct snd_soc_codec_driver hifibunny3_codec_codec_driver = {
	.set_bias_level = es9038q2m_set_bias_level,
	.idle_bias_off = true,
	.component_driver = {
		.controls         = hifibunny3_codec_controls,
		.num_controls     = ARRAY_SIZE(hifibunny3_codec_controls),
		.dapm_widgets	  = hifibunny3_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(hifibunny3_dapm_widgets),
		.dapm_routes      = hifibunny3_dapm_routes,
		.num_dapm_routes  = ARRAY_SIZE(hifibunny3_dapm_routes),
	}
};


static const struct regmap_config hifibunny3_codec_regmap = {
	.reg_bits         = 8,
	.val_bits         = 8,
	.max_register     = 102,

	.reg_defaults     = hifibunny3_codec_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(hifibunny3_codec_reg_defaults),

	.writeable_reg    = hifibunny3_codec_writeable,
	.readable_reg     = hifibunny3_codec_readable,
	.volatile_reg     = hifibunny3_codec_volatile,

	.cache_type       = REGCACHE_RBTREE,
};


static int hifibunny3_codec_probe(struct device *dev, struct regmap *regmap)
{
	struct hifibunny3_codec_priv *hifibunny3_codec;
	int ret = 0;
	hifibunny3_codec = devm_kzalloc(dev, sizeof(*hifibunny3_codec), GFP_KERNEL);
	if (!hifibunny3_codec) {
		dev_err(dev, "devm_kzalloc");
		return (-ENOMEM);
	}
	printk("Registering hifibunny3-codec \n");
	hifibunny3_codec->regmap = regmap;
	regmap_write(regmap, ES9038Q2M_INPUT_CONFIG,0xC0);
	regmap_write(regmap, ES9038Q2M_DEEMP_DOP,0x48);
	regmap_write(regmap, ES9038Q2M_GPIO_CONFIG,0xFF);
	regmap_write(regmap, ES9038Q2M_MASTER_MODE,0xA0);
	regmap_write(regmap, ES9038Q2M_SOFT_START,0x0C);
	regmap_write(regmap, ES9038Q2M_GENERAL_CONFIG_0,0x54);
	regmap_write(regmap, ES9038Q2M_GENERAL_CONFIG_1,0x40);
	msleep(10);
	dev_set_drvdata(dev, hifibunny3_codec);
	ret = snd_soc_register_codec(dev,
			&hifibunny3_codec_codec_driver, &hifibunny3_codec_dai, 1);
	if (ret != 0) {
		dev_err(dev, "Failed to register CODEC: %d\n", ret);
		return ret;
	}
	return ret;
}

static void hifibunny3_codec_remove(struct device *dev)
{
	snd_soc_unregister_codec(dev);
}


static int hifibunny3_codec_i2c_probe(
		struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(i2c, &hifibunny3_codec_regmap);
	if (IS_ERR(regmap)) {
		return PTR_ERR(regmap);
	}

	return hifibunny3_codec_probe(&i2c->dev, regmap);
}

static int hifibunny3_codec_i2c_remove(struct i2c_client *i2c)
{
	hifibunny3_codec_remove(&i2c->dev);

	return 0;
}


static const struct i2c_device_id hifibunny3_codec_i2c_id[] = {
	{ "hifibunny3-codec", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, hifibunny3_codec_i2c_id);

static const struct of_device_id hifibunny3_codec_of_match[] = {
	{ .compatible = "tuxiong,hifibunny3-codec", },
	{ }
};
MODULE_DEVICE_TABLE(of, hifibunny3_codec_of_match);

static struct i2c_driver hifibunny3_codec_i2c_driver = {
	.driver = {
		.name           = "hifibunny3-codec-i2c",
		.owner          = THIS_MODULE,
		.of_match_table = of_match_ptr(hifibunny3_codec_of_match),
	},
	.probe    = hifibunny3_codec_i2c_probe,
	.remove   = hifibunny3_codec_i2c_remove,
	.id_table = hifibunny3_codec_i2c_id,
};
module_i2c_driver(hifibunny3_codec_i2c_driver);


MODULE_DESCRIPTION("ASoC Hifibunny3 Q2M codec driver");
MODULE_LICENSE("GPL");
