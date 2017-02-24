/*
 * ALSA SoC CX20721/CX20723 codec driver
 *
 * Copyright:	(C) 2017 Conexant Systems, Inc.
 * Author:	Simon Ho, <Simon.ho@conexant.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *************************************************************************
 *  Modified Date:  21/2/17
 *  File Version:   4.4.50
 ************************************************************************/
#define DEBUG
/*#define INTEL_MCLK_CONTROL*/
/*#define ENABLE_MIC_POP_WA*/
#define CXDBG_REG_DUMP

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/acpi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/jack.h>
#include <linux/version.h>
#include "cx2072x.h"


#define PLL_OUT_HZ_48 (1024*3*48000)
#define CX2072X_REV_A2 0x00100002
#define BITS_PER_SLOT 8

#define CX2072X_PLBK_EQ_BAND_NUM 7
#define CX2072X_PLBK_EQ_COEF_LEN 11
#define CX2072X_PLBK_DRC_PARM_LEN 9
#define CX2072X_CLASSD_AMP_LEN 6


#ifdef CXDBG_REG_DUMP
#define _REG(_name_, _size_, _access_, _volatile_) { \
	 #_name_, _name_, _size_ | _access_ | _volatile_}

struct CX2072X_REG_DEF {
	const char  *name;
	unsigned int addr;
	unsigned int attr;
};

#define WO 0x0100
#define RO 0x0200
#define RW 0x0300
#define VO 0x8000
#define NV 0x0000
#define REGISTER_SIZE_MASK 0x000F
#define REGISTER_ASSCESS_MASK 0x0F00
#define REGISTER_VOLATILE_MASK 0x8000
#define UNAVAILABLE 0


static const struct CX2072X_REG_DEF cx2072x_regs[] = {
	_REG(CX2072X_VENDOR_ID,			     4, RO, VO),
	_REG(CX2072X_REVISION_ID,		     4, RO, VO),
	_REG(CX2072X_CURRENT_BCLK_FREQUENCY,	     4, RO, VO),
	_REG(CX2072X_AFG_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_UM_RESPONSE,		     1, RW, NV),
	_REG(CX2072X_GPIO_DATA,			     1, RW, NV),
	_REG(CX2072X_GPIO_ENABLE,		     1, RW, NV),
	_REG(CX2072X_GPIO_DIRECTION,		     1, RW, NV),
	_REG(CX2072X_GPIO_WAKE,			     1, RW, NV),
	_REG(CX2072X_GPIO_UM_ENABLE,		     1, RW, NV),
	_REG(CX2072X_GPIO_STICKY_MASK,		     1, RW, NV),
	_REG(CX2072X_AFG_FUNCTION_RESET,	     1, WO, NV),
	_REG(CX2072X_DAC1_CONVERTER_FORMAT,	     2, RW, NV),
	_REG(CX2072X_DAC1_AMP_GAIN_RIGHT,	     1, RW, NV),
	_REG(CX2072X_DAC1_AMP_GAIN_LEFT,	     1, RW, NV),
	_REG(CX2072X_DAC1_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_DAC1_CONVERTER_STREAM_CHANNEL,  1, RW, NV),
	_REG(CX2072X_DAC1_EAPD_ENABLE,		     1, RW, NV),
	_REG(CX2072X_DAC2_CONVERTER_FORMAT,	     2, RW, NV),
	_REG(CX2072X_DAC2_AMP_GAIN_RIGHT,	     1, RW, NV),
	_REG(CX2072X_DAC2_AMP_GAIN_LEFT,	     1, RW, NV),
	_REG(CX2072X_DAC2_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_DAC2_CONVERTER_STREAM_CHANNEL,  1, RW, NV),
	_REG(CX2072X_ADC1_CONVERTER_FORMAT,	     2, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_RIGHT_0,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_LEFT_0,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_RIGHT_1,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_LEFT_1,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_RIGHT_2,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_LEFT_2,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_RIGHT_3,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_LEFT_3,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_RIGHT_4,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_LEFT_4,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_RIGHT_5,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_LEFT_5,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_RIGHT_6,	     1, RW, NV),
	_REG(CX2072X_ADC1_AMP_GAIN_LEFT_6,	     1, RW, NV),
	_REG(CX2072X_ADC1_CONNECTION_SELECT_CONTROL, 1, RW, NV),
	_REG(CX2072X_ADC1_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_ADC1_CONVERTER_STREAM_CHANNEL,  1, RW, NV),
	_REG(CX2072X_ADC2_CONVERTER_FORMAT,	     2, WO, NV),
	_REG(CX2072X_ADC2_AMP_GAIN_RIGHT_0,	     1, RW, NV),
	_REG(CX2072X_ADC2_AMP_GAIN_LEFT_0,	     1, RW, NV),
	_REG(CX2072X_ADC2_AMP_GAIN_RIGHT_1,	     1, RW, NV),
	_REG(CX2072X_ADC2_AMP_GAIN_LEFT_1,	     1, RW, NV),
	_REG(CX2072X_ADC2_AMP_GAIN_RIGHT_2,	     1, RW, NV),
	_REG(CX2072X_ADC2_AMP_GAIN_LEFT_2,	     1, RW, NV),
	_REG(CX2072X_ADC2_CONNECTION_SELECT_CONTROL, 1, RW, NV),
	_REG(CX2072X_ADC2_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_ADC2_CONVERTER_STREAM_CHANNEL,  1, RW, NV),
	_REG(CX2072X_PORTA_CONNECTION_SELECT_CTRL,   1, RW, NV),
	_REG(CX2072X_PORTA_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTA_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTA_UNSOLICITED_RESPONSE,     1, RW, NV),
	_REG(CX2072X_PORTA_PIN_SENSE,		     4, RO, VO),
	_REG(CX2072X_PORTA_EAPD_BTL,		     1, RW, NV),
	_REG(CX2072X_PORTB_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTB_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTB_UNSOLICITED_RESPONSE,     1, RW, NV),
	_REG(CX2072X_PORTB_PIN_SENSE,		     4, RO, VO),
	_REG(CX2072X_PORTB_EAPD_BTL,		     1, RW, NV),
	_REG(CX2072X_PORTB_GAIN_RIGHT,		     1, RW, NV),
	_REG(CX2072X_PORTB_GAIN_LEFT,		     1, RW, NV),
	_REG(CX2072X_PORTC_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTC_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTC_GAIN_RIGHT,		     1, RW, NV),
	_REG(CX2072X_PORTC_GAIN_LEFT,		     1, RW, NV),
	_REG(CX2072X_PORTD_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTD_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTD_UNSOLICITED_RESPONSE,     1, RW, NV),
	_REG(CX2072X_PORTD_PIN_SENSE,		     4, RO, VO),
	_REG(CX2072X_PORTD_GAIN_RIGHT,		     1, RW, NV),
	_REG(CX2072X_PORTD_GAIN_LEFT,		     1, RW, NV),
	_REG(CX2072X_PORTE_CONNECTION_SELECT_CTRL,   1, RW, NV),
	_REG(CX2072X_PORTE_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTE_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTE_UNSOLICITED_RESPONSE,     1, RW, NV),
	_REG(CX2072X_PORTE_PIN_SENSE,		     4, RO, VO),
	_REG(CX2072X_PORTE_EAPD_BTL,		     1, RW, NV),
	_REG(CX2072X_PORTE_GAIN_RIGHT,		     1, RW, NV),
	_REG(CX2072X_PORTE_GAIN_LEFT,		     1, RW, NV),
	_REG(CX2072X_PORTF_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTF_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTF_UNSOLICITED_RESPONSE,     1, RW, NV),
	_REG(CX2072X_PORTF_PIN_SENSE,		     4, RO, VO),
	_REG(CX2072X_PORTF_GAIN_RIGHT,		     1, RW, NV),
	_REG(CX2072X_PORTF_GAIN_LEFT,		     1, RW, NV),
	_REG(CX2072X_PORTG_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTG_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTG_CONNECTION_SELECT_CTRL,   1, RW, NV),
	_REG(CX2072X_PORTG_EAPD_BTL,		     1, RW, NV),
	_REG(CX2072X_PORTM_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_PORTM_PIN_CTRL,		     1, RW, NV),
	_REG(CX2072X_PORTM_CONNECTION_SELECT_CTRL,   1, RW, NV),
	_REG(CX2072X_PORTM_EAPD_BTL,		     1, RW, NV),
	_REG(CX2072X_MIXER_POWER_STATE,		     1, RW, NV),
	_REG(CX2072X_MIXER_GAIN_RIGHT_0,	     1, RW, NV),
	_REG(CX2072X_MIXER_GAIN_LEFT_0,		     1, WO, NV),
	_REG(CX2072X_MIXER_GAIN_RIGHT_1,	     1, RW, NV),
	_REG(CX2072X_MIXER_GAIN_LEFT_1,		     1, RW, NV),
	_REG(CX2072X_EQ_ENABLE_BYPASS,		     2, RW, NV),
	_REG(CX2072X_EQ_B0_COEFF,			2, WO, VO),
	_REG(CX2072X_EQ_B1_COEFF,			2, WO, VO),
	_REG(CX2072X_EQ_B2_COEFF,			2, WO, VO),
	_REG(CX2072X_EQ_A1_COEFF,			2, WO, VO),
	_REG(CX2072X_EQ_A2_COEFF,			2, WO, VO),
	_REG(CX2072X_EQ_G_COEFF,			1, WO, VO),
	_REG(CX2072X_EQ_BAND,				1, WO, VO),
	_REG(CX2072X_SPKR_DRC_ENABLE_STEP,		1, RW, NV),
	_REG(CX2072X_SPKR_DRC_CONTROL,			4, RW, NV),
	_REG(CX2072X_SPKR_DRC_TEST,			4, RW, NV),
	_REG(CX2072X_DIGITAL_BIOS_TEST0,		4, RW, NV),
	_REG(CX2072X_DIGITAL_BIOS_TEST2,		4, RW, NV),
	_REG(CX2072X_I2SPCM_CONTROL1,		     4, RW, NV),
	_REG(CX2072X_I2SPCM_CONTROL2,		     4, RW, NV),
	_REG(CX2072X_I2SPCM_CONTROL3,		     4, RW, NV),
	_REG(CX2072X_I2SPCM_CONTROL4,		     4, RW, NV),
	_REG(CX2072X_I2SPCM_CONTROL5,		     4, RW, NV),
	_REG(CX2072X_I2SPCM_CONTROL6,		     4, RW, NV),
	_REG(CX2072X_UM_INTERRUPT_CRTL_E,	     4, RW, NV),
	_REG(CX2072X_CODEC_TEST20,		     2, RW, NV),
	_REG(CX2072X_CODEC_TEST26,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST4,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST5,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST6,		     2, WO, NV),
	_REG(CX2072X_ANALOG_TEST7,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST8,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST9,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST10,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST11,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST12,		     2, RW, NV),
	_REG(CX2072X_ANALOG_TEST13,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST0,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST1,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST11,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST12,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST15,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST16,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST17,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST18,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST19,		     2, RW, NV),
	_REG(CX2072X_DIGITAL_TEST20,		     2, RW, NV),
};
#endif


/* codec private data */
struct cx2072x_priv {
	struct regmap *regmap;
	struct clk *mclk;
	unsigned int mclk_rate;
	struct device *dev;
	struct snd_soc_codec *codec;
	struct snd_soc_dai_driver *dai_drv;
	int is_biason;
	struct snd_soc_jack *jack;
	bool jack_detecting;
	bool jack_mic;
	int jack_mode;
	int jack_flips;
	unsigned int jack_state;
	int audsmt_enable;
	struct mutex lock;
	unsigned int bclk_ratio;

#ifdef ENABLE_MIC_POP_WA
	struct delayed_work mic_pop_workq;
#endif

	bool plbk_eq_en;
	bool plbk_drc_en;
	bool plbk_dsp_changed;
	bool plbk_dsp_init;
	int plbk_eq_channel;
	int plbk_eq_band;
	bool plbk_eq_update;
	bool plbk_drc_update;
	bool pll_changed;
	bool i2spcm_changed;
	int sample_size;
	int frame_size;
	int sample_rate;
	unsigned int dai_fmt;
	int tdm_rx_mask;
	int tdm_tx_mask;
	int tdm_slot_width;
	int tdm_slots;
	u32 rev_id;
	bool en_aec_ref;
	u8 plbk_eq[2][CX2072X_PLBK_EQ_BAND_NUM][CX2072X_PLBK_EQ_COEF_LEN];
	u8 plbk_drc[CX2072X_PLBK_DRC_PARM_LEN];
	u8 classd_amp[CX2072X_CLASSD_AMP_LEN];
	struct mutex eq_coeff_lock;
};

