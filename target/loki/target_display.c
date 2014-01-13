/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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
#include <smem.h>
#include <err.h>
#include <msm_panel.h>
#include <mipi_dsi.h>
#include <pm8x41.h>
#include <pm8x41_wled.h>
#include <board.h>
#include <mdp5.h>
#include <endian.h>
#include <platform/gpio.h>
#include <platform/clock.h>
#include <platform/iomap.h>
#include <target/display.h>
#include "include/panel.h"
#include "include/display_resource.h"

#define HFPLL_LDO_ID 12

#define GPIO_STATE_LOW 0
#define GPIO_STATE_HIGH 2
#define RESET_GPIO_SEQ_LEN 3

static struct msm_fb_panel_data panel;
static uint8_t display_enable;


extern int msm_display_init(struct msm_fb_panel_data *pdata);
extern int msm_display_off();
extern int loki_dsi_uniphy_pll_config(uint32_t ctl_base);


static int loki_mdss_dsi_panel_clock(uint8_t enable)
{
	uint32_t dual_dsi = panel.panel_info.mipi.dual_dsi;
	if (enable) {
		mdp_gdsc_ctrl(enable);
		mdp_clock_enable();
		loki_dsi_uniphy_pll_config(MIPI_DSI0_BASE);
		mmss_bus_clock_enable();
		mmss_dsi_clock_enable(DSI0_PHY_PLL_OUT, dual_dsi,0,0,0);
	} else if(!target_cont_splash_screen()) {
		// * Add here for continuous splash  *
	}

	return 0;
}

/* Pull DISP_RST_N high to get panel out of reset */
static void loki_mdss_mipi_panel_reset(uint8_t enable)
{
	uint8_t rst_gpio=96;
	uint8_t disp_en_gpio=59;
	uint8_t lcd_en_gpio=85;

	if (enable) {
		gpio_tlmm_config(rst_gpio, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_DISABLE);
		gpio_tlmm_config(disp_en_gpio, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_DISABLE);
		gpio_tlmm_config(lcd_en_gpio, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_DISABLE);

		gpio_set(rst_gpio, 0);
		mdelay(1);
		gpio_set(disp_en_gpio, 0);
		mdelay(10);
		gpio_set(disp_en_gpio, 2);
		mdelay(15);
		gpio_set(lcd_en_gpio, 2);
		mdelay(15);
		gpio_set(rst_gpio, 2);
		mdelay(15);
	} else {
		gpio_tlmm_config(rst_gpio, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
		gpio_tlmm_config(disp_en_gpio, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
		gpio_tlmm_config(lcd_en_gpio, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);

		gpio_set(rst_gpio, 0);
		gpio_set(disp_en_gpio, 0);
		gpio_set(lcd_en_gpio, 0);
	}
}

static int loki_mipi_panel_power(uint8_t enable)
{
	if (enable) {
		/* backlight enable is out of this */
		struct pm8x41_ldo ldo22 = LDO(PM8x41_LDO22, PLDO_TYPE);
		struct pm8x41_ldo ldo12 = LDO(PM8x41_LDO2, PLDO_TYPE);
		struct pm8x41_ldo ldo2 = LDO(PM8x41_LDO2, NLDO_TYPE);

		/* Turn on LDO8 for lcd1 mipi vdd */
		pm8x41_ldo_set_voltage(&ldo22, 3000000);
		pm8x41_ldo_control(&ldo22, enable);

		/* Turn on LDO23 for lcd1 mipi vddio */
		pm8x41_ldo_set_voltage(&ldo12, 1800000);
		pm8x41_ldo_control(&ldo12, enable);

		/* Turn on LDO2 for vdda_mipi_dsi */
		pm8x41_ldo_set_voltage(&ldo2, 1200000);
		pm8x41_ldo_control(&ldo2, enable);

		dprintf(INFO, " Panel Reset \n");
		/* Panel Reset */
	loki_mdss_mipi_panel_reset(enable);
		dprintf(INFO, " Panel Reset Done\n");
	} else {
		struct pm8x41_ldo ldo22 = LDO(PM8x41_LDO22, PLDO_TYPE);
		struct pm8x41_ldo ldo2 = LDO(PM8x41_LDO2, NLDO_TYPE);
		loki_mdss_mipi_panel_reset(enable);
		pm8x41_ldo_control(&ldo2, enable);
		pm8x41_ldo_control(&ldo22, enable);

	}
	return 0;
}
void display_init(void)
{
	uint32_t ret = 0;
        if (!display_enable)
        {
		mipi_jdi_video_wqxga_init(&(panel.panel_info));
		panel.clk_func = loki_mdss_dsi_panel_clock;
		panel.power_func = loki_mipi_panel_power;
		panel.fb.base = MIPI_FB_ADDR;
		panel.fb.width =  panel.panel_info.xres;
		panel.fb.height =  panel.panel_info.yres;
		panel.fb.stride =  panel.panel_info.xres;
		panel.fb.bpp =  panel.panel_info.bpp;
		panel.fb.format = FB_FORMAT_RGB888;
		panel.mdp_rev = MDP_REV_50;

		if (msm_display_init(&panel)) {
			dprintf(CRITICAL, "Display init failed!\n");
			return;
		}
		display_enable = 1;
        }
}

void display_shutdown(void)
{
	if (display_enable){
		if(!target_cont_splash_screen()) {
			//lp855x_bl_off();
		}
		msm_display_off();
	    }
}
