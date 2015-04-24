/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <err.h>
#include <i2c_qup.h>
#include <blsp_qup.h>
#include <target/lp55x1.h>
#include <target/init.h>




static void start_leds_hw_subtype_1(void)
{
	struct qup_i2c_dev  *dev;
	unsigned char ret[50];
	struct i2c_msg msg_buf[3];

	dev = qup_blsp_i2c_init(BLSP_ID_1, QUP_ID_1, 100000, 19200000);

	if (!dev) {
		dprintf(CRITICAL, "QM8626 Failed initializing I2c\n");
		return;
	}

/* Check if LP5521 is connected */

/* Resets all registers to default */
	ret[0] = LP5521_REG_RESET;
	ret[1] = 0xff;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

	mdelay(20);

/* Enable the LP5521 */
	ret[0] = LP5521_REG_ENABLE;
	ret[1] = LP5521_MASTER_ENABLE;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

		mdelay(20);

/*
 * Check that this LED device is LP5521 and not LP55231. The LP5521 Red LED
 * current register defaults to 0xAF and this is unique to the LP5521.
 * This register deafults to 0xFF in the LP55231
 */
	ret[0] = LP5521_REG_R_CURRENT;
	ret[10] = 0x00;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 1;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);
	msg_buf[1].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[1].flags = I2C_M_RD;
	msg_buf[1].len = 1;
	msg_buf[1].buf = (unsigned char*)(&ret[10]);

	if (qup_i2c_xfer(dev, msg_buf, 2) != 2)
		goto xferErr5521;

	if (ret[10] != LP5521_REG_R_CURR_DEFAULT)
		goto wrong_device;
	else
		dprintf(CRITICAL, "LP5521 Detected\n");


/* Set all PWMs to direct control mode */
	ret[0] = LP5521_REG_OP_MODE;
	ret[1] = LP5521_CMD_DIRECT;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

/* Configure LP5521 */
	ret[0] = LP5521_REG_CONFIG;
	ret[1] = (LP5521_PWRSAVE_EN | LP5521_CP_MODE_AUTO | LP5521_R_TO_BATT | LP5521_CLK_INT);
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

/* Modify BLUE LED current */
	ret[0] = LP5521_REG_B_CURRENT;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

/* Turn on BLUE LED PWM*/
	ret[0] = LP5521_REG_B_PWM;
	ret[1] = 0x02;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

/* Modify GREEN LED current */
	ret[0] = LP5521_REG_G_CURRENT;
	ret[1] = 0x03;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

/* Turn on GREEN LED PWM*/
	ret[0] = LP5521_REG_G_PWM;
	ret[1] = 0x02;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

	/* Modify RED LED current */
	ret[0] = LP5521_REG_R_CURRENT;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

	/* Turn on RED LED PWM*/
	ret[0] = LP5521_REG_R_PWM;
	ret[1] = 0x02;
	msg_buf[0].addr = LP5521_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr5521;

	return;

xferErr5521:
	dprintf(CRITICAL, "LP5521 I2C Communication Error\n");
	return;

wrong_device:
	dprintf(CRITICAL, "Wrong device for HW subtype 1\n");
	return;

}

static void start_leds_hw_subtype_4(void)
{
	struct qup_i2c_dev  *dev;
	unsigned char ret[50];
	struct i2c_msg msg_buf[3];

	dev = qup_blsp_i2c_init(BLSP_ID_1, QUP_ID_1, 100000, 19200000);

	if (!dev) {
		dprintf(CRITICAL, "Failed initializing I2c\n");
		return;
	}

/* Check if LP55231 is connected */

/* Reset all LP55231 registers to default */
	ret[0] = LP5523_REG_RESET;
	ret[1] = 0xff;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

	mdelay(20);

/* Enable the LP55231  */
	ret[0] = LP5523_REG_ENABLE;
	ret[1] = LP5523_ENABLE;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

/* Read back the Enable Register to check if the enable bit is set, if so we
 * have detected the LP55231
 */
	ret[0] = LP5523_REG_ENABLE;
	ret[10] = 0x00;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 1;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);
	msg_buf[1].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[1].flags = I2C_M_RD;
	msg_buf[1].len = 1;
	msg_buf[1].buf = (unsigned char*)(&ret[10]);

	if (qup_i2c_xfer(dev, msg_buf, 2) != 2)
		goto xferErr55231;

	if (ret[10] != LP5523_ENABLE)
		goto wrong_device;
	else
		dprintf(CRITICAL, "LP55231 Detected\n");

/* Configure LP55231 */
	ret[0] = LP5523_REG_CONFIG;
	ret[1] = ( LP5523_AUTO_INC | LP5523_PWR_SAVE |
			LP5523_CP_AUTO | LP5523_INT_CLK | LP5523_PWM_PWR_SAVE);
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

/* Turn on D1, D2, D3, D7, D8 and D9 */

/* Turn on D1 */
	ret[0] = LP5523_REG_LED_PWM_BASE;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

/* Turn on D2 */
	ret[0] = LP5523_REG_LED_PWM_BASE + 1;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

/* Turn on D3 */
	ret[0] = LP5523_REG_LED_PWM_BASE + 2;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

/* Turn on D7 */
	ret[0] = LP5523_REG_LED_PWM_BASE + 6;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

/* Turn on D8 */
	ret[0] = LP5523_REG_LED_PWM_BASE + 7;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

/* Turn on D9 */
	ret[0] = LP5523_REG_LED_PWM_BASE + 8;
	ret[1] = 0x0A;
	msg_buf[0].addr = LP55231_HW_I2C_ADDRESS;
	msg_buf[0].flags = I2C_M_WR;
	msg_buf[0].len = 2;
	msg_buf[0].buf = (unsigned char*)(&ret[0]);

	if (qup_i2c_xfer(dev, msg_buf, 1) != 1)
		goto xferErr55231;

	return;

xferErr55231:
	dprintf(CRITICAL, "LP55231 I2C Communication Error\n");
	return;

wrong_device:
	dprintf(CRITICAL, "Wrong device for HW subtype 4\n");
	return;

}

void lp55x1_start_leds(unsigned int hw_subtype)
{
	if (hw_subtype == QM8626_HW_PLATFORM_SUBTYPE_1)
		start_leds_hw_subtype_1();
	else if (hw_subtype == QM8626_HW_PLATFORM_SUBTYPE_4)
		start_leds_hw_subtype_4();
}