/*
 * DAC/ADC Volume
 *
 * max : 74 : 0 dB
 *	 ( in 1 dB  step )
 * min : 0 : -74 dB
 */
static const DECLARE_TLV_DB_SCALE(adc_tlv, -7400, 100, 0);
static const DECLARE_TLV_DB_SCALE(dac_tlv, -7400, 100, 0);
static const DECLARE_TLV_DB_SCALE(boost_tlv, 0, 1200, 0);

struct CX2072X_EQ_CTRL {
	int ch;
	int band;
};


static DECLARE_TLV_DB_RANGE(hpf_tlv,
		0, 0, TLV_DB_SCALE_ITEM(120, 0, 0),
		1, 63, TLV_DB_SCALE_ITEM(30, 30, 0)
);

#define get_cx2072x_priv(_codec_) \
	((struct cx2072x_priv *) snd_soc_codec_get_drvdata(_codec_))

/* Lookup table for PRE_DIV*/
static struct {
	unsigned int mclk;
	unsigned int div;
} MCLK_PRE_DIV[] = {
	{ 6144000, 1 },
	{ 12288000, 2 },
	{ 19200000, 3 },
	{ 26000000, 4 },
	{ 28224000, 5 },
	{ 36864000, 6 },
	{ 36864000, 7 },
	{ 48000000, 8 },
	{ 49152000, 8 },
};

/*
 * cx2072x register cache.
 */
