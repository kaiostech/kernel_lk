/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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

#ifndef _PANEL_HX8399A_1080P_VIDEO_H_

#define _PANEL_HX8399A_1080P_VIDEO_H_
/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#include "panel.h"

/*---------------------------------------------------------------------------*/
/* Panel configuration                                                       */
/*---------------------------------------------------------------------------*/
static struct panel_config hx8399a_1080p_video_panel_data = {
	"qcom,mdss_dsi_hx8399a_1080p_video", "dsi:0:", "qcom,mdss-dsi-panel",
	10, 0, "DISPLAY_1", 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 866400000, 0, 0, 0, NULL
};

/*---------------------------------------------------------------------------*/
/* Panel resolution                                                          */
/*---------------------------------------------------------------------------*/
static struct panel_resolution hx8399a_1080p_video_panel_res = {
	640, 480, 16, 48, 96, 0, 32, 10, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel color information                                                   */
/*---------------------------------------------------------------------------*/
static struct color_info hx8399a_1080p_video_color = {
	24, 0, 0xff, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel on/off command information                                          */
/*---------------------------------------------------------------------------*/
static char hx8399a_1080p_video_on_cmd14[] = {
	0x02, 0x00, 0x29, 0xC0,
	0x11, 0x00, 0xFF, 0xFF,
};

static char hx8399a_1080p_video_on_cmd15[] = {
	0x02, 0x00, 0x29, 0xC0,
	0x29, 0x00, 0xFF, 0xFF,
};

static struct mipi_dsi_cmd hx8399a_1080p_video_on_command[] = {
	{0x8, hx8399a_1080p_video_on_cmd14, 0x78},
	{0x8, hx8399a_1080p_video_on_cmd15, 0x0A}
};

#define HX8399A_1080P_VIDEO_ON_COMMAND 2

static char hx8399a_1080p_videooff_cmd0[] = {
	0x28, 0x00, 0x05, 0x80
};

static char hx8399a_1080p_videooff_cmd1[] = {
	0x10, 0x00, 0x05, 0x80
};

static struct mipi_dsi_cmd hx8399a_1080p_video_off_command[] = {
	{0x4, hx8399a_1080p_videooff_cmd0, 0x32},
	{0x4, hx8399a_1080p_videooff_cmd1, 0x78}
};

#define HX8399A_1080P_VIDEO_OFF_COMMAND 2


static struct command_state hx8399a_1080p_video_state = {
	0, 1
};

/*---------------------------------------------------------------------------*/
/* Command mode panel information                                            */
/*---------------------------------------------------------------------------*/
static struct commandpanel_info hx8399a_1080p_video_command_panel = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Video mode panel information                                              */
/*---------------------------------------------------------------------------*/
static struct videopanel_info hx8399a_1080p_video_video_panel = {
	0, 0, 0, 0, 1, 1, 1, 0, 0x9
};

/*---------------------------------------------------------------------------*/
/* Lane configuration                                                        */
/*---------------------------------------------------------------------------*/
static struct lane_configuration hx8399a_1080p_video_lane_config = {
	2, 0, 1, 1, 0, 0, 1
};
#if 0   //8916
/*---------------------------------------------------------------------------*/
/* Panel timing                                                              */
/*---------------------------------------------------------------------------*/
static const uint32_t hx8399a_1080p_video_timings[] = {
	0x76, 0x1A, 0x10, 0x00, 0x3C, 0x40, 0x14, 0x1C, 0x12, 0x03, 0x04, 0x00
};

static struct panel_timing hx8399a_1080p_video_timing_info = {
	0, 4, 0x1e, 0x38
};
#endif
/*---------------------------------------------------------------------------*/
/* Panel timing                                                              */
/*---------------------------------------------------------------------------*/
static const uint32_t hx8399a_1080p_video_timings[] = {
        0xE6, 0x38, 0x26, 0x00, 0x68, 0x6C, 0x2A, 0x3A, 0x2C, 0x03, 0x04, 0x00
};

static struct panel_timing hx8399a_1080p_video_timing_info = {
        0, 4, 0x02, 0x2B
};

/*---------------------------------------------------------------------------*/
/* Panel reset sequence                                                      */
/*---------------------------------------------------------------------------*/
static struct panel_reset_sequence hx8399a_1080p_video_reset_seq = {
	{1, 0, 1, }, {20, 2, 20, }, 2
};

/*---------------------------------------------------------------------------*/
/* Backlight setting                                                         */
/*---------------------------------------------------------------------------*/
static struct backlight hx8399a_1080p_video_backlight = {
	1, 1, 4095, 100, 1, "PMIC_8941"
};

#define HX8399A_1080P_VIDEO_SIGNATURE 0xA0000

#endif /*_PANEL_HX8399A_1080P_VIDEO_H_*/
