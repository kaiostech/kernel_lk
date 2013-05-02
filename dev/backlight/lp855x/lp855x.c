/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <bits.h>
#include <debug.h>
#include <reg.h>
#include <spmi.h>
#include <string.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <i2c_qup.h>
#include <blsp_qup.h>
#include <pm8x41_hw.h>
#include <lp855x.h>

#define LP8557_HW_I2C_ADDRESS       (0x2c)
#define LP8557_BACKLIGHT_EN_GPIO     57

static struct qup_i2c_dev *dev = NULL;

int lp855x_bl_init()
{
	uint8_t ret = 0;

	/* enable i2c bus pull up voltage (LVS1) */
	pm8x41_reg_write(0x18046 /* LVS1_EN_CTL1 */, 0x80);

	/* configure i2c bus */
	dev = qup_blsp_i2c_init(BLSP_ID_2, QUP_ID_5, 100000, 24000000);

	if (!dev) {
		dprintf(CRITICAL, "Failed to initialize backlight I2c\n");
		return 1;
	}

	/* configure en gpio */
	/* The DISABLE really should be ENABLE, however DISABLE is defined as 1 which is what we want */
	gpio_tlmm_config(LP8557_BACKLIGHT_EN_GPIO, 0 /*func*/, GPIO_OUTPUT, \
			GPIO_NO_PULL, GPIO_6MA, GPIO_DISABLE);

	/* Set output value to enable
	   here 2 should be 1, however 2 has to be used to match register definition */
	gpio_set(LP8557_BACKLIGHT_EN_GPIO, 2); 
	mdelay(20); /* this delay is taken from kernel */

	return ret;
}

uint8_t lp855x_bl_i2c_write(uint8_t addr, uint8_t val)
{
	uint8_t data_buf[] = { addr, val };

	/* Create a i2c_msg buffer, that is used to put the controller into write
	   mode and then to write some data. */
	struct i2c_msg msg_buf[] = {
		{LP8557_HW_I2C_ADDRESS, I2C_M_WR, 2, data_buf}
	};

	qup_i2c_xfer(dev, msg_buf, 1);

	return 0;
}

void lp855x_bl_on()
{
	uint8_t ret = 0;

	/* Init I2C for backlight init */
	ret = lp855x_bl_init();
	if ( ret != 0 ){
		dprintf(CRITICAL, "Failed turning on backlight\n");
	}

	lp855x_bl_i2c_write(LP8557_REG_CONFIG, 0x41); /* BRTHI/BRTLO control */
	lp855x_bl_i2c_write(LP8557_REG_LEDEN, 0xFF);  /* LEDEN */
	lp855x_bl_i2c_write(LP8557_REG_CMD, LP8557_BL_CMD_ON);
	lp855x_bl_i2c_write(LP8557_REG_BRTHI, 101); /* default brightness */
}