static const struct reg_default cx2072x_reg_defaults[] = {
	{ 0x0414, 0x00000003 },	/*2072X_AFG_POWER_STATE */
	{ 0x0420, 0x00000000 },	/*2072X_UM_RESPONSE */
	{ 0x0454, 0x00000000 },	/*2072X_GPIO_DATA */
	{ 0x0458, 0x00000000 },	/*2072X_GPIO_ENABLE */
	{ 0x045c, 0x00000000 },	/*2072X_GPIO_DIRECTION */
	{ 0x0460, 0x00000000 },	/*2072X_GPIO_WAKE */
	{ 0x0464, 0x00000000 },	/*2072X_GPIO_UM_ENABLE */
	{ 0x0468, 0x00000000 },	/*2072X_GPIO_STICKY_MASK */
	{ 0x43c8, 0x00000031 },	/*2072X_DAC1_CONVERTER_FORMAT */
	{ 0x41c0, 0x0000004a },	/*2072X_DAC1_AMP_GAIN_RIGHT */
	{ 0x41e0, 0x0000004a },	/*2072X_DAC1_AMP_GAIN_LEFT */
	{ 0x4014, 0x00000433 },	/*2072X_DAC1_POWER_STATE */
	{ 0x4018, 0x00000000 },	/*2072X_DAC1_CONVERTER_STREAM_CHANNEL */
	{ 0x4030, 0x00000000 },	/*2072X_DAC1_EAPD_ENABLE */
	{ 0x47c8, 0x00000031 },	/*2072X_DAC2_CONVERTER_FORMAT */
	{ 0x45c0, 0x0000004a },	/*2072X_DAC2_AMP_GAIN_RIGHT */
	{ 0x45e0, 0x0000004a },	/*2072X_DAC2_AMP_GAIN_LEFT */
	{ 0x4414, 0x00000433 },	/*2072X_DAC2_POWER_STATE */
	{ 0x4418, 0x00000000 },	/*2072X_DAC2_CONVERTER_STREAM_CHANNEL */
	{ 0x4fc8, 0x00000031 },	/*2072X_ADC1_CONVERTER_FORMAT */
	{ 0x4d80, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_RIGHT_0 */
	{ 0x4da0, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_LEFT_0  */
	{ 0x4d84, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_RIGHT_1 */
	{ 0x4da4, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_LEFT_1 */
	{ 0x4d88, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_RIGHT_2  */
	{ 0x4da8, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_LEFT_2  */
	{ 0x4d8c, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_RIGHT_3  */
	{ 0x4dac, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_LEFT_3  */
	{ 0x4d90, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_RIGHT_4  */
	{ 0x4db0, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_LEFT_4  */
	{ 0x4d94, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_RIGHT_5  */
	{ 0x4db4, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_LEFT_5 */
	{ 0x4d98, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_RIGHT_6 */
	{ 0x4db8, 0x0000004a },	/*2072X_ADC1_AMP_GAIN_LEFT_6 */
	{ 0x4c04, 0x00000000 },	/*2072X_ADC1_CONNECTION_SELECT_CONTROL */
	{ 0x4c14, 0x00000433 },	/*2072X_ADC1_POWER_STATE */
	{ 0x4c18, 0x00000000 },	/*2072X_ADC1_CONVERTER_STREAM_CHANNEL */
	{ 0x53c8, 0x00000031 },	/*2072X_ADC2_CONVERTER_FORMAT */
	{ 0x5180, 0x0000004a },	/*2072X_ADC2_AMP_GAIN_RIGHT_0 */
	{ 0x51a0, 0x0000004a },	/*2072X_ADC2_AMP_GAIN_LEFT_0 */
	{ 0x5184, 0x0000004a },	/*2072X_ADC2_AMP_GAIN_RIGHT_1 */
	{ 0x51a4, 0x0000004a },	/*2072X_ADC2_AMP_GAIN_LEFT_1 */
	{ 0x5188, 0x0000004a },	/*2072X_ADC2_AMP_GAIN_RIGHT_2 */
	{ 0x51a8, 0x0000004a },	/*2072X_ADC2_AMP_GAIN_LEFT_2 */
	{ 0x5004, 0x00000000 },	/*2072X_ADC2_CONNECTION_SELECT_CONTROL */
	{ 0x5014, 0x00000433 },	/*2072X_ADC2_POWER_STATE */
	{ 0x5018, 0x00000000 },	/*2072X_ADC2_CONVERTER_STREAM_CHANNEL */
	{ 0x5804, 0x00000000 },	/*2072X_PORTA_CONNECTION_SELECT_CTRL */
	{ 0x5814, 0x00000433 },	/*2072X_PORTA_POWER_STATE */
	{ 0x581c, 0x000000c0 },	/*2072X_PORTA_PIN_CTRL */
	{ 0x5820, 0x00000000 },	/*2072X_PORTA_UNSOLICITED_RESPONSE */
	{ 0x5824, 0x00000000 },	/*2072X_PORTA_PIN_SENSE */
	{ 0x5830, 0x00000002 },	/*2072X_PORTA_EAPD_BTL */
	{ 0x6014, 0x00000433 },	/*2072X_PORTB_POWER_STATE */
	{ 0x601c, 0x00000000 },	/*2072X_PORTB_PIN_CTRL */
	{ 0x6020, 0x00000000 },	/*2072X_PORTB_UNSOLICITED_RESPONSE */
	{ 0x6024, 0x00000000 },	/*2072X_PORTB_PIN_SENSE */
	{ 0x6030, 0x00000002 },	/*2072X_PORTB_EAPD_BTL */
	{ 0x6180, 0x00000000 },	/*2072X_PORTB_GAIN_RIGHT */
	{ 0x61a0, 0x00000000 },	/*2072X_PORTB_GAIN_LEFT */
	{ 0x6814, 0x00000433 },	/*2072X_PORTC_POWER_STATE */
	{ 0x681c, 0x00000000 },	/*2072X_PORTC_PIN_CTRL */
	{ 0x6980, 0x00000000 },	/*2072X_PORTC_GAIN_RIGHT */
	{ 0x69a0, 0x00000000 },	/*2072X_PORTC_GAIN_LEFT */
	{ 0x6414, 0x00000433 },	/*2072X_PORTD_POWER_STATE */
	{ 0x641c, 0x00000020 },	/*2072X_PORTD_PIN_CTRL */
	{ 0x6420, 0x00000000 },	/*2072X_PORTD_UNSOLICITED_RESPONSE */
	{ 0x6424, 0x00000000 },	/*2072X_PORTD_PIN_SENSE */
	{ 0x6580, 0x00000000 },	/*2072X_PORTD_GAIN_RIGHT */
	{ 0x65a0, 0x00000000 },	/*2072X_PORTD_GAIN_LEFT */
	{ 0x7404, 0x00000000 },	/*2072X_PORTE_CONNECTION_SELECT_CTRL */
	{ 0x7414, 0x00000433 },	/*2072X_PORTE_POWER_STATE */
	{ 0x741c, 0x00000040 },	/*2072X_PORTE_PIN_CTRL */
	{ 0x7420, 0x00000000 },	/*2072X_PORTE_UNSOLICITED_RESPONSE */
	{ 0x7424, 0x00000000 },	/*2072X_PORTE_PIN_SENSE */
	{ 0x7430, 0x00000002 },	/*2072X_PORTE_EAPD_BTL */
	{ 0x7580, 0x00000000 },	/*2072X_PORTE_GAIN_RIGHT */
	{ 0x75a0, 0x00000000 },	/*2072X_PORTE_GAIN_LEFT */
	{ 0x7814, 0x00000433 },	/*2072X_PORTF_POWER_STATE */
	{ 0x781c, 0x00000000 },	/*2072X_PORTF_PIN_CTRL */
	{ 0x7820, 0x00000000 },	/*2072X_PORTF_UNSOLICITED_RESPONSE */
	{ 0x7824, 0x00000000 },	/*2072X_PORTF_PIN_SENSE */
	{ 0x7980, 0x00000000 },	/*2072X_PORTF_GAIN_RIGHT */
	{ 0x79a0, 0x00000000 },	/*2072X_PORTF_GAIN_LEFT */
	{ 0x5c14, 0x00000433 },	/*2072X_PORTG_POWER_STATE */
	{ 0x5c1c, 0x00000040 },	/*2072X_PORTG_PIN_CTRL */
	{ 0x5c04, 0x00000000 },	/*2072X_PORTG_CONNECTION_SELECT_CTRL */
	{ 0x5c30, 0x00000002 },	/*2072X_PORTG_EAPD_BTL */
	{ 0x8814, 0x00000433 },	/*2072X_PORTM_POWER_STATE */
	{ 0x881c, 0x00000000 },	/*2072X_PORTM_PIN_CTRL */
	{ 0x8804, 0x00000000 },	/*2072X_PORTM_CONNECTION_SELECT_CTRL */
	{ 0x8830, 0x00000002 },	/*2072X_PORTM_EAPD_BTL */
	{ 0x5414, 0x00000433 },	/*2072X_MIXER_POWER_STATE */
	{ 0x5580, 0x0000004a },	/*2072X_MIXER_GAIN_RIGHT_0 */
	{ 0x55a0, 0x0000004a },	/*2072X_MIXER_GAIN_LEFT_0 */
	{ 0x5584, 0x0000004a },	/*2072X_MIXER_GAIN_RIGHT_1 */
	{ 0x55a4, 0x0000004a },	/*2072X_MIXER_GAIN_LEFT_1 */
	{ 0x6d00, 0x0000720c },	/*2072X_EQ_ENABLE_BYPASS */
	{ 0x6d10, 0x040065a4 },	/*2072X_SPKR_DRC_ENABLE_STEP */
	{ 0x6d14, 0x007b0024 },	/*2072X_SPKR_DRC_CONTROL */
	{ 0X6D18, 0x00000000 },	/*2072X_SPKR_DRC_TEST */
	{ 0x6d80, 0x001f008a },	/*2072X_DIGITAL_BIOS_TEST0 */
	{ 0x6d84, 0x00990026 },	/*2072X_DIGITAL_BIOS_TEST2 */
	{ 0x6e00, 0x00010001 },	/*2072X_I2SPCM_CONTROL1 */
	{ 0x6e04, 0x00000000 },	/*2072X_I2SPCM_CONTROL2 */
	{ 0x6e08, 0x00000000 },	/*2072X_I2SPCM_CONTROL3 */
	{ 0x6e0c, 0x00000000 },	/*2072X_I2SPCM_CONTROL4 */
	{ 0x6e10, 0x00000000 },	/*2072X_I2SPCM_CONTROL5 */
	{ 0x6e18, 0x00000000 },	/*2072X_I2SPCM_CONTROL6 */
	{ 0x6e14, 0x00000000 },	/*2072X_UM_INTERRUPT_CRTL_E */
	{ 0x7108, 0x00000000 },	/*2072X_CODEC_TEST2  */
	{ 0x7124, 0x00000004 },	/*2072X_CODEC_TEST9  */
	{ 0x7310, 0x00000600 },	/*2072X_CODEC_TEST20 */
	{ 0x7328, 0x00000208 },	/*2072X_CODEC_TEST26 */
	{ 0x7190, 0x00000000 },	/*2072X_ANALOG_TEST4 */
	{ 0x7194, 0x00000000 },	/*2072X_ANALOG_TEST5 */
	{ 0x7198, 0x0000059a },	/*2072X_ANALOG_TEST6 */
	{ 0x719c, 0x000000a7 },	/*2072X_ANALOG_TEST7 */
	{ 0x71a0, 0x00000017 },	/*2072X_ANALOG_TEST8 */
	{ 0x71a4, 0x00000000 },	/*2072X_ANALOG_TEST9 */
	{ 0x71a8, 0x00000285 },	/*2072X_ANALOG_TEST10 */
	{ 0x71ac, 0x00000000 },	/*2072X_ANALOG_TEST11 */
	{ 0x71b0, 0x00000000 },	/*2072X_ANALOG_TEST12 */
	{ 0x71b4, 0x00000000 },	/*2072X_ANALOG_TEST13 */
	{ 0x7204, 0x00000242 },	/*2072X_DIGITAL_TEST1 */
	{ 0x7224, 0x00000000 },	/*2072X_DIGITAL_TEST11 */
	{ 0x7230, 0x00000084 },	/*2072X_DIGITAL_TEST12 */
	{ 0x723c, 0x00000077 },	/*2072X_DIGITAL_TEST15 */
	{ 0x7080, 0x00000021 },	/*2072X_DIGITAL_TEST16 */
	{ 0x7084, 0x00000018 },	/*2072X_DIGITAL_TEST17 */
	{ 0x7088, 0x00000024 },	/*2072X_DIGITAL_TEST18 */
	{ 0x708c, 0x00000001 },	/*2072X_DIGITAL_TEST19 */
	{ 0x7090, 0x00000002 },	/*2072X_DIGITAL_TEST20 */
};


/*
 * register patch.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
static const struct reg_sequence cx2072x_patch[] = {
#else
static const struct reg_default cx2072x_patch[] = {
#endif
	{ 0x71A4, 0x080 }, /* DC offset Calibration	   */
	{ 0x71a8, 0x287 }, /* Set max spk power to 1.5 W   */
	{ 0x7328, 0xa8c }, /* Set average spk power to 1.5W*/
	{ 0x7310, 0xf01 }, /*				  */
	{ 0x7328, 0xa8f }, /*				   */
	{ 0x7124, 0x001 }, /* Enable 30 Hz High pass filter*/
	{ 0x718c, 0x300 }, /* Disable PCBEEP pad	   */
	{ 0x731c, 0x100 }, /* Disable SnM mode		   */
	{ 0x641c, 0x020 }, /* Enable PortD input	   */
	{ 0x0458, 0x040 }, /* Enable GPIO7 pin for button  */
	{ 0x0464, 0x040 }, /* Enable UM for GPIO7	   */
	{ 0x0420, 0x080 }, /* Enable button response	   */
	{ 0x7230, 0x0c4 }, /* Enable headset button	   */
	{ 0x7200, 0x415 }, /* Power down class-d during idle*/
};


/* return register size */
static unsigned int cx2072x_register_size(struct device *dev,
	unsigned int reg)
{
	switch (reg) {
	case CX2072X_VENDOR_ID:
	case CX2072X_REVISION_ID:
	case CX2072X_PORTA_PIN_SENSE:
	case CX2072X_PORTB_PIN_SENSE:
	case CX2072X_PORTD_PIN_SENSE:
	case CX2072X_PORTE_PIN_SENSE:
	case CX2072X_PORTF_PIN_SENSE:
	case CX2072X_I2SPCM_CONTROL1:
	case CX2072X_I2SPCM_CONTROL2:
	case CX2072X_I2SPCM_CONTROL3:
	case CX2072X_I2SPCM_CONTROL4:
	case CX2072X_I2SPCM_CONTROL5:
	case CX2072X_I2SPCM_CONTROL6:
	case CX2072X_UM_INTERRUPT_CRTL_E:
	case CX2072X_EQ_G_COEFF:
	case CX2072X_SPKR_DRC_CONTROL:
	case CX2072X_SPKR_DRC_TEST:
	case CX2072X_DIGITAL_BIOS_TEST0:
	case CX2072X_DIGITAL_BIOS_TEST2:
		return 4;
	case CX2072X_EQ_ENABLE_BYPASS:
	case CX2072X_EQ_B0_COEFF:
	case CX2072X_EQ_B1_COEFF:
	case CX2072X_EQ_B2_COEFF:
	case CX2072X_EQ_A1_COEFF:
	case CX2072X_EQ_A2_COEFF:
	case CX2072X_DAC1_CONVERTER_FORMAT:
	case CX2072X_DAC2_CONVERTER_FORMAT:
	case CX2072X_ADC1_CONVERTER_FORMAT:
	case CX2072X_ADC2_CONVERTER_FORMAT:
	case CX2072X_CODEC_TEST2:
	case CX2072X_CODEC_TEST9:
	case CX2072X_CODEC_TEST20:
	case CX2072X_CODEC_TEST26:
	case CX2072X_ANALOG_TEST3:
	case CX2072X_ANALOG_TEST4:
	case CX2072X_ANALOG_TEST5:
	case CX2072X_ANALOG_TEST6:
	case CX2072X_ANALOG_TEST7:
	case CX2072X_ANALOG_TEST8:
	case CX2072X_ANALOG_TEST9:
	case CX2072X_ANALOG_TEST10:
	case CX2072X_ANALOG_TEST11:
	case CX2072X_ANALOG_TEST12:
	case CX2072X_ANALOG_TEST13:
	case CX2072X_DIGITAL_TEST0:
	case CX2072X_DIGITAL_TEST1:
	case CX2072X_DIGITAL_TEST11:
	case CX2072X_DIGITAL_TEST12:
	case CX2072X_DIGITAL_TEST15:
	case CX2072X_DIGITAL_TEST16:
	case CX2072X_DIGITAL_TEST17:
	case CX2072X_DIGITAL_TEST18:
	case CX2072X_DIGITAL_TEST19:
	case CX2072X_DIGITAL_TEST20:
		return 2;
	default:
		return 1;
	}

}

#ifdef CXDBG_REG_DUMP
static const char *cx2072x_get_reg_name(struct device *dev, unsigned int reg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cx2072x_regs); i++)
		if (cx2072x_regs[i].addr == reg)
			return cx2072x_regs[i].name;

	dev_err(dev, "Unkown reg %08x\n", reg);

	return "Unknown reg";
}
#endif

static int cx2072x_reg_write(void *context, unsigned int reg,
			      unsigned int value)
{
	struct i2c_client *client = context;
	unsigned int i;
	unsigned int size;
	uint8_t buf[6];
	int ret;
	struct device *dev = &client->dev;

	size = cx2072x_register_size(dev, reg);
	if (size == 0)
		return -EINVAL;
#ifdef CXDBG_REG_DUMP
	dev_dbg(dev, "I2C write address %40s,%d <= %08x\n",
		cx2072x_get_reg_name(dev, reg), size, value);
#endif

	if (reg == CX2072X_UM_INTERRUPT_CRTL_E) {
		/* Update the MSB byte only */
		reg += 3;
		size = 1;
		value >>= 24;
	}

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	for (i = 2; i < (size + 2); ++i) {
		buf[i] = value;
		value >>= 8;
	}

	ret = i2c_master_send(client, buf, size + 2);
	if (ret == size + 2) {
		ret =  0;
	} else if (ret < 0) {
		dev_err(dev,
			"I2C write address failed\n");
	} else {
		dev_err(dev,
			"I2C write failed\n");
		ret =  -EIO;
	}
	return ret;
}

static int cx2072x_reg_bulk_write(struct snd_soc_codec *codec,
	unsigned int reg, const void *val, size_t val_count)
{
	/*fix me*/
	struct i2c_client *client = to_i2c_client(codec->dev);
	uint8_t buf[2 + MAC_EQ_COEFF];
	int ret;
	struct device *dev = &client->dev;

#ifdef CXDBG_REG_DUMP
	dev_dbg(dev, "I2C bulk write address %40s,%d\n",
		cx2072x_get_reg_name(dev, reg), val_count);
#endif

	if (val_count > MAC_EQ_COEFF) {
		dev_err(dev,
			"cx2072x_reg_bulk_write failed, writing count = %d\n",
			val_count);
		return -EINVAL;
	}

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	memcpy(&buf[2], val, val_count);

	ret = i2c_master_send(client, buf, val_count + 2);
	if (ret == val_count + 2)
		return 0;
	else if (ret < 0) {
		dev_err(dev,
			"I2C bulk write address failed\n");
	} else {
		dev_err(dev,
			"I2C bulk write address failed\n");
		ret = -EIO;
	}
	return ret;
}


/* get suggested pre_div valuce from mclk frequency */
static unsigned int get_div_from_mclk(unsigned int mclk)
{
	unsigned int div = 8;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(MCLK_PRE_DIV); i++) {
		if (mclk <= MCLK_PRE_DIV[i].mclk) {
			div = MCLK_PRE_DIV[i].div;
			break;
		}
	}
	return div;
}

static int cx2072x_reg_read(void *context, unsigned int reg,
	unsigned int *value)
{
	int ret;
	unsigned int size;
	uint8_t send_buf[2];
	unsigned int recv_buf = 0;
	struct i2c_client *client = context;
	struct i2c_msg msgs[2];
	struct device *dev = &client->dev;

	size = cx2072x_register_size(dev, reg);
	if (size == 0)
		return -EINVAL;

	send_buf[0] = reg >> 8;
	send_buf[1] = reg & 0xff;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = size;
	msgs[1].buf = (uint8_t *)&recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(dev,
			"Failed to register codec: %d\n", ret);
		return ret;
	} else if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev,
			"Failed to register codec: %d\n", ret);
		return -EIO;
	}

	*value = recv_buf;

#ifdef CXDBG_REG_DUMP
	dev_dbg(dev,
		"I2C read address %40s,%d => %08x\n",
		cx2072x_get_reg_name(dev, reg), size, *value);
#endif
	return 0;
}

static int cx2072x_config_headset_det(struct cx2072x_priv *cx2072x)
{
	const int interrupt_gpio_pin = 1;

	dev_dbg(cx2072x->dev,
		"Configure interrupt pin: %d\n", interrupt_gpio_pin);
	/*No-sticky input type*/
	regmap_write(cx2072x->regmap, CX2072X_GPIO_STICKY_MASK, 0x1f);

	/*Use GPOI0 as interrupt pin*/
	regmap_write(cx2072x->regmap, CX2072X_UM_INTERRUPT_CRTL_E, 0x12 << 24);

	/* Enables unsolitited message on PortA*/
	regmap_write(cx2072x->regmap, CX2072X_PORTA_UNSOLICITED_RESPONSE, 0x80);

	/* support both nokia and apple headset set. Monitor time = 275 ms*/
	regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST15, 0x73);

	/* Disable TIP detection*/
	regmap_write(cx2072x->regmap, CX2072X_ANALOG_TEST12, 0x300);

	/* Switch MusicD3Live pin to GPIO */
	regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST1, 0);

	return 0;
}

static int cx2072x_config_pll(struct cx2072x_priv *cx2072x)
{
	struct device *dev = cx2072x->dev;
	unsigned int pre_div;
	unsigned int pre_div_val;
	unsigned int pll_input;
	unsigned int pll_output;
	unsigned int int_div;
	unsigned int frac_div;
	u64 frac_num;
	unsigned int frac;
	unsigned int sample_rate = cx2072x->sample_rate;
	int pt_sample_per_sync = 2;
	int pt_clock_per_sample = 96;

	switch (sample_rate) {
	case 48000:
	case 32000:
	case 24000:
	case 16000:
		break;
	case 96000:
		pt_sample_per_sync = 1;
		pt_clock_per_sample = 48;
		break;
	case 192000:
		pt_sample_per_sync = 0;
		pt_clock_per_sample = 24;
		break;
	default:
		dev_err(dev, "Unsupported sample rate %d\n", sample_rate);
		return -EINVAL;
	}


	/*Configure PLL settings*/
	pre_div = get_div_from_mclk(cx2072x->mclk_rate);
	pll_input = cx2072x->mclk_rate / pre_div;
	pll_output = sample_rate * 3072;
	int_div = pll_output / pll_input;
	frac_div = pll_output - (int_div * pll_input);

	if (frac_div) {
		frac_div *= 1000;
		frac_div /= pll_input;
		frac_num = ((4000 + frac_div) * ((1 << 20) - 4));
		do_div(frac_num, 7);
		frac = ((u32)frac_num + 499) / 1000;
	}
	pre_div_val = (pre_div - 1) * 2;

	regmap_write(cx2072x->regmap, CX2072X_ANALOG_TEST4, 0X40
		| (pre_div_val << 8));
	if (frac_div == 0) {
		/*Int mode*/
		regmap_write(cx2072x->regmap, CX2072X_ANALOG_TEST7, 0x100);
	} else {
		/*frac mode*/
		regmap_write(cx2072x->regmap, CX2072X_ANALOG_TEST6,
			frac & 0xfff);
		regmap_write(cx2072x->regmap, CX2072X_ANALOG_TEST7,
			(unsigned char)(frac >> 12));
	}
	int_div--;
	regmap_write(cx2072x->regmap, CX2072X_ANALOG_TEST8,
		(unsigned char)int_div & 0xffff);

	/* configure PLL tracking*/
	if (frac_div == 0) {
		/* disable PLL tracking*/
		regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST16, 0x00);
	} else {
		/* configure and enable PLL tracking*/
		regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST16,
			(pt_sample_per_sync << 4) & 0xf0);
		regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST17,
			pt_clock_per_sample);
		regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST18,
			pt_clock_per_sample * 3 / 2);
		regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST19, 0x01);
		regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST20, 0x02);
		regmap_update_bits(cx2072x->regmap, CX2072X_DIGITAL_TEST16,
			0X01, 0X01);
	}

	return 0;
}


