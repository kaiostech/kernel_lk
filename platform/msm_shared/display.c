/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
#include <err.h>
#include <msm_panel.h>
#include <mdp4.h>
#include <mipi_dsi.h>
#include <boot_stats.h>

static struct msm_fb_panel_data *panel;


#define spi_panel_reset_gpio 25
#define spi_panel_cs_gpio 18
#define spi_panel_data_gpio 16
#define spi_panel_clk_gpio 19
#define spi_panel_dc_gpio 110
#define BIT(x)  (1<<(x))


extern int lvds_on(struct msm_fb_panel_data *pdata);

static int msm_fb_alloc(struct fbcon_config *fb)
{
	if (fb == NULL)
		return ERROR;

	if (fb->base == NULL)
		fb->base = memalign(4096, fb->width
							* fb->height
							* (fb->bpp / 8));

	if (fb->base == NULL)
		return ERROR;

	return NO_ERROR;
}

int msm_display_config()
{
	int ret = NO_ERROR;
	int mdp_rev;
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	pinfo = &(panel->panel_info);

	/* Set MDP revision */
	mdp_set_revision(panel->mdp_rev);

	switch (pinfo->type) {
	case LVDS_PANEL:
		dprintf(INFO, "Config LVDS_PANEL.\n");
		ret = mdp_lcdc_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Config MIPI_VIDEO_PANEL.\n");

		mdp_rev = mdp_get_revision();
		if (mdp_rev == MDP_REV_50 || mdp_rev == MDP_REV_304 ||
						mdp_rev == MDP_REV_305)
			ret = mdss_dsi_config(panel);
		else
			ret = mipi_config(panel);

		if (ret)
			goto msm_display_config_out;

		if (pinfo->early_config)
			ret = pinfo->early_config((void *)pinfo);

		ret = mdp_dsi_video_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Config MIPI_CMD_PANEL.\n");
		mdp_rev = mdp_get_revision();
		if (mdp_rev == MDP_REV_50 || mdp_rev == MDP_REV_304 ||
						mdp_rev == MDP_REV_305)
			ret = mdss_dsi_config(panel);
		else
			ret = mipi_config(panel);
		if (ret)
			goto msm_display_config_out;

		ret = mdp_dsi_cmd_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case LCDC_PANEL:
		dprintf(INFO, "Config LCDC PANEL.\n");
		ret = mdp_lcdc_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case HDMI_PANEL:
		dprintf(INFO, "Config HDMI PANEL.\n");
		ret = mdss_hdmi_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	case EDP_PANEL:
		dprintf(INFO, "Config EDP PANEL.\n");
		ret = mdp_edp_config(pinfo, &(panel->fb));
		if (ret)
			goto msm_display_config_out;
		break;
	default:
		return ERR_INVALID_ARGS;
	};

	if (pinfo->config)
		ret = pinfo->config((void *)pinfo);

msm_display_config_out:
	return ret;
}

