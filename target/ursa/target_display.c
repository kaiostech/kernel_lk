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
#include <pm8x41_wled.h>
#include <board.h>
#include <mdp5.h>
#include <platform/gpio.h>
#include <target/display.h>
#include "include/target/board.h"

static struct msm_fb_panel_data panel;
static uint8_t display_enable;

extern int msm_display_init(struct msm_fb_panel_data *pdata);
extern int msm_display_off();
extern int mdss_dsi_uniphy_pll_config(void);

static int ursa_backlight_on()
{
	static struct pm8x41_wled_data wled_ctrl = {
		.mod_scheme      = 0xC3,
		.led1_brightness = (0x0F << 8) | 0xEF,
		.led2_brightness = (0x0F << 8) | 0xEF,
		.led3_brightness = (0x0F << 8) | 0xEF,
		.max_duty_cycle  = 0x01,
	};

	pm8x41_wled_config(&wled_ctrl);
	pm8x41_wled_sink_control(1);
	pm8x41_wled_iled_sync_control(1);
	pm8x41_wled_enable(1);

	return 0;
}

static int ursa_mdss_dsi_panel_clock(uint8_t enable)
{
	if (enable) {
		mdp_gdsc_ctrl(enable);
		mdp_clock_init();
		mdss_dsi_uniphy_pll_config();
		mmss_clock_init();
	} else if(!target_cont_splash_screen()) {
		// * Add here for continuous splash  *
		dprintf(SPEW, "TODO: Disable display clocks if continuous splash is disabled\n");
	}

	return 0;
}

static int ursa_display_panel_power(uint8_t enable)
{
	struct pm8x41_gpio gpio19_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_CMOS,
		.out_strength = PM_GPIO_OUT_DRIVE_MED,
		.vin_sel = 2 //This GPIO is based on voltage source #2
	};

	if (enable) {
		/* Enable backlight */
		ursa_backlight_on();

		dprintf(SPEW, " Entering ursa_display_panel_power\n");

		//tA is the rise time of this regulator (TP_VDD) turning on
		dprintf(SPEW, " Setting LDO22\n");
		pm8x41_ldo_set_voltage("LDO22", 3150000); //vdd_vreg AKA vdd-supply
		pm8x41_ldo_control("LDO22", 1);
		udelay(10); //tB

		dprintf(SPEW, " Setting LDO12\n");
		//vdd_io_vreg == L12 == Core PLL power
		pm8x41_ldo_set_voltage("LDO12", 1800000);
		pm8x41_ldo_control("LDO12", 1);

		dprintf(SPEW, " Enabling LVS2\n");
		//vdd_io_panel_vreg
		pm8x41_reg_write(0x18146, 0x80);
		if (0x80 != pm8x41_reg_read(0x18146))
		{
			dprintf(CRITICAL, "Error enabling LVS2!  Did TrustZone block the write?\n");
			dprintf(CRITICAL, "Splash screen disabled!\n");

			//Don't want to risk turning L12 off, since it's a core PLL power rail for
			// a number of systems
			pm8x41_ldo_control("LDO22", 0);
			return;
		}
		dprintf(SPEW, " LVS2 enabled\n");

		dprintf(SPEW, " Setting LDO2\n");
		/* Turn on LDO2 for vdda_mipi_dsi */
		pm8x41_ldo_set_voltage("LDO2", 1200000);
		pm8x41_ldo_control("LDO2", 1);

		//tC is the rise time of the above regulators (IVDDI & co)

		//Touch reset line
		gpio_tlmm_config(60, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
		gpio_set(60, 2); //gpio_set(uint32_t gpio, uint32_t dir)
		mdelay(3); //tH, padded slightly to avoid needing udelay

		//Display reset line
		pm8x41_gpio_config(19, &gpio19_param);

		pm8x41_gpio_set(19, PM_GPIO_FUNC_HIGH);
		mdelay(10); //tD

		//+- 5.4V rails
		gpio_tlmm_config(58, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
		gpio_set(58, 2); //gpio_set(uint32_t gpio, uint32_t dir)
		mdelay(2); //tE + tF + tG
		mdelay(1); //tN

		pm8x41_gpio_set(19, PM_GPIO_FUNC_LOW);
		udelay(10);//tO
		pm8x41_gpio_set(19, PM_GPIO_FUNC_HIGH);
		mdelay(5); //tL

		pm8x41_gpio_set(19, PM_GPIO_FUNC_LOW);
		udelay(10); //tO
		pm8x41_gpio_set(19, PM_GPIO_FUNC_HIGH);
		mdelay(5); //tL

		dprintf(SPEW, " Panel Reset Done\n");
	}
	else
	{
		//Shut it down

		pm8x41_gpio_set(19, PM_GPIO_FUNC_LOW);
		mdelay(10); //tQ
		//wmb();

		gpio_set(58, 0);
		mdelay(3); //tR + tS + tT + tU
		//wmb();

		//VDDA
		pm8x41_ldo_control("LDO2", 0);

		//Here's where we would release L12, but
		// it's too risky to just cut that rail here since
		// it's used all over the system
		//VDD_IO (PLL power)
		//pm8x41_ldo_control("LDO12", 0);

		//Disable LVS2 (vdd_io_panel_vreg)
		pm8x41_reg_write(0x18146, 0);

		gpio_set(60, 0); //Touch reset GPIO
		mdelay(15); //tV + tW
// 		//tV (1.8V rail falling) is taking 12ms on our hardware

		//VDD (3.15V)
		pm8x41_ldo_control("LDO22", 0);

	}

	return 0;
}

void display_init(void)
{
	uint32_t hw_id = board_hardware_id();
	uint32_t soc_ver = board_soc_version();

// 	if (target_pause_for_battery_charge()) {
// 		/* no splash or display for charging mode */
// 		dprintf(INFO, "no splash or display for charging mode\n");
// 		return;
// 	}

	dprintf(INFO, "display_init(),target_id=%d.\n", hw_id);

	switch (hw_id) {
	case HW_PLATFORM_URSA_P0:
		dprintf(INFO, "Splash screen not supported on P0\n");
		return;
	default:
		dprintf(INFO, "Initializing URSA P1 display\n", hw_id);
		mipi_ursa_cmd_720p_init(&(panel.panel_info));
		panel.clk_func = ursa_mdss_dsi_panel_clock;
		panel.power_func = ursa_display_panel_power;
		panel.fb.base = MIPI_FB_ADDR;
		panel.fb.width =  panel.panel_info.xres;
		panel.fb.height =  panel.panel_info.yres;
		panel.fb.stride =  panel.panel_info.xres;
		panel.fb.bpp =  panel.panel_info.bpp;
		panel.fb.format = FB_FORMAT_RGB888;
		panel.mdp_rev = MDP_REV_50;
		break;
	};

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