static int cx2072x_config_i2spcm(struct cx2072x_priv *cx2072x)
{
	struct device *dev = cx2072x->dev;
	unsigned int bclk_rate = 0;
	int is_i2s = 0;
	int has_one_bit_delay = 0;
	int is_right_j = 0;
	int is_frame_inv = 0;
	int is_bclk_inv = 0;
	int pulse_len = 1;
	int frame_len = cx2072x->frame_size;
	int sample_size = cx2072x->sample_size;
	int i2s_right_slot;
	int i2s_right_pause_interval = 0;
	int i2s_right_pause_pos;
	int is_big_endian = 1;
	const int slots_per_channel = cx2072x->tdm_slot_width / BITS_PER_SLOT;
	const unsigned int fmt = cx2072x->dai_fmt;
	u64 div;
	unsigned int mod;
	union REG_I2SPCM_CTRL_REG1 reg1;
	union REG_I2SPCM_CTRL_REG2 reg2;
	union REG_I2SPCM_CTRL_REG3 reg3;
	union REG_I2SPCM_CTRL_REG4 reg4;
	union REG_I2SPCM_CTRL_REG5 reg5;
	union REG_I2SPCM_CTRL_REG6 reg6;
	union REG_DIGITAL_BIOS_TEST2 regDBT2;

	if (frame_len <= 0) {
		dev_err(dev, "Incorrect frame len %d\n", frame_len);
		return -EINVAL;
	}

	if (sample_size <= 0) {
		dev_err(dev, "Incorrect sample size %d\n", sample_size);
		return -EINVAL;
	}

	dev_dbg(dev, "config_i2spcm set_dai_fmt- %08x\n", fmt);

	regDBT2.ulVal = 0xac;

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		if (cx2072x->en_aec_ref) {
			reg2.r.tx_master = 0;
			reg3.r.rx_master = 0;
			dev_dbg(dev, "Sets Slave mode for AEC\n");
		} else {
			reg2.r.tx_master = 1;
			reg3.r.rx_master = 1;
			dev_dbg(dev, "Sets Master mode\n");
		}
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		reg2.r.tx_master = 0;
		reg3.r.rx_master = 0;
		dev_dbg(dev, "Sets Slave mode\n");
		break;
	default:
		dev_err(dev, "Unsupported DAI master mode\n");
		return -EINVAL;
	}

	/* set format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		is_i2s = 1;
		has_one_bit_delay = 1;
		pulse_len = frame_len / 2;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		is_i2s = 1;
		is_right_j = 1;
		pulse_len = frame_len / 2;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		is_i2s = 1;
		pulse_len = frame_len / 2;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		has_one_bit_delay = 1;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		dev_err(dev, "Unsupported DAI format\n");
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		is_frame_inv = is_i2s;
		is_bclk_inv = is_i2s;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		is_frame_inv = !is_i2s;
		is_bclk_inv = !is_i2s;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		is_frame_inv = is_i2s;
		is_bclk_inv = !is_i2s;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		is_frame_inv = !is_i2s;
		is_bclk_inv = is_i2s;
		break;
	default:
		dev_err(dev, "Unsupported DAI clock inversion\n");
		return -EINVAL;
	}


	reg1.r.rx_data_one_line = 1;
	reg1.r.tx_data_one_line = 1;

	if (is_i2s) {
		i2s_right_slot = (frame_len / 2) / BITS_PER_SLOT;
		i2s_right_pause_interval = (frame_len / 2) % BITS_PER_SLOT;
		i2s_right_pause_pos = i2s_right_slot * BITS_PER_SLOT;
	}

	reg1.r.rx_ws_pol = is_frame_inv;
	reg1.r.rx_ws_wid = pulse_len - 1;

	reg1.r.rx_frm_len = frame_len / BITS_PER_SLOT - 1;
	reg1.r.rx_sa_size = (sample_size / BITS_PER_SLOT) - 1;

	reg1.r.tx_ws_pol = reg1.r.rx_ws_pol;
	reg1.r.tx_ws_wid = pulse_len - 1;
	reg1.r.tx_frm_len = reg1.r.rx_frm_len;
	reg1.r.tx_sa_size = reg1.r.rx_sa_size;

	reg2.r.tx_endian_sel = is_big_endian ? 0 : 1;
	reg2.r.tx_dstart_dly = has_one_bit_delay;

	reg3.r.rx_endian_sel = is_big_endian ? 0 : 1;
	reg3.r.rx_dstart_dly = has_one_bit_delay;

	if (is_i2s) {
		reg2.r.tx_en_ch1 = 1;
		reg2.r.tx_en_ch2 = 1;
		reg2.r.tx_slot_1 = 0;
		reg2.r.tx_slot_2 = i2s_right_slot;
		reg3.r.rx_en_ch1 = 1;
		reg3.r.rx_en_ch2 = 1;
		reg3.r.rx_slot_1 = 0;
		reg3.r.rx_slot_2 = i2s_right_slot;
		reg6.r.rx_pause_start_pos = i2s_right_pause_pos;
		reg6.r.rx_pause_cycles = i2s_right_pause_interval;
		reg6.r.tx_pause_start_pos = i2s_right_pause_pos;
		reg6.r.tx_pause_cycles = i2s_right_pause_interval;
	} else {
		reg2.r.tx_en_ch1 = cx2072x->tdm_tx_mask & 0x01 ? 1 : 0;
		reg2.r.tx_en_ch2 = cx2072x->tdm_tx_mask & 0x02 ? 1 : 0;
		reg2.r.tx_en_ch3 = cx2072x->tdm_tx_mask & 0x04 ? 1 : 0;
		reg2.r.tx_en_ch4 = cx2072x->tdm_tx_mask & 0x08 ? 1 : 0;
		reg2.r.tx_slot_1 = 0;
		reg2.r.tx_slot_2 = slots_per_channel * 1;
		reg2.r.tx_slot_3 = slots_per_channel * 2;
		reg2.r.tx_slot_4 = slots_per_channel * 3;

		reg3.r.rx_en_ch1 = cx2072x->tdm_rx_mask & 0x01 ? 1 : 0;
		reg3.r.rx_en_ch2 = cx2072x->tdm_rx_mask & 0x02 ? 1 : 0;
		reg3.r.rx_en_ch3 = cx2072x->tdm_rx_mask & 0x04 ? 1 : 0;
		reg3.r.rx_en_ch4 = cx2072x->tdm_rx_mask & 0x08 ? 1 : 0;
		reg3.r.rx_slot_1 = 0;
		reg3.r.rx_slot_2 = slots_per_channel * 1;
		reg3.r.rx_slot_3 = slots_per_channel * 2;
		reg3.r.rx_slot_4 = slots_per_channel * 3;
	}
	regDBT2.r.i2s_bclk_invert = is_bclk_inv;

	reg1.r.rx_data_one_line = 1;
	reg1.r.tx_data_one_line = 1;

#ifdef ENABLE_MIC_POP_WA
	/*Mute I2S TX*/
	reg4.ulVal |= 0x2;
#endif


	/* Configures the BCLK output.*/
	bclk_rate = cx2072x->sample_rate * frame_len;
	reg5.r.i2s_pcm_clk_div_chan_en = 0;

	/*Disables bclk output before setting new value*/
	regmap_write(cx2072x->regmap, CX2072X_I2SPCM_CONTROL5, 0);
	if (reg2.r.tx_master) {
		/*Configures BCLK rate*/
		div = PLL_OUT_HZ_48;
		mod = do_div(div, bclk_rate);
		if (mod) {
			dev_err(dev, "Unsupported BCLK %dHz\n", bclk_rate);
			return -EINVAL;
		}
		dev_dbg(dev, "enables BCLK %dHz output\n", bclk_rate);
		reg5.r.i2s_pcm_clk_div = (u32)div - 1;
		reg5.r.i2s_pcm_clk_div_chan_en = 1;
	}

	regmap_write(cx2072x->regmap, CX2072X_I2SPCM_CONTROL1, reg1.ulVal);
	regmap_write(cx2072x->regmap, CX2072X_I2SPCM_CONTROL2, reg2.ulVal);
	regmap_write(cx2072x->regmap, CX2072X_I2SPCM_CONTROL3, reg3.ulVal);
	regmap_write(cx2072x->regmap, CX2072X_I2SPCM_CONTROL4, reg4.ulVal);
	regmap_write(cx2072x->regmap, CX2072X_I2SPCM_CONTROL6, reg6.ulVal);
	regmap_write(cx2072x->regmap, CX2072X_I2SPCM_CONTROL5, reg5.ulVal);

	/*Enables bclk and EAPD pin*/
	if (cx2072x->rev_id == CX2072X_REV_A2)
		regmap_update_bits(cx2072x->regmap, CX2072X_DIGITAL_BIOS_TEST2,
			0x84, 0xFF);
	else
		regmap_write(cx2072x->regmap, CX2072X_DIGITAL_BIOS_TEST2,
			regDBT2.ulVal);

	return 0;
}


static void cx2072x_dsp_init(struct snd_soc_codec *codec)
{
 struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	int band, channel, value;

	band = cx2072x->plbk_eq_band;
	channel = cx2072x->plbk_eq_channel;
	regmap_write(cx2072x->regmap, CX2072X_EQ_ENABLE_BYPASS, 0x6e0f);

	if (cx2072x->plbk_eq_update) {
		cx2072x_reg_bulk_write(codec, CX2072X_EQ_B0_COEFF,
				       cx2072x->plbk_eq[channel][band],
				       MAC_EQ_COEFF);
		value = band + (channel << 3) + (1 << 6);
		regmap_write(cx2072x->regmap, CX2072X_EQ_BAND, value);
		mdelay(5);
		cx2072x->plbk_eq_update = false;
	}

	if (cx2072x->plbk_drc_update) {
		cx2072x_reg_bulk_write(codec, CX2072X_SPKR_DRC_ENABLE_STEP,
			cx2072x->plbk_drc, MAX_DRC_REGS);
		cx2072x->plbk_drc_update = false;
	}
}