int msm_display_on()
{
	int ret = NO_ERROR;
	int mdp_rev;
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	bs_set_timestamp(BS_SPLASH_SCREEN_DISPLAY);

	pinfo = &(panel->panel_info);

	if (pinfo->pre_on) {
		ret = pinfo->pre_on();
		if (ret)
			goto msm_display_on_out;
	}

	switch (pinfo->type) {
	case LVDS_PANEL:
		dprintf(INFO, "Turn on LVDS PANEL.\n");
		ret = mdp_lcdc_on(panel);
		if (ret)
			goto msm_display_on_out;
		ret = lvds_on(panel);
		if (ret)
			goto msm_display_on_out;
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Turn on MIPI_VIDEO_PANEL.\n");
		ret = mdp_dsi_video_on(pinfo);
		if (ret)
			goto msm_display_on_out;

		ret = mdss_dsi_post_on(panel);
		if (ret)
			goto msm_display_on_out;

		ret = mipi_dsi_on();
		if (ret)
			goto msm_display_on_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Turn on MIPI_CMD_PANEL.\n");
		ret = mdp_dma_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		mdp_rev = mdp_get_revision();
		if (mdp_rev != MDP_REV_50 && mdp_rev != MDP_REV_304 &&
						mdp_rev != MDP_REV_305) {
			ret = mipi_cmd_trigger();
			if (ret)
				goto msm_display_on_out;
		}

		ret = mdss_dsi_post_on(panel);
		if (ret)
			goto msm_display_on_out;

		break;
	case LCDC_PANEL:
		dprintf(INFO, "Turn on LCDC PANEL.\n");
		ret = mdp_lcdc_on(panel);
		if (ret)
			goto msm_display_on_out;
		break;
	case HDMI_PANEL:
		dprintf(INFO, "Turn on HDMI PANEL.\n");
		ret = mdss_hdmi_init();
		if (ret)
			goto msm_display_on_out;

		ret = mdss_hdmi_on();
		if (ret)
			goto msm_display_on_out;
		break;
	case EDP_PANEL:
		dprintf(INFO, "Turn on EDP PANEL.\n");
		ret = mdp_edp_on(pinfo);
		if (ret)
			goto msm_display_on_out;
		break;
	default:
		return ERR_INVALID_ARGS;
	};

	if (pinfo->on)
		ret = pinfo->on();

msm_display_on_out:
	return ret;
}
static int spi_write_byte(char data)
{
		
	 int i;  
	 for (i=7; i>=0; i--) {  
		 gpio_set(spi_panel_clk_gpio, 0);  
		 gpio_set(spi_panel_data_gpio, (((data>>i)&BIT(0))<<1));  
		// udelay(1);
		 gpio_set(spi_panel_clk_gpio, 2);
		// udelay(1);	   
	 }	
	return 0;
}
void mdss_spi_write (unsigned char* buf, int len)  
{  
	int i;	
	gpio_set(spi_panel_cs_gpio, 0);
	//udelay(1);
	for (i=0; i<len; i++)  
		spi_write_byte(buf[i]);  
	//udelay(1);  
	gpio_set(spi_panel_cs_gpio, 2);
}  

static int spidev_write_cmd(char cmd)
{
    char buf[2];

    buf[0] = cmd;

    gpio_set(spi_panel_dc_gpio, 0);
    mdss_spi_write(&buf[0], 1);
    gpio_set(spi_panel_dc_gpio, 2);

    return 0;
}

static int spidev_write_data(char data)
{
    char buf[2];
    buf[0] = data;
    gpio_set(spi_panel_dc_gpio, 2);

    mdss_spi_write(&buf[0], 1);

    return 0;

}


typedef struct gpio_pin{

	char    *pin_source;
	uint32_t pin_id;
	uint32_t pin_strength;
	uint32_t pin_direction;
	uint32_t pin_pull;
	uint32_t pin_state;
};

