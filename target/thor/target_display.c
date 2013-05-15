/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
#include <msm_panel.h>
#include <pm8x41.h>
#include <board.h>
#include <mdp5.h>
#include <platform/gpio.h>
#include <target/display.h>

static struct msm_fb_panel_data panel;
static uint8_t display_enable;

extern int msm_display_init(struct msm_fb_panel_data *pdata);
extern int msm_display_off();
extern int mdss_dsi_uniphy_pll_config(void);

static int thor_mdss_dsi_panel_clock(uint8_t enable)
{
	if (enable) {
		mdp_gdsc_ctrl(enable);
		mdp_clock_init();
		mdss_dsi_uniphy_pll_config();
		mmss_clock_init();
	} else if(!target_cont_splash_screen()) {
		// * Add here for continuous splash  *
	}

	return 0;
}

/* Pull DISP_RST_N high to get panel out of reset */
static void thor_mdss_mipi_panel_reset(void)
{
	struct pm8x41_gpio gpio19_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_CMOS,
		.out_strength = PM_GPIO_OUT_DRIVE_MED,
	};

	pm8x41_gpio_config(19, &gpio19_param);
	gpio_tlmm_config(58, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_DISABLE);

	pm8x41_gpio_set(19, PM_GPIO_FUNC_HIGH);
	mdelay(20);
	pm8x41_gpio_set(19, PM_GPIO_FUNC_LOW);
	mdelay(200);
	pm8x41_gpio_set(19, PM_GPIO_FUNC_HIGH);
	mdelay(20);
	gpio_set(58, 2);
	mdelay(100);
}

static int thor_mipi_panel_power(uint8_t enable)
{
	if (enable) {
		/* backlight enable is out of this */

		/* Turn on LDO8 for lcd1 mipi vdd */
		pm8x41_ldo_set_voltage("LDO22", 3000000);
		pm8x41_ldo_control("LDO22", enable);

		/* Turn on LDO23 for lcd1 mipi vddio */
		pm8x41_ldo_set_voltage("LDO12", 1800000);
		pm8x41_ldo_control("LDO12", enable);

		/* Turn on LDO2 for vdda_mipi_dsi */
		pm8x41_ldo_set_voltage("LDO2", 1200000);
		pm8x41_ldo_control("LDO2", enable);

		dprintf(INFO, " Panel Reset \n");
		/* Panel Reset */
		thor_mdss_mipi_panel_reset();
	}

	return 0;
}

void display_init(void)
{
	dprintf(INFO, "display_init()\n");

	mipi_novatek_video_1080p_init(&(panel.panel_info));

	panel.clk_func = thor_mdss_dsi_panel_clock;
	panel.power_func = thor_mipi_panel_power;
	panel.fb.base = MIPI_FB_ADDR;
	panel.fb.width =  panel.panel_info.xres;
	panel.fb.height =  panel.panel_info.yres;
	panel.fb.stride =  panel.panel_info.xres;
	panel.fb.bpp =  panel.panel_info.bpp;
	panel.fb.format = FB_FORMAT_RGB888;
	panel.mdp_rev = MDP_REV_50;
	dprintf(INFO, "panel.fb.width =%d. panel.fb.height = %d \n", panel.fb.width, panel.fb.height);

	if (msm_display_init(&panel)) {
		dprintf(CRITICAL, "Display init failed!\n");
		return;
	}

	display_enable = 1;
}

void display_shutdown(void)
{
	if (display_enable)
		msm_display_off();
}