static void cx2072x_update_dsp(struct snd_soc_codec *codec)
{
	unsigned int afg_reg;
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	u8 drc_status = cx2072x->plbk_drc[0];

	regmap_read(cx2072x->regmap, CX2072X_AFG_POWER_STATE, &afg_reg);

	if ((afg_reg & 0xf) != 0)
		/*skip since device is on D3 mode*/
		return;


	if (!cx2072x->plbk_dsp_changed)
		/*nothing change*/
		return;



	if (cx2072x->plbk_eq_update || cx2072x->plbk_drc_update)
		cx2072x_dsp_init(codec);


	if (cx2072x->plbk_eq_en)
		regmap_write(cx2072x->regmap, CX2072X_EQ_ENABLE_BYPASS,
			0x6203);
	else
		regmap_write(cx2072x->regmap, CX2072X_EQ_ENABLE_BYPASS,
			0x620c);

	if (cx2072x->plbk_drc_en) {
		drc_status |= 0x1;
		regmap_write(cx2072x->regmap, CX2072X_SPKR_DRC_ENABLE_STEP,
		      drc_status);
		cx2072x->plbk_drc[0] = drc_status;
	} else {
		drc_status &= 0xfe;
		regmap_write(cx2072x->regmap, CX2072X_SPKR_DRC_ENABLE_STEP,
		      drc_status);
		cx2072x->plbk_drc[0] = drc_status;
	}

	cx2072x->plbk_dsp_changed = false;
}



static int afg_power_ev(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
#else
	struct snd_soc_codec *codec = w->codec;
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(cx2072x->regmap, CX2072X_DIGITAL_BIOS_TEST0,
			0x00, 0x10);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(cx2072x->regmap, CX2072X_DIGITAL_BIOS_TEST0,
			0x10, 0x10);
		break;
	}

	return 0;
}

#ifdef ENABLE_MIC_POP_WA
/*
 * This work will be called at ADC widget power on time.
 * to reduce initial mic pop noise caused by hardware
 */

static void cx2072x_anit_mic_pop_work(struct work_struct *work)
{
	struct snd_soc_dapm_context *dapm =
		container_of(work, struct snd_soc_dapm_context,
		delayed_work.work);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(dapm);
#else
	struct snd_soc_codec *codec = dapm->codec;
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	dev_dbg(cx2072x->dev, "Unmute I2S TX\n");
	/*Unmute I2S TX*/

	regmap_update_bits(cx2072x->regmap, CX2072X_I2SPCM_CONTROL4,
		0x2, 0x0);
}
#endif

static int adc1_power_ev(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
#ifdef ENABLE_MIC_POP_WA
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
#else
	struct snd_soc_codec *codec = w->codec;
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* Umute I2S TX after 300 ms to get around the mic
		 * pop noise issue.
		 */
		schedule_delayed_work(&codec->dapm.delayed_work,
			msecs_to_jiffies(300));
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*Mute TX I2S*/

		regmap_update_bits(cx2072x->regmap, CX2072X_I2SPCM_CONTROL4,
			0x2, 0x2);
		break;
	}
#endif
	return 0;
}


static int portg_power_ev(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
#else
	struct snd_soc_codec *codec = w->codec;
#endif

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		cx2072x_update_dsp(codec);
		break;
	default:
		break;
	}
	return 0;
}

static int cx2072x_plbk_eq_en_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int cx2072x_plbk_eq_en_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	ucontrol->value.integer.value[0] = cx2072x->plbk_eq_en;

	return 0;
}

static int cx2072x_plbk_eq_en_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	const bool enable = ucontrol->value.integer.value[0];

	if (ucontrol->value.integer.value[0] > 1)
		return -EINVAL;

	if (cx2072x->plbk_eq_en != enable) {
		cx2072x->plbk_eq_en = enable;
		cx2072x->plbk_dsp_changed = true;
		cx2072x_update_dsp(codec);
	}
	return 0;
}

static int cx2072x_plbk_drc_en_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;

	return 0;
}

static int cx2072x_plbk_drc_en_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	ucontrol->value.integer.value[0] = cx2072x->plbk_drc_en;

	return 0;
}

static int cx2072x_plbk_drc_en_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	const bool enable = ucontrol->value.integer.value[0];

	if (ucontrol->value.integer.value[0] > 1)
		return -EINVAL;

	if (cx2072x->plbk_drc_en != enable) {
		cx2072x->plbk_drc_en = enable;
		cx2072x->plbk_dsp_changed = true;
		cx2072x_update_dsp(codec);
	}

	return 0;
}

static int cx2072x_plbk_eq_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = CX2072X_PLBK_EQ_COEF_LEN;

	return 0;
}

static int cx2072x_plbk_eq_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	struct CX2072X_EQ_CTRL *eq =
		(struct CX2072X_EQ_CTRL *) kcontrol->private_value;
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx2072x->plbk_eq[eq->ch][eq->band];

	memcpy(param, cache, CX2072X_PLBK_EQ_COEF_LEN);

	return 0;
}

static int cx2072x_plbk_eq_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	struct CX2072X_EQ_CTRL *eq =
	(struct CX2072X_EQ_CTRL *) kcontrol->private_value;
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx2072x->plbk_eq[eq->ch][eq->band];

	mutex_lock(&cx2072x->eq_coeff_lock);
	memcpy(cache, param, CX2072X_PLBK_EQ_COEF_LEN);

	cx2072x->plbk_dsp_changed = true;
	cx2072x->plbk_eq_update = true;
	cx2072x->plbk_eq_channel = eq->ch;
	cx2072x->plbk_eq_band = eq->band;
	cx2072x_update_dsp(codec);
	mutex_unlock(&cx2072x->eq_coeff_lock);
	return 0;
}

static int cx2072x_classd_level_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = CX2072X_CLASSD_AMP_LEN;

	return 0;
}

static int cx2072x_classd_level_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx2072x->classd_amp;

	memcpy(param, cache, CX2072X_CLASSD_AMP_LEN);

	return 0;
}

static int cx2072x_classd_level_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx2072x->classd_amp;

	memcpy(cache, param, CX2072X_CLASSD_AMP_LEN);

	/* Config Power Averaging */
	cx2072x_reg_bulk_write(codec, CX2072X_ANALOG_TEST10,
			&cx2072x->classd_amp[0], 2);
	cx2072x_reg_bulk_write(codec, CX2072X_CODEC_TEST20,
			&cx2072x->classd_amp[2], 2);
	cx2072x_reg_bulk_write(codec, CX2072X_CODEC_TEST26,
			&cx2072x->classd_amp[4], 2);

	return 0;
}

static int cx2072x_plbk_drc_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = CX2072X_PLBK_DRC_PARM_LEN;

	return 0;
}

static int cx2072x_plbk_drc_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx2072x->plbk_drc;

	memcpy(param, cache, CX2072X_PLBK_DRC_PARM_LEN);

	return 0;
}

static int cx2072x_plbk_drc_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
#else
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#endif
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	u8 *param = ucontrol->value.bytes.data;
	u8 *cache = cx2072x->plbk_drc;

	memcpy(cache, param, CX2072X_PLBK_DRC_PARM_LEN);

	cx2072x->plbk_dsp_changed = true;
	cx2072x->plbk_drc_update = true;
	cx2072x_update_dsp(codec);

	return 0;
}

#define CX2072X_PLBK_DRC_COEF(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx2072x_plbk_drc_info, \
	.get = cx2072x_plbk_drc_get, .put = cx2072x_plbk_drc_put}

#define CX2072X_PLBK_EQ_COEF(xname, xch, xband) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE | \
		  SNDRV_CTL_ELEM_ACCESS_VOLATILE, \
	.info = cx2072x_plbk_eq_info, \
	.get = cx2072x_plbk_eq_get, .put = cx2072x_plbk_eq_put, \
	.private_value = \
		(unsigned long)&(struct CX2072X_EQ_CTRL) \
		 {.ch = xch, .band = xband } }

#define CX2072X_PLBK_DSP_EQ_SWITCH(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx2072x_plbk_eq_en_info, \
	.get = cx2072x_plbk_eq_en_get, .put = cx2072x_plbk_eq_en_put}

#define CX2072X_PLBK_DSP_DRC_SWITCH(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx2072x_plbk_drc_en_info, \
	.get = cx2072x_plbk_drc_en_get, .put = cx2072x_plbk_drc_en_put}

#define CX2072X_CLASSD_LEVEL(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = cx2072x_classd_level_info, \
	.get = cx2072x_classd_level_get, .put = cx2072x_classd_level_put}

static const struct snd_kcontrol_new cx2072x_snd_controls[] = {

	SOC_DOUBLE_R_TLV("PortD Boost", CX2072X_PORTD_GAIN_LEFT,
	CX2072X_PORTD_GAIN_RIGHT, 0, 3, 0, boost_tlv),
	SOC_DOUBLE_R_TLV("PortC Boost", CX2072X_PORTC_GAIN_LEFT,
	CX2072X_PORTC_GAIN_RIGHT, 0, 3, 0, boost_tlv),
	SOC_DOUBLE_R_TLV("PortB Boost", CX2072X_PORTB_GAIN_LEFT,
	CX2072X_PORTB_GAIN_RIGHT, 0, 3, 0, boost_tlv),

	SOC_DOUBLE_R_TLV("PortD ADC1 Volume", CX2072X_ADC1_AMP_GAIN_LEFT_1,
	CX2072X_ADC1_AMP_GAIN_RIGHT_1, 0, 0x4a, 0, adc_tlv),
	SOC_DOUBLE_R_TLV("PortC ADC1 Volume", CX2072X_ADC1_AMP_GAIN_LEFT_2,
	CX2072X_ADC1_AMP_GAIN_RIGHT_2, 0, 0x4a, 0, adc_tlv),
	SOC_DOUBLE_R_TLV("PortB ADC1 Volume", CX2072X_ADC1_AMP_GAIN_LEFT_0,
	CX2072X_ADC1_AMP_GAIN_RIGHT_0, 0, 0x4a, 0, adc_tlv),

	SOC_DOUBLE_R_TLV("DAC1 Volume", CX2072X_DAC1_AMP_GAIN_LEFT,
	CX2072X_DAC1_AMP_GAIN_RIGHT, 0, 0x4a, 0, dac_tlv),

	SOC_DOUBLE_R("DAC1 Mute", CX2072X_DAC1_AMP_GAIN_LEFT,
		CX2072X_DAC1_AMP_GAIN_RIGHT, 7,  1, 0),

	SOC_DOUBLE_R_TLV("DAC2 Volume", CX2072X_DAC2_AMP_GAIN_LEFT,
	CX2072X_DAC2_AMP_GAIN_RIGHT, 0, 0x4a, 0, dac_tlv),
	CX2072X_PLBK_DSP_EQ_SWITCH("EQ Switch"),
	CX2072X_PLBK_DSP_DRC_SWITCH("DRC Switch"),

	CX2072X_PLBK_EQ_COEF("DACL EQ 0", 0, 0),
	CX2072X_PLBK_EQ_COEF("DACL EQ 1", 0, 1),
	CX2072X_PLBK_EQ_COEF("DACL EQ 2", 0, 2),
	CX2072X_PLBK_EQ_COEF("DACL EQ 3", 0, 3),
	CX2072X_PLBK_EQ_COEF("DACL EQ 4", 0, 4),
	CX2072X_PLBK_EQ_COEF("DACL EQ 5", 0, 5),
	CX2072X_PLBK_EQ_COEF("DACL EQ 6", 0, 6),
	CX2072X_PLBK_EQ_COEF("DACR EQ 0", 1, 0),
	CX2072X_PLBK_EQ_COEF("DACR EQ 1", 1, 1),
	CX2072X_PLBK_EQ_COEF("DACR EQ 2", 1, 2),
	CX2072X_PLBK_EQ_COEF("DACR EQ 3", 1, 3),
	CX2072X_PLBK_EQ_COEF("DACR EQ 4", 1, 4),
	CX2072X_PLBK_EQ_COEF("DACR EQ 5", 1, 5),
	CX2072X_PLBK_EQ_COEF("DACR EQ 6", 1, 6),
	CX2072X_PLBK_DRC_COEF("DRC"),
	SOC_SINGLE_TLV("HPF Freq", CX2072X_CODEC_TEST9, 0, 0x3f, 0, hpf_tlv),
	SOC_DOUBLE("HPF Switch", CX2072X_CODEC_TEST9, 8, 9, 1, 1),
	CX2072X_CLASSD_LEVEL("Class-D Output Level"),
};