int mdss_spi_panel_init()
{
	gpio_tlmm_config(spi_panel_reset_gpio, 0,
			1, 0,
			3, 1);
	mdelay(1);
	gpio_tlmm_config(spi_panel_cs_gpio, 0,
			1, 1,
			2, 1);
	mdelay(1);
	gpio_tlmm_config(spi_panel_data_gpio, 0,
			1, 0,
			12, 1);
	mdelay(1);
	gpio_tlmm_config(spi_panel_clk_gpio, 0,
			1, 0,
			12, 1);
	mdelay(1);
	gpio_tlmm_config(spi_panel_dc_gpio, 0,
			1, 0,
			3, 1);
	
	mdelay(1);


	//gpio_set(reset_gpio.pin_id, 2);

#if 1
	gpio_set(spi_panel_reset_gpio, 2);
	gpio_set(spi_panel_dc_gpio, 1);
	gpio_set(spi_panel_reset_gpio, 2);
	mdelay(120);
	gpio_set(spi_panel_reset_gpio, 0);
	mdelay(120);
	gpio_set(spi_panel_reset_gpio, 2);
	mdelay(120);


	spidev_write_cmd(0xfe);
	spidev_write_cmd(0xef);
	spidev_write_cmd(0x36);
	spidev_write_data(0x48);
	spidev_write_cmd(0x3a);
	spidev_write_data(0x05);//RGB565
	//spidev_write_data(0x06);//RGB666
	spidev_write_cmd(0x35);
	spidev_write_data(0x00);
	//------------------------------end display control setting--------------------------------//
	//------------------------------Power Control Registers Initial------------------------------//
	spidev_write_cmd(0xa4);
	spidev_write_data(0x44);
	spidev_write_data(0x44);
	spidev_write_cmd(0xa5);
	spidev_write_data(0x42);
	spidev_write_data(0x42);
	spidev_write_cmd(0xaa);
	spidev_write_data(0x88);
	spidev_write_data(0x88);
	spidev_write_cmd(0xe8);
	//spidev_write_data(0x11);
	//spidev_write_data(0x0b);
	spidev_write_data(0x12);//54.1
	//spidev_write_data(0x13);//46.9
	spidev_write_data(0x40);
	spidev_write_cmd(0xe3);
	spidev_write_data(0x01);
	spidev_write_data(0x10);
	spidev_write_cmd(0xff);
	spidev_write_data(0x61);
	spidev_write_cmd(0xAC);
	spidev_write_data(0x00);
	spidev_write_cmd(0xa6);
	spidev_write_data(0x2a);
	spidev_write_data(0x2a);
	spidev_write_cmd(0xa7);
	spidev_write_data(0x2b);
	spidev_write_data(0x2b);
	spidev_write_cmd(0xa8);
	spidev_write_data(0x18);
	spidev_write_data(0x18);
	spidev_write_cmd(0xa9);
	spidev_write_data(0x2a);
	spidev_write_data(0x2a);
	spidev_write_cmd(0xad);
	spidev_write_data(0x33);
	spidev_write_cmd(0xaf);
	spidev_write_data(0x55);
	spidev_write_cmd(0xae);
	spidev_write_data(0x2b);
	//------------------------end Power Control Registers Initial------------------------------//
	//----------------------------display window 240X320------------------------------------//
	spidev_write_cmd(0x2a);
	spidev_write_data(0x00);
	spidev_write_data(0x00);
	spidev_write_data(0x00);
	spidev_write_data(0xef);
	spidev_write_cmd(0x2b);
	spidev_write_data(0x00);
	spidev_write_data(0x00);
	spidev_write_data(0x01);
	spidev_write_data(0x3f);
	spidev_write_cmd(0x2c);
	//----------------------------------end display window ----------------------------------------//
	//----------------------------------------gamma setting-----------------------------------------//
	spidev_write_cmd(0xf0);
	spidev_write_data(0x2);
	spidev_write_data(0x2);
	spidev_write_data(0x0);
	spidev_write_data(0x8);
	spidev_write_data(0xC);
	spidev_write_data(0x10);
	spidev_write_cmd(0xf1);
	spidev_write_data(0x1);
	spidev_write_data(0x0);
	spidev_write_data(0x0);
	spidev_write_data(0x14);
	spidev_write_data(0x1D);
	spidev_write_data(0xE);
	spidev_write_cmd(0xf2);
	spidev_write_data(0x10);
	spidev_write_data(0x9);
	spidev_write_data(0x37);
	spidev_write_data(0x4);
	spidev_write_data(0x4);
	spidev_write_data(0x48);
	spidev_write_cmd(0xf3);
	spidev_write_data(0x10);
	spidev_write_data(0xB);
	spidev_write_data(0x3F);
	spidev_write_data(0x5);
	spidev_write_data(0x5);
	spidev_write_data(0x4E);
	spidev_write_cmd(0xf4);
	spidev_write_data(0xD);
	spidev_write_data(0x19);
	spidev_write_data(0x17);
	spidev_write_data(0x1D);
	spidev_write_data(0x1E);
	spidev_write_data(0xF);
	spidev_write_cmd(0xf5);
	spidev_write_data(0x6);
	spidev_write_data(0x12);
	spidev_write_data(0x13);
	spidev_write_data(0x1A);
	spidev_write_data(0x1B);
	spidev_write_data(0xF);
	//------------------------------------end gamma setting-----------------------------------------//
	spidev_write_cmd(0x11);
	mdelay(120);
	spidev_write_cmd(0x29);
	spidev_write_cmd(0x2c);


#endif
	return 0;
}


