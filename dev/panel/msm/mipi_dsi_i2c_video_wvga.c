/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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
 *  * Neither the name of The Linux Foundation. nor the names of its
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


#include <msm_panel.h>
#include <mipi_dsi.h>

static struct msm_panel_info pinfo;


/* timing, phy and pll ctl */
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720x480, RGB888, 3 Lane 60 fps video mode */
	/* regulator n/a */
	{0x00, 0x0a, 0x04, 0x00},
	/* timing 0x440*/
	{0x49, 0x0D, 0x09, 0x00,
	 0x2C, 0x33, 0x0D, 0x11,
	 0x0E, 0x03, 0x04},
	/* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x00, 0x9f, 0x49, 0xd9,
	 0x00, 0x50, 0x48, 0x63,
	 0x03, 0x1f, 0x03, 0x00,
	 0x14, 0x03, 0x00, 0x02,
	 0x00, 0x20, 0x00, 0x01,
	 0x00},
};


int mipi_dsi_i2c_video_wvga_init(uint32_t pdest)
{
	int ret;

	pinfo.xres = 720;
	pinfo.yres = 480;

	pinfo.lcdc.xres_pad = 0;
	pinfo.lcdc.yres_pad = 0;

	pinfo.type = MIPI_VIDEO_PANEL;
	//pinfo.pdest = pdest;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 16;
	pinfo.lcdc.h_pulse_width = 62;
	pinfo.lcdc.v_back_porch = 30;
	pinfo.lcdc.v_front_porch = 9;
	pinfo.lcdc.v_pulse_width = 6;
	pinfo.lcdc.border_clr = 0; /* blk */
	pinfo.lcdc.underflow_clr = 0xff; /* blue */
	pinfo.lcdc.hsync_skew = 0;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = FALSE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	/*
	 * video_mode_data_ctrl 0x1c
	 * no config from test code
	 * DSI_RGB_SWAP_RGB
	 */
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;

	/*
	 * dsi_ctrl 0x0 = 0x177 and 0x175
	 * bit(6) = 1 dln2_en
	 * bit(5) = 1 dln1_en
	 * bit(4) = 1 dln0_en
	 */
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = FALSE;
	/*
	 * dsi1_clkout_timing_ctrl 0xc0 = 0x38
	 * bit(5:0) = 0x38 t_clk_pre
	 */
	pinfo.mipi.t_clk_post = 0x05;
	pinfo.mipi.t_clk_pre = 0x12;

	pinfo.mipi.stream = 0;
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;

	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;

	return ret;
}