/*
 * cx2072x_hs_jack_report: Report jack notification to upper layer
 * @codec : pointer variable to codec having information related to codec
 * @jack : Pointer variable to snd_soc_jack having information of codec
 * and pin number$
 * @report : Provides informaton of whether it is headphone or microphone
 *
 */
int cx2072x_hs_jack_report(struct snd_soc_codec *codec)
{
	unsigned int jack;
	unsigned int type = 0;
	int  state = 0;
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	regcache_cache_bypass(cx2072x->regmap, true);
	cx2072x->jack_state = CX_JACK_NONE;
	regmap_read(cx2072x->regmap, CX2072X_PORTA_PIN_SENSE, &jack);
	jack = jack >> 24;
	regmap_read(cx2072x->regmap, CX2072X_DIGITAL_TEST11, &type);
	regcache_cache_bypass(cx2072x->regmap, false);
	if (jack == 0x80) {
		type = type >> 8;

		if (type & 0x8) {
			state |= SND_JACK_HEADSET;
			cx2072x->jack_state = CX_JACK_APPLE_HEADSET;
			if (type & 0x2)
				state |= SND_JACK_BTN_0;
		} else if (type & 0x4) {
			state |= SND_JACK_HEADPHONE;
			cx2072x->jack_state = CX_JACK_NOKIE_HEADSET;
		} else {
			state |= SND_JACK_HEADPHONE;
			cx2072x->jack_state = CX_JACK_HEADPHONE;
		}
	}

	/* clear interrupt */
	regmap_write(cx2072x->regmap, CX2072X_UM_INTERRUPT_CRTL_E, 0x12 << 24);

	dev_err(codec->dev, "CX2072X_HSDETECT type=0x%X,Jack state = %x\n",
		type, state);
	return state;
}
EXPORT_SYMBOL_GPL(cx2072x_hs_jack_report);

static int cx2072x_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask,
	unsigned int rx_mask, int slots, int slot_width)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx2072x_priv *cx2072x = snd_soc_codec_get_drvdata(codec);

	if (slots == 0)
		goto out;


	switch (rx_mask) {
	case 1 ... 0xf:
	default:
		return -EINVAL;
	}


	switch (tx_mask) {
	case 1 ... 0xf:
	default:
		return -EINVAL;
	}

out:
	cx2072x->tdm_rx_mask = rx_mask;
	cx2072x->tdm_tx_mask = tx_mask;
	cx2072x->tdm_slot_width = slot_width;
	cx2072x->tdm_slots = slots;
	return 0;
}

static int cx2072x_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	struct device *dev = codec->dev;
	const unsigned int sample_rate = params_rate(params);
	int sample_size, frame_size;

	/* Data sizes if not using TDM */
	sample_size = params_width(params);

	if (sample_size < 0)
		return sample_size;

	frame_size = snd_soc_params_to_frame_size(params);
	if (frame_size < 0)
		return frame_size;

	if (cx2072x->mclk_rate == 0) {
		dev_err(dev, "Master clock rate is not configued\n");
		return -EINVAL;
	}

	if (cx2072x->bclk_ratio)
		frame_size = cx2072x->bclk_ratio;

	switch (sample_rate) {
	case 48000:
	case 32000:
	case 24000:
	case 16000:
	case 96000:
	case 192000:
		break;
	default:
		dev_err(dev, "Unsupported sample rate %d\n", sample_rate);
		return -EINVAL;
	}

	dev_dbg(dev, "Sample size %d bits, frame = %d bits, rate = %d Hz\n",
		sample_size, frame_size, sample_rate);

	cx2072x->frame_size = frame_size;
	cx2072x->sample_size = sample_size;
	cx2072x->sample_rate = sample_rate;

	if (dai->id == CX2072X_DAI_DSP) {
		cx2072x->en_aec_ref = true;
		dev_dbg(cx2072x->dev, "enables aec reference\n");
		regmap_write(cx2072x->regmap,
			     CX2072X_ADC1_CONNECTION_SELECT_CONTROL, 3);
	}

	if (cx2072x->pll_changed) {
		cx2072x_config_pll(cx2072x);
		cx2072x->pll_changed = false;
	}
	if (cx2072x->i2spcm_changed) {
		cx2072x_config_i2spcm(cx2072x);
		cx2072x->i2spcm_changed = false;
	}

	return 0;
}

static void cx2072x_shutdown(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	/* shutdown codec. */
	if (!snd_soc_codec_is_active(codec)) {
		regcache_cache_only(cx2072x->regmap, false);
		regmap_write(cx2072x->regmap, CX2072X_PORTA_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_PORTB_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_PORTC_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_PORTD_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_PORTE_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_PORTG_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_ADC1_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_ADC2_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_DAC1_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_DAC2_POWER_STATE, 3);
		regmap_write(cx2072x->regmap, CX2072X_AFG_POWER_STATE, 3);
		mdelay(200);
	}
}

static int cx2072x_set_dai_bclk_ratio(struct snd_soc_dai *dai,
				      unsigned int ratio)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	cx2072x->bclk_ratio = ratio;
	return 0;
}


static int cx2072x_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
	unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	switch (clk_id) {
	case CX2072X_MCLK_EXTERNAL_PLL:
		if (cx2072x->mclk && clk_set_rate(cx2072x->mclk, freq))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	cx2072x->mclk_rate = freq;
	return 0;
}