int msm_display_init(struct msm_fb_panel_data *pdata)
{
	int ret = NO_ERROR;

	panel = pdata;
	if (!panel) {
		ret = ERR_INVALID_ARGS;
		goto msm_display_init_out;
	}

	/* Turn on panel */
	if (pdata->power_func)
		ret = pdata->power_func(1, &(panel->panel_info));

	if (ret)
		goto msm_display_init_out;

	{
		char *tx_buf1;
		int data_size = 240*320*2;
		tx_buf1 = malloc(data_size);

		mdss_spi_panel_init();

		mdelay(120);
		{
				int i,j,k;	 
				k = 0;
				for (i = 0; i < 320; i++) { 	   
					for (j = 0; j < 240; j++) { 		   
						tx_buf1[k] = 0xF8;
						tx_buf1[k+1] = 0x00;
						k = k+2;
					}	 
				}
		}
		mdss_spi_write(tx_buf1, data_size);
		free(tx_buf1);
	}


	/* Enable clock */
	if (pdata->clk_func)
		ret = pdata->clk_func(1);

	/* Only enabled for auto PLL calculation */
	if (pdata->pll_clk_func)
		ret = pdata->pll_clk_func(1, &(panel->panel_info));

	if (ret)
		goto msm_display_init_out;

	/* pinfo prepare  */
	if (pdata->panel_info.prepare) {
		/* this is for edp which pinfo derived from edid */
		ret = pdata->panel_info.prepare();
		panel->fb.width =  panel->panel_info.xres;
		panel->fb.height =  panel->panel_info.yres;
		panel->fb.stride =  panel->panel_info.xres;
		panel->fb.bpp =  panel->panel_info.bpp;
	}

	if (ret)
		goto msm_display_init_out;

	ret = msm_fb_alloc(&(panel->fb));
	if (ret)
		goto msm_display_init_out;

	fbcon_setup(&(panel->fb));
	display_image_on_screen();
	ret = msm_display_config();
	if (ret)
		goto msm_display_init_out;

	ret = msm_display_on();
	if (ret)
		goto msm_display_init_out;

	if (pdata->post_power_func)
		ret = pdata->post_power_func(1);
	if (ret)
		goto msm_display_init_out;

	/* Turn on backlight */
	if (pdata->bl_func)
		ret = pdata->bl_func(1);

	if (ret)
		goto msm_display_init_out;

msm_display_init_out:
	return ret;
}

int msm_display_off()
{
	int ret = NO_ERROR;
	struct msm_panel_info *pinfo;

	if (!panel)
		return ERR_INVALID_ARGS;

	pinfo = &(panel->panel_info);

	if (pinfo->pre_off) {
		ret = pinfo->pre_off();
		if (ret)
			goto msm_display_off_out;
	}

	switch (pinfo->type) {
	case LVDS_PANEL:
		dprintf(INFO, "Turn off LVDS PANEL.\n");
		mdp_lcdc_off();
		break;
	case MIPI_VIDEO_PANEL:
		dprintf(INFO, "Turn off MIPI_VIDEO_PANEL.\n");
		ret = mdp_dsi_video_off();
		if (ret)
			goto msm_display_off_out;
		ret = mipi_dsi_off(pinfo);
		if (ret)
			goto msm_display_off_out;
		break;
	case MIPI_CMD_PANEL:
		dprintf(INFO, "Turn off MIPI_CMD_PANEL.\n");
		ret = mdp_dsi_cmd_off();
		if (ret)
			goto msm_display_off_out;
		ret = mipi_dsi_off(pinfo);
		if (ret)
			goto msm_display_off_out;
		break;
	case LCDC_PANEL:
		dprintf(INFO, "Turn off LCDC PANEL.\n");
		mdp_lcdc_off();
		break;
	case EDP_PANEL:
		dprintf(INFO, "Turn off EDP PANEL.\n");
		ret = mdp_edp_off();
		if (ret)
			goto msm_display_off_out;
		break;
	default:
		return ERR_INVALID_ARGS;
	};

	if (target_cont_splash_screen()) {
		dprintf(INFO, "Continuous splash enabled, keeping panel alive.\n");
		return NO_ERROR;
	}

	if (panel->post_power_func)
		ret = panel->post_power_func(0);
	if (ret)
		goto msm_display_off_out;

	/* Turn off backlight */
	if (panel->bl_func)
		ret = panel->bl_func(0);

	if (pinfo->off)
		ret = pinfo->off();

	/* Disable clock */
	if (panel->clk_func)
		ret = panel->clk_func(0);

	/* Only for AUTO PLL calculation */
	if (panel->pll_clk_func)
		ret = panel->pll_clk_func(0, pinfo);

	if (ret)
		goto msm_display_off_out;

	/* Disable panel */
	if (panel->power_func)
		ret = panel->power_func(0, pinfo);

msm_display_off_out:
	return ret;
}
