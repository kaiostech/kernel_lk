/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <debug.h>
#include <reg.h>
#include <platform/iomap.h>
#include <platform/gpio.h>
#include <gsbi.h>
#include <blsp_qup.h>

void gpio_tlmm_config(uint32_t gpio, uint8_t func,
		      uint8_t dir, uint8_t pull,
		      uint8_t drvstr, uint32_t enable)
{
	uint32_t val = 0;
	val |= pull;
	val |= func << 2;
	val |= drvstr << 6;
	val |= enable << 9;
	writel(val, (unsigned int *)GPIO_CONFIG_ADDR(gpio));
	return;
}

void gpio_set(uint32_t gpio, uint32_t dir)
{
	writel(dir, (unsigned int *)GPIO_IN_OUT_ADDR(gpio));
	return;
}

int gpio_get(uint32_t gpio)
{
	return (readl((unsigned int *)GPIO_IN_OUT_ADDR(gpio)) & 1);
}

/* Configure gpio for blsp uart 2 */
void gpio_config_uart_dm(uint8_t id)
{
	if (id == 1)
	{
		/* configure rx gpio */
		gpio_tlmm_config(5, 2, GPIO_INPUT, GPIO_NO_PULL,
					GPIO_8MA, GPIO_DISABLE);

		/* configure tx gpio */
		gpio_tlmm_config(4, 2, GPIO_OUTPUT, GPIO_NO_PULL,
					GPIO_8MA, GPIO_DISABLE);
	}
	else if (id == 7)
	{
		/* configure rx gpio */
		gpio_tlmm_config(46, 2, GPIO_INPUT, GPIO_NO_PULL,
					GPIO_8MA, GPIO_DISABLE);

		/* configure tx gpio */
		gpio_tlmm_config(45, 2, GPIO_OUTPUT, GPIO_NO_PULL,
					GPIO_8MA, GPIO_DISABLE);
	}
	else
	{
		dprintf(CRITICAL, "invalid uart id = %d\n", id);
	}
}

void gpio_config_blsp_i2c(uint8_t blsp_id, uint8_t qup_id)
{
	if (blsp_id == BLSP_ID_2) {
		switch (qup_id) {
		case QUP_ID_4:
			gpio_tlmm_config(83, 3, GPIO_OUTPUT, GPIO_NO_PULL,
						GPIO_6MA, GPIO_DISABLE);
			gpio_tlmm_config(84, 3, GPIO_OUTPUT, GPIO_NO_PULL,
						GPIO_6MA, GPIO_DISABLE);
		break;

		case QUP_ID_5:
			gpio_tlmm_config(87, 3, GPIO_OUTPUT, GPIO_NO_PULL,
						GPIO_6MA, GPIO_DISABLE);
			gpio_tlmm_config(88, 3, GPIO_OUTPUT, GPIO_NO_PULL,
						GPIO_6MA, GPIO_DISABLE);
		break;

		default:
			dprintf(CRITICAL, "Configure gpios for QUP instance: %u\n",
					  qup_id);
			ASSERT(0);
		};
	}
	else if (blsp_id == BLSP_ID_1) {
		switch (qup_id) {
		default:
			dprintf(CRITICAL, "Configure gpios for QUP instance: %u\n",
					   qup_id);
			ASSERT(0);
		};
	}
}

void tlmm_set_val(uint32_t off, uint8_t val)
{
	uint32_t reg_val;
	uint8_t mask = 0x7;

	reg_val = readl(SDC1_HDRV_PULL_CTL);

	reg_val &= ~(mask << off);

	reg_val |= (val << off);

	writel(reg_val, SDC1_HDRV_PULL_CTL);
}

void tlmm_set_hdrive(uint8_t dat_val, uint8_t cmd_val, uint8_t clk_val)
{
	tlmm_set_val(SDC1_DATA_HDRV_CTL_OFF, dat_val);
	tlmm_set_val(SDC1_CMD_HDRV_CTL_OFF, cmd_val);
	tlmm_set_val(SDC1_CLK_HDRV_CTL_OFF, clk_val);
}

void tlmm_set_pull(uint8_t dat_val, uint8_t cmd_val, uint8_t clk_val)
{
	tlmm_set_val(SDC1_DATA_PULL_CTL_OFF, dat_val);
	tlmm_set_val(SDC1_CMD_PULL_CTL_OFF, cmd_val);
	tlmm_set_val(SDC1_CLK_PULL_CTL_OFF, clk_val);
}