static int cx2072x_set_dai_fmt(struct snd_soc_dai *dai,
	unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	struct device *dev = codec->dev;

	dev_dbg(dev, "set_dai_fmt- %08x\n", fmt);
	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		dev_err(dev, "Unsupported DAI master mode\n");
		return -EINVAL;
	}

	/* set format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		dev_err(dev, "Unsupported DAI format\n");
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_IB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		dev_err(dev, "Unsupported DAI clock inversion\n");
		return -EINVAL;
	}

	cx2072x->dai_fmt = fmt;
	return 0;
}


static const char * const dac_enum_text[] = {
	"DAC1 Switch", "DAC2 Switch",
};

static const struct soc_enum porta_dac_enum =
SOC_ENUM_SINGLE(CX2072X_PORTA_CONNECTION_SELECT_CTRL, 0, 2, dac_enum_text);

static const struct snd_kcontrol_new porta_mux =
SOC_DAPM_ENUM("PortA Mux", porta_dac_enum);

static const struct soc_enum portg_dac_enum =
SOC_ENUM_SINGLE(CX2072X_PORTG_CONNECTION_SELECT_CTRL, 0, 2, dac_enum_text);

static const struct snd_kcontrol_new portg_mux =
SOC_DAPM_ENUM("PortG Mux", portg_dac_enum);

static const struct soc_enum porte_dac_enum =
SOC_ENUM_SINGLE(CX2072X_PORTE_CONNECTION_SELECT_CTRL, 0, 2, dac_enum_text);

static const struct snd_kcontrol_new porte_mux =
SOC_DAPM_ENUM("PortE Mux", porte_dac_enum);

static const char * const adc1in_sel_text[] = {
	"PortB Switch", "PortD Switch", "PortC Switch", "Widget15 Switch",
	"PortE Switch", "PortF Switch", "PortH Switch"
};

static const struct soc_enum adc1in_sel_enum =
SOC_ENUM_SINGLE(CX2072X_ADC1_CONNECTION_SELECT_CONTROL, 0, 7, adc1in_sel_text);

static const struct snd_kcontrol_new adc1_mux =
SOC_DAPM_ENUM("ADC1 Mux", adc1in_sel_enum);

#define CX2072X_DAPM_SUPPLY_S(wname, wsubseq, wreg, wshift, wmask,  won_val, \
	woff_val, wevent, wflags) \
	{.id = snd_soc_dapm_supply, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = wreg, .shift = wshift, .mask = wmask, \
	.on_val = won_val, .off_val = woff_val, \
	.subseq = wsubseq, .event = wevent, .event_flags = wflags}

#define CX2072X_DAPM_SWITCH(wname,  wreg, wshift, wmask,  won_val, woff_val, \
	wevent, wflags) \
	{.id = snd_soc_dapm_switch, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = wreg, .shift = wshift, .mask = wmask, \
	.on_val = won_val, .off_val = woff_val, \
	.event = wevent, .event_flags = wflags}


#define CX2072X_DAPM_SWITCH(wname,  wreg, wshift, wmask,  won_val, woff_val, \
	wevent, wflags) \
	{.id = snd_soc_dapm_switch, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = wreg, .shift = wshift, .mask = wmask, \
	.on_val = won_val, .off_val = woff_val, \
	.event = wevent, .event_flags = wflags}

#define CX2072X_DAPM_REG_E(wid, wname, wreg, wshift, wmask, won_val, woff_val, \
				wevent, wflags) \
	{.id = wid, .name = wname, .kcontrol_news = NULL, .num_kcontrols = 0, \
	.reg = wreg, .shift = wshift, .mask = wmask, \
	.on_val = won_val, .off_val = woff_val, \
	.event = wevent, .event_flags = wflags}

static const struct snd_soc_dapm_widget cx2072x_dapm_widgets[] = {
	/*Playback*/
	SND_SOC_DAPM_AIF_IN("In AIF", "Playback", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_REG(snd_soc_dapm_dac, "DAC1", CX2072X_DAC1_POWER_STATE,
	0, 0xFFF, 0x00, 0x03),

	SND_SOC_DAPM_REG(snd_soc_dapm_dac, "DAC2", CX2072X_DAC2_POWER_STATE,
	0, 0xFFF, 0x00, 0x03),

	SND_SOC_DAPM_MUX("PortA Mux", SND_SOC_NOPM, 0, 0, &porta_mux),
	SND_SOC_DAPM_MUX("PortG Mux", SND_SOC_NOPM, 0, 0, &portg_mux),
	SND_SOC_DAPM_MUX("PortE Mux", SND_SOC_NOPM, 0, 0, &porte_mux),

	SND_SOC_DAPM_REG(snd_soc_dapm_switch, "PortA",
		CX2072X_PORTA_POWER_STATE, 0, 0xFFF, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_switch, "PortG",
		CX2072X_PORTG_POWER_STATE, 0, 0xFFF, 0x00, 0x03),
	CX2072X_DAPM_SWITCH("PortG", CX2072X_PORTG_POWER_STATE, 0, 0xFF,
		0x00, 0x03, portg_power_ev, SND_SOC_DAPM_POST_PMU),

	CX2072X_DAPM_SUPPLY_S("AFG Power", 0, CX2072X_AFG_POWER_STATE,
		0, 0xFFF, 0x00, 0x03, afg_power_ev,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_OUTPUT("PORTA"),
	SND_SOC_DAPM_OUTPUT("PORTG"),
	SND_SOC_DAPM_OUTPUT("PORTE"),
	SND_SOC_DAPM_OUTPUT("AEC REF"),

	/*Capture*/
	SND_SOC_DAPM_AIF_OUT("Out AIF", "Capture", 0, SND_SOC_NOPM, 0, 0),

	CX2072X_DAPM_REG_E(snd_soc_dapm_adc, "ADC1", CX2072X_ADC1_POWER_STATE,
		0, 0xFF, 0x00, 0x03, adc1_power_ev,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_REG(snd_soc_dapm_adc, "ADC2", CX2072X_ADC2_POWER_STATE,
		0, 0xFF, 0x00, 0x03),

	SND_SOC_DAPM_MUX("ADC1 Mux", SND_SOC_NOPM, 0, 0, &adc1_mux),

	SND_SOC_DAPM_REG(snd_soc_dapm_switch, "PortB",
		CX2072X_PORTB_POWER_STATE, 0, 0xFFF, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_switch, "PortC",
		CX2072X_PORTC_POWER_STATE, 0, 0xFFF, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_switch, "PortD",
		CX2072X_PORTD_POWER_STATE, 0, 0xFFF, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_switch, "PortE",
		CX2072X_PORTE_POWER_STATE, 0, 0xFFF, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_switch, "Widget15",
		CX2072X_MIXER_POWER_STATE, 0, 0xFFF, 0x00, 0x03),

	SND_SOC_DAPM_INPUT("PORTB"),
	SND_SOC_DAPM_INPUT("PORTC"),
	SND_SOC_DAPM_INPUT("PORTD"),

	SND_SOC_DAPM_MICBIAS("Headset Bias", CX2072X_ANALOG_TEST11, 1, 0),
	SND_SOC_DAPM_MICBIAS("PortD Mic Bias", CX2072X_PORTD_PIN_CTRL, 2, 0),
	SND_SOC_DAPM_MICBIAS("PortB Mic Bias", CX2072X_PORTB_PIN_CTRL, 2, 0),
};

static const struct snd_soc_dapm_route cx2072x_intercon[] = {
	/* Playback */
	{"In AIF", NULL, "AFG Power"},
	{"DAC1", NULL, "In AIF"},
	{"DAC2", NULL, "In AIF"},
	{"PortA Mux", "DAC1 Switch", "DAC1"},
	{"PortA Mux", "DAC2 Switch", "DAC2"},
	{"PortG Mux", "DAC1 Switch", "DAC1"},
	{"PortG Mux", "DAC2 Switch", "DAC2"},
	{"PortE Mux", "DAC1 Switch", "DAC1"},
	{"PortE Mux", "DAC2 Switch", "DAC2"},
	{"Widget15", NULL, "DAC1"},
	{"Widget15", NULL, "DAC2"},
	{"PortA", NULL, "PortA Mux"},
	{"PortG", NULL, "PortG Mux"},
	{"PortE", NULL, "PortE Mux"},
	{"PORTA", NULL, "PortA"},
	{"PORTG", NULL, "PortG"},
	{"PORTE", NULL, "PortE"},

	/* Capture */
	{"PORTD", NULL, "Headset Bias"},
	{"PortD", NULL, "PORTD"},
	{"PortC", NULL, "PORTC"},
	{"PortB", NULL, "PORTB"},
	{"ADC1 Mux", "PortD Switch", "PortD"},
	{"ADC1 Mux", "PortC Switch", "PortC"},
	{"ADC1 Mux", "PortB Switch", "PortB"},
	{"ADC1 Mux", "Widget15 Switch", "Widget15"},
	{"ADC1", NULL, "ADC1 Mux"},
	{"Out AIF", NULL, "ADC1"},
	{"Out AIF", NULL, "AFG Power"},
	{"AEC REF", NULL, "Out AIF"},
};


static void cx2072x_sw_reset(struct cx2072x_priv *cx2072x)
{

	regmap_write(cx2072x->regmap, CX2072X_AFG_FUNCTION_RESET, 0x01);
	regmap_write(cx2072x->regmap, CX2072X_AFG_FUNCTION_RESET, 0x01);
}

static int cx2072x_init(struct snd_soc_codec *codec)
{
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);

	cx2072x->plbk_dsp_changed = true;

	regmap_write(cx2072x->regmap, CX2072X_AFG_POWER_STATE, 0);
	/* reduce the jack monitor time*/
	regmap_update_bits(cx2072x->regmap, CX2072X_DIGITAL_TEST15,
	0x00, 0x06);

	cx2072x_config_headset_det(cx2072x);
	/* configre PortC as input device */
	regmap_update_bits(cx2072x->regmap, CX2072X_PORTC_PIN_CTRL,
		0x20, 0x20);


	/*enable bclk and EAPD input*/
	if (cx2072x->rev_id == CX2072X_REV_A2)
		regmap_update_bits(cx2072x->regmap, CX2072X_DIGITAL_BIOS_TEST2,
			0x84, 0xFF);

	return 0;
}
static int cx2072x_set_bias_level(struct snd_soc_codec *codec,
enum snd_soc_bias_level level)
{
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	const enum snd_soc_bias_level old_level =
		 snd_soc_codec_get_bias_level(codec);
#else
	const enum snd_soc_bias_level old_level = codec->dapm.bias_level;
#endif
	int ret;

	switch (level) {
	case SND_SOC_BIAS_ON:
		dev_dbg(cx2072x->dev, "SND_SOC_BIAS_ON\n");
		break;
	case SND_SOC_BIAS_PREPARE:
		dev_dbg(cx2072x->dev, "SND_SOC_BIAS_PREPARE\n");
		break;
	case SND_SOC_BIAS_STANDBY:
		dev_dbg(cx2072x->dev, "SND_SOC_BIAS_STANDBY\n");

		if (old_level == SND_SOC_BIAS_OFF) {
			if (cx2072x->mclk) {
				dev_dbg(cx2072x->dev, "Turn on MCLK\n");
				ret = clk_prepare_enable(cx2072x->mclk);
				if (ret)
					return ret;
			}
			dev_dbg(codec->dev, "cache only =>false\n");
			regcache_cache_only(cx2072x->regmap, false);
			dev_dbg(codec->dev, "regcache_sync\n");
			regmap_write(cx2072x->regmap,
				     CX2072X_AFG_POWER_STATE, 0);
			regcache_sync(cx2072x->regmap);
			dev_dbg(codec->dev, "regcache_sync done\n");
		}
		break;
	case SND_SOC_BIAS_OFF:
		dev_dbg(cx2072x->dev, "SND_SOC_BIAS_OFF\n");
		/* Gracefully shutdwon codec*/

		/*Shutdown codec completely*/
		cx2072x_sw_reset(cx2072x);
		regmap_write(cx2072x->regmap, CX2072X_AFG_POWER_STATE, 3);
		mdelay(200);
		dev_dbg(codec->dev, "cache only\n");
		regcache_mark_dirty(cx2072x->regmap);
		regcache_cache_only(cx2072x->regmap, true);
		if (cx2072x->mclk) {
			dev_dbg(cx2072x->dev, "Turn off MCLK\n");
			clk_disable_unprepare(cx2072x->mclk);
		}
		break;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	codec->dapm.bias_level = level;
#endif
	return 0;
}


static int cx2072x_probe(struct snd_soc_codec *codec)
{
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	int ret = 0;
	unsigned int ven_id;

	cx2072x->codec = codec;
	codec->control_data = cx2072x->regmap;


	dev_dbg(codec->dev, "codec version: 4.4.50\n");
	regmap_read(cx2072x->regmap, CX2072X_VENDOR_ID, &ven_id);
	regmap_read(cx2072x->regmap, CX2072X_REVISION_ID, &cx2072x->rev_id);
	dev_dbg(codec->dev, "codec version: %08x,%08x\n",
		ven_id, cx2072x->rev_id);

	/* Check if MCLK is specified, if not the clock is control by machine
	 * driver
	 */
	cx2072x->mclk = devm_clk_get(codec->dev, "mclk");
	if (IS_ERR(cx2072x->mclk)) {
		ret = PTR_ERR(cx2072x->mclk);
		if (ret == -ENOENT) {
			cx2072x->mclk = NULL;
			dev_warn(codec->dev, "Assuming static MCLK\n");
			ret = 0;
		} else {
			dev_err(codec->dev, "Failed to get MCLK: %d\n", ret);
			return ret;
		}
	}

	dev_dbg(codec->dev, "Initialize codec\n");

	/* enable clock for codec access*/
	if (cx2072x->mclk)
		ret = clk_prepare_enable(cx2072x->mclk);

	cx2072x_init(codec);

	ret = regmap_register_patch(cx2072x->regmap, cx2072x_patch,
		ARRAY_SIZE(cx2072x_patch));
	if (ret)
		return ret;
	regmap_write(cx2072x->regmap, CX2072X_AFG_POWER_STATE, 3);
	regcache_cache_only(cx2072x->regmap, true);

	/* disable clock */
	if (cx2072x->mclk)
		clk_disable_unprepare(cx2072x->mclk);
	return ret;
}

static int cx2072x_remove(struct snd_soc_codec *codec)
{
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(codec);
	/*power off device*/
	cx2072x_set_bias_level(cx2072x->codec, SND_SOC_BIAS_OFF);
	return 0;
}

#ifdef CONFIG_PM
static int cx2072x_runtime_suspend(struct device *dev)
{
	struct cx2072x_priv *cx2072x = dev_get_drvdata(dev);

	dev_dbg(cx2072x->codec->dev, "%s----%d\n", __func__, __LINE__);


	return 0;
}

static int cx2072x_runtime_resume(struct device *dev)
{
	struct cx2072x_priv *cx2072x = dev_get_drvdata(dev);

	dev_dbg(cx2072x->codec->dev, "%s----%d\n", __func__, __LINE__);

	return 0;
}

#else
#define cx2072x_suspend NULL
#define cx2072x_resume NULL
#endif




static bool cx2072x_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {

	case CX2072X_VENDOR_ID:
	case CX2072X_REVISION_ID:
	case CX2072X_CURRENT_BCLK_FREQUENCY:
	case CX2072X_AFG_POWER_STATE:
	case CX2072X_UM_RESPONSE:
	case CX2072X_GPIO_DATA:
	case CX2072X_GPIO_ENABLE:
	case CX2072X_GPIO_DIRECTION:
	case CX2072X_GPIO_WAKE:
	case CX2072X_GPIO_UM_ENABLE:
	case CX2072X_GPIO_STICKY_MASK:
	case CX2072X_DAC1_CONVERTER_FORMAT:
	case CX2072X_DAC1_AMP_GAIN_RIGHT:
	case CX2072X_DAC1_AMP_GAIN_LEFT:
	case CX2072X_DAC1_POWER_STATE:
	case CX2072X_DAC1_CONVERTER_STREAM_CHANNEL:
	case CX2072X_DAC1_EAPD_ENABLE:
	case CX2072X_DAC2_CONVERTER_FORMAT:
	case CX2072X_DAC2_AMP_GAIN_RIGHT:
	case CX2072X_DAC2_AMP_GAIN_LEFT:
	case CX2072X_DAC2_POWER_STATE:
	case CX2072X_DAC2_CONVERTER_STREAM_CHANNEL:
	case CX2072X_ADC1_CONVERTER_FORMAT:
	case CX2072X_ADC1_AMP_GAIN_RIGHT_0:
	case CX2072X_ADC1_AMP_GAIN_LEFT_0:
	case CX2072X_ADC1_AMP_GAIN_RIGHT_1:
	case CX2072X_ADC1_AMP_GAIN_LEFT_1:
	case CX2072X_ADC1_AMP_GAIN_RIGHT_2:
	case CX2072X_ADC1_AMP_GAIN_LEFT_2:
	case CX2072X_ADC1_AMP_GAIN_RIGHT_3:
	case CX2072X_ADC1_AMP_GAIN_LEFT_3:
	case CX2072X_ADC1_AMP_GAIN_RIGHT_4:
	case CX2072X_ADC1_AMP_GAIN_LEFT_4:
	case CX2072X_ADC1_AMP_GAIN_RIGHT_5:
	case CX2072X_ADC1_AMP_GAIN_LEFT_5:
	case CX2072X_ADC1_AMP_GAIN_RIGHT_6:
	case CX2072X_ADC1_AMP_GAIN_LEFT_6:
	case CX2072X_ADC1_CONNECTION_SELECT_CONTROL:
	case CX2072X_ADC1_POWER_STATE:
	case CX2072X_ADC1_CONVERTER_STREAM_CHANNEL:
	case CX2072X_ADC2_CONVERTER_FORMAT:
	case CX2072X_ADC2_AMP_GAIN_RIGHT_0:
	case CX2072X_ADC2_AMP_GAIN_LEFT_0:
	case CX2072X_ADC2_AMP_GAIN_RIGHT_1:
	case CX2072X_ADC2_AMP_GAIN_LEFT_1:
	case CX2072X_ADC2_AMP_GAIN_RIGHT_2:
	case CX2072X_ADC2_AMP_GAIN_LEFT_2:
	case CX2072X_ADC2_CONNECTION_SELECT_CONTROL:
	case CX2072X_ADC2_POWER_STATE:
	case CX2072X_ADC2_CONVERTER_STREAM_CHANNEL:
	case CX2072X_PORTA_CONNECTION_SELECT_CTRL:
	case CX2072X_PORTA_POWER_STATE:
	case CX2072X_PORTA_PIN_CTRL:
	case CX2072X_PORTA_UNSOLICITED_RESPONSE:
	case CX2072X_PORTA_PIN_SENSE:
	case CX2072X_PORTA_EAPD_BTL:
	case CX2072X_PORTB_POWER_STATE:
	case CX2072X_PORTB_PIN_CTRL:
	case CX2072X_PORTB_UNSOLICITED_RESPONSE:
	case CX2072X_PORTB_PIN_SENSE:
	case CX2072X_PORTB_EAPD_BTL:
	case CX2072X_PORTB_GAIN_RIGHT:
	case CX2072X_PORTB_GAIN_LEFT:
	case CX2072X_PORTC_POWER_STATE:
	case CX2072X_PORTC_PIN_CTRL:
	case CX2072X_PORTC_GAIN_RIGHT:
	case CX2072X_PORTC_GAIN_LEFT:
	case CX2072X_PORTD_POWER_STATE:
	case CX2072X_PORTD_PIN_CTRL:
	case CX2072X_PORTD_UNSOLICITED_RESPONSE:
	case CX2072X_PORTD_PIN_SENSE:
	case CX2072X_PORTD_GAIN_RIGHT:
	case CX2072X_PORTD_GAIN_LEFT:
	case CX2072X_PORTE_CONNECTION_SELECT_CTRL:
	case CX2072X_PORTE_POWER_STATE:
	case CX2072X_PORTE_PIN_CTRL:
	case CX2072X_PORTE_UNSOLICITED_RESPONSE:
	case CX2072X_PORTE_PIN_SENSE:
	case CX2072X_PORTE_EAPD_BTL:
	case CX2072X_PORTE_GAIN_RIGHT:
	case CX2072X_PORTE_GAIN_LEFT:
	case CX2072X_PORTF_POWER_STATE:
	case CX2072X_PORTF_PIN_CTRL:
	case CX2072X_PORTF_UNSOLICITED_RESPONSE:
	case CX2072X_PORTF_PIN_SENSE:
	case CX2072X_PORTF_GAIN_RIGHT:
	case CX2072X_PORTF_GAIN_LEFT:
	case CX2072X_PORTG_POWER_STATE:
	case CX2072X_PORTG_PIN_CTRL:
	case CX2072X_PORTG_CONNECTION_SELECT_CTRL:
	case CX2072X_PORTG_EAPD_BTL:
	case CX2072X_PORTM_POWER_STATE:
	case CX2072X_PORTM_PIN_CTRL:
	case CX2072X_PORTM_CONNECTION_SELECT_CTRL:
	case CX2072X_PORTM_EAPD_BTL:
	case CX2072X_MIXER_POWER_STATE:
	case CX2072X_MIXER_GAIN_RIGHT_0:
	case CX2072X_MIXER_GAIN_LEFT_0:
	case CX2072X_MIXER_GAIN_RIGHT_1:
	case CX2072X_MIXER_GAIN_LEFT_1:
	case CX2072X_EQ_ENABLE_BYPASS:
	case CX2072X_EQ_B0_COEFF:
	case CX2072X_EQ_B1_COEFF:
	case CX2072X_EQ_B2_COEFF:
	case CX2072X_EQ_A1_COEFF:
	case CX2072X_EQ_A2_COEFF:
	case CX2072X_EQ_G_COEFF:
	case CX2072X_SPKR_DRC_ENABLE_STEP:
	case CX2072X_SPKR_DRC_CONTROL:
	case CX2072X_SPKR_DRC_TEST:
	case CX2072X_DIGITAL_BIOS_TEST0:
	case CX2072X_DIGITAL_BIOS_TEST2:
	case CX2072X_I2SPCM_CONTROL1:
	case CX2072X_I2SPCM_CONTROL2:
	case CX2072X_I2SPCM_CONTROL3:
	case CX2072X_I2SPCM_CONTROL4:
	case CX2072X_I2SPCM_CONTROL5:
	case CX2072X_I2SPCM_CONTROL6:
	case CX2072X_UM_INTERRUPT_CRTL_E:
	case CX2072X_CODEC_TEST2:
	case CX2072X_CODEC_TEST9:
	case CX2072X_CODEC_TEST20:
	case CX2072X_CODEC_TEST26:
	case CX2072X_ANALOG_TEST4:
	case CX2072X_ANALOG_TEST5:
	case CX2072X_ANALOG_TEST6:
	case CX2072X_ANALOG_TEST7:
	case CX2072X_ANALOG_TEST8:
	case CX2072X_ANALOG_TEST9:
	case CX2072X_ANALOG_TEST10:
	case CX2072X_ANALOG_TEST11:
	case CX2072X_ANALOG_TEST12:
	case CX2072X_ANALOG_TEST13:
	case CX2072X_DIGITAL_TEST0:
	case CX2072X_DIGITAL_TEST1:
	case CX2072X_DIGITAL_TEST11:
	case CX2072X_DIGITAL_TEST12:
	case CX2072X_DIGITAL_TEST15:
	case CX2072X_DIGITAL_TEST16:
	case CX2072X_DIGITAL_TEST17:
	case CX2072X_DIGITAL_TEST18:
	case CX2072X_DIGITAL_TEST19:
	case CX2072X_DIGITAL_TEST20:
		return true;
	default:
		return false;
	}
}

static bool cx2072x_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CX2072X_VENDOR_ID:
	case CX2072X_REVISION_ID:
	case CX2072X_UM_INTERRUPT_CRTL_E:
	case CX2072X_DIGITAL_TEST11:
	case CX2072X_PORTA_PIN_SENSE:
	case CX2072X_PORTB_PIN_SENSE:
	case CX2072X_PORTD_PIN_SENSE:
	case CX2072X_PORTE_PIN_SENSE:
	case CX2072X_PORTF_PIN_SENSE:
	case CX2072X_EQ_G_COEFF:
	case CX2072X_EQ_BAND:
		return true;
	default:
		return false;

	}
}


static struct snd_soc_codec_driver soc_codec_driver_cx2072x = {
	.probe = cx2072x_probe,
	.remove = cx2072x_remove,
	.set_bias_level = cx2072x_set_bias_level,
	.suspend_bias_off = true,
	.idle_bias_off = 1,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	.component_driver = {
		.controls = cx2072x_snd_controls,
		.num_controls = ARRAY_SIZE(cx2072x_snd_controls),
		.dapm_widgets = cx2072x_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(cx2072x_dapm_widgets),
		.dapm_routes = cx2072x_intercon,
		.num_dapm_routes = ARRAY_SIZE(cx2072x_intercon),
	},
#else
	.controls = cx2072x_snd_controls,
	.num_controls = ARRAY_SIZE(cx2072x_snd_controls),
	.dapm_widgets = cx2072x_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cx2072x_dapm_widgets),
	.dapm_routes = cx2072x_intercon,
	.num_dapm_routes = ARRAY_SIZE(cx2072x_intercon),
#endif
};

/*
 * DAI ops
 */
static struct snd_soc_dai_ops cx2072x_dai_ops = {
	.set_sysclk = cx2072x_set_dai_sysclk,
	.set_fmt = cx2072x_set_dai_fmt,
	.set_tdm_slot = cx2072x_set_tdm_slot,
	.hw_params = cx2072x_hw_params,
	.shutdown = cx2072x_shutdown,
	.set_bclk_ratio = cx2072x_set_dai_bclk_ratio,
};

static int cx2072x_dsp_dai_probe(struct snd_soc_dai *dai)
{
	struct cx2072x_priv *cx2072x = get_cx2072x_priv(dai->codec);

	cx2072x->en_aec_ref = true;
	return 0;
}

#define CX2072X_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
			 | SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_driver soc_codec_cx2072x_dai[] = {
	{ /* playback and capture */
		.name = "cx2072x-hifi",
		.id	= CX2072X_DAI_HIFI,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = CX2072X_RATES_DSP,
			.formats = CX2072X_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = CX2072X_RATES_DSP,
			.formats = CX2072X_FORMATS,
		},
		.ops = &cx2072x_dai_ops,
		.symmetric_rates = 1,
	},
	{ /* plabayck only, return echo reference to Conexant DSP chip*/
		.name = "cx2072x-dsp",
		.id	= CX2072X_DAI_DSP,
		.probe = cx2072x_dsp_dai_probe,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = CX2072X_RATES_DSP,
			.formats = CX2072X_FORMATS,
		},
		.ops = &cx2072x_dai_ops,
	},
	{ /* plabayck only, return echo reference through I2S TX*/
		.name = "cx2072x-aec",
		.id	= 3,
		.capture = {
			.stream_name = "Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = CX2072X_RATES_DSP,
			.formats = CX2072X_FORMATS,
		},
	},
};
EXPORT_SYMBOL_GPL(soc_codec_cx2072x_dai);

static const struct regmap_config cx2072x_regmap = {
	.reg_bits = 16,
	.val_bits = 32,
	.max_register = CX2072X_REG_MAX, .reg_defaults = cx2072x_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(cx2072x_reg_defaults),
	.cache_type = REGCACHE_RBTREE,

	.readable_reg = cx2072x_readable_register,
	.volatile_reg = cx2072x_volatile_register,
	/* Needs custom READ/WRITE functions
	 * for various register lengths.
	 */
	.reg_read = cx2072x_reg_read,
	.reg_write = cx2072x_reg_write,
};


static int cx2072x_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *id)
{
	int ret = -1;
	struct cx2072x_priv *cx2072x;

	cx2072x = (struct cx2072x_priv *)devm_kzalloc(
		&i2c->dev, sizeof(struct cx2072x_priv), GFP_KERNEL);
	if (cx2072x == NULL) {
		dev_err(&i2c->dev, "Out of memory!\n");
		return -ENOMEM;
	}

	cx2072x->regmap = devm_regmap_init(&i2c->dev, NULL, i2c,
		&cx2072x_regmap);
	if (IS_ERR(&cx2072x->regmap)) {
		ret = PTR_ERR(cx2072x->regmap);
		dev_err(&i2c->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}


	mutex_init(&cx2072x->eq_coeff_lock);
	mutex_init(&cx2072x->lock);

	i2c_set_clientdata(i2c, cx2072x);

	cx2072x->dev = &i2c->dev;
	cx2072x->pll_changed = true;
	cx2072x->i2spcm_changed = true;

	/* sets the frame size to
	 * Frame size = number of channel * sample width
	 */
	cx2072x->bclk_ratio = 0;

	ret = snd_soc_register_codec(cx2072x->dev,
		&soc_codec_driver_cx2072x, soc_codec_cx2072x_dai,
			ARRAY_SIZE(soc_codec_cx2072x_dai));
	if (ret < 0)
		dev_err(cx2072x->dev,
		"Failed to register codec: %d\n", ret);
	else
		dev_dbg(cx2072x->dev,
		"%s: Register codec.\n", __func__);

	return ret;
}

static int cx2072x_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}

static void cx2072x_i2c_shutdown(struct i2c_client *client)
{
	/*
	 * struct cx2072x_priv *cx2072x = i2c_get_clientdata(client);
	 */
}


const struct dev_pm_ops cx2072x_pm_ops = {
	SET_RUNTIME_PM_OPS(cx2072x_runtime_suspend, cx2072x_runtime_resume,
	NULL)
};
static const struct i2c_device_id cx2072x_i2c_id[] = {
	{ "cx20721", 0 },
	{ "cx20723", 0 },
	{ "14F10720", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, cx2072x_i2c_id);

static const struct of_device_id cx2072x_of_match[] = {
	{ .compatible = "cnxt,cx20721", },
	{ .compatible = "cnxt,cx20723", },
	{ .compatible = "cnxt,cx7601", },
	{}
};
MODULE_DEVICE_TABLE(of, cx2072x_of_match);

#ifdef CONFIG_ACPI
static struct acpi_device_id cx2072x_acpi_match[] = {
	{ "14F10720", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, cx2072x_acpi_match);
#endif

static struct i2c_driver cx2072x_i2c_driver = {
	.probe = cx2072x_i2c_probe,
	.remove = cx2072x_i2c_remove,
	.shutdown = cx2072x_i2c_shutdown,
	.id_table = cx2072x_i2c_id,
	.driver = {
		.name = "cx2072x",
		.owner = THIS_MODULE,
		.of_match_table = cx2072x_of_match,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(cx2072x_acpi_match),
#endif
		.pm	= &cx2072x_pm_ops,
	},
};

module_i2c_driver(cx2072x_i2c_driver);

MODULE_DESCRIPTION("ASoC cx2072x Codec Driver");
MODULE_AUTHOR("Simon Ho <simon.ho@conexant.com>");
MODULE_LICENSE("GPL");
