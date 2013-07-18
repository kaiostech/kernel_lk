/* Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
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
 *
 */
#include <hdmi.h>
#include <msm_panel.h>
#include <platform/timer.h>
#include <platform/clock.h>
#include <platform/iomap.h>

#define MDP4_OVERLAYPROC1_BASE  0x18000
#define MDP4_RGB_BASE           0x40000
#define MDP4_RGB_OFF            0x10000
#define DBC_START_OFFSET	4
#define SHORT_VIDEO_DESCRIPTOR	2
#define HDMI_MSM_BPP		24

/* all video formats defined by CEA 861D */
#define HDMI_VFRMT_UNKNOWN		0
#define HDMI_VFRMT_640x480p60_4_3	1
#define HDMI_VFRMT_720x480p60_4_3	2
#define HDMI_VFRMT_720x480p60_16_9	3
#define HDMI_VFRMT_1280x720p60_16_9	4
#define HDMI_VFRMT_1920x1080i60_16_9	5
#define HDMI_VFRMT_720x480i60_4_3	6
#define HDMI_VFRMT_1440x480i60_4_3	HDMI_VFRMT_720x480i60_4_3
#define HDMI_VFRMT_720x480i60_16_9	7
#define HDMI_VFRMT_1440x480i60_16_9	HDMI_VFRMT_720x480i60_16_9
#define HDMI_VFRMT_720x240p60_4_3	8
#define HDMI_VFRMT_1440x240p60_4_3	HDMI_VFRMT_720x240p60_4_3
#define HDMI_VFRMT_720x240p60_16_9	9
#define HDMI_VFRMT_1440x240p60_16_9	HDMI_VFRMT_720x240p60_16_9
#define HDMI_VFRMT_2880x480i60_4_3	10
#define HDMI_VFRMT_2880x480i60_16_9	11
#define HDMI_VFRMT_2880x240p60_4_3	12
#define HDMI_VFRMT_2880x240p60_16_9	13
#define HDMI_VFRMT_1440x480p60_4_3	14
#define HDMI_VFRMT_1440x480p60_16_9	15
#define HDMI_VFRMT_1920x1080p60_16_9	16
#define HDMI_VFRMT_720x576p50_4_3	17
#define HDMI_VFRMT_720x576p50_16_9	18
#define HDMI_VFRMT_1280x720p50_16_9	19
#define HDMI_VFRMT_1920x1080i50_16_9	20
#define HDMI_VFRMT_720x576i50_4_3	21
#define HDMI_VFRMT_1440x576i50_4_3	HDMI_VFRMT_720x576i50_4_3
#define HDMI_VFRMT_720x576i50_16_9	22
#define HDMI_VFRMT_1440x576i50_16_9	HDMI_VFRMT_720x576i50_16_9
#define HDMI_VFRMT_720x288p50_4_3	23
#define HDMI_VFRMT_1440x288p50_4_3	HDMI_VFRMT_720x288p50_4_3
#define HDMI_VFRMT_720x288p50_16_9	24
#define HDMI_VFRMT_1440x288p50_16_9	HDMI_VFRMT_720x288p50_16_9
#define HDMI_VFRMT_2880x576i50_4_3	25
#define HDMI_VFRMT_2880x576i50_16_9	26
#define HDMI_VFRMT_2880x288p50_4_3	27
#define HDMI_VFRMT_2880x288p50_16_9	28
#define HDMI_VFRMT_1440x576p50_4_3	29
#define HDMI_VFRMT_1440x576p50_16_9	30
#define HDMI_VFRMT_1920x1080p50_16_9	31
#define HDMI_VFRMT_1920x1080p24_16_9	32
#define HDMI_VFRMT_1920x1080p25_16_9	33
#define HDMI_VFRMT_1920x1080p30_16_9	34
#define HDMI_VFRMT_2880x480p60_4_3	35
#define HDMI_VFRMT_2880x480p60_16_9	36
#define HDMI_VFRMT_2880x576p50_4_3	37
#define HDMI_VFRMT_2880x576p50_16_9	38
#define HDMI_VFRMT_1920x1250i50_16_9	39
#define HDMI_VFRMT_1920x1080i100_16_9	40
#define HDMI_VFRMT_1280x720p100_16_9	41
#define HDMI_VFRMT_720x576p100_4_3	42
#define HDMI_VFRMT_720x576p100_16_9	43
#define HDMI_VFRMT_720x576i100_4_3	44
#define HDMI_VFRMT_1440x576i100_4_3	HDMI_VFRMT_720x576i100_4_3
#define HDMI_VFRMT_720x576i100_16_9	45
#define HDMI_VFRMT_1440x576i100_16_9	HDMI_VFRMT_720x576i100_16_9
#define HDMI_VFRMT_1920x1080i120_16_9	46
#define HDMI_VFRMT_1280x720p120_16_9	47
#define HDMI_VFRMT_720x480p120_4_3	48
#define HDMI_VFRMT_720x480p120_16_9	49
#define HDMI_VFRMT_720x480i120_4_3	50
#define HDMI_VFRMT_1440x480i120_4_3	HDMI_VFRMT_720x480i120_4_3
#define HDMI_VFRMT_720x480i120_16_9	51
#define HDMI_VFRMT_1440x480i120_16_9	HDMI_VFRMT_720x480i120_16_9
#define HDMI_VFRMT_720x576p200_4_3	52
#define HDMI_VFRMT_720x576p200_16_9	53
#define HDMI_VFRMT_720x576i200_4_3	54
#define HDMI_VFRMT_1440x576i200_4_3	HDMI_VFRMT_720x576i200_4_3
#define HDMI_VFRMT_720x576i200_16_9	55
#define HDMI_VFRMT_1440x576i200_16_9	HDMI_VFRMT_720x576i200_16_9
#define HDMI_VFRMT_720x480p240_4_3	56
#define HDMI_VFRMT_720x480p240_16_9	57
#define HDMI_VFRMT_720x480i240_4_3	58
#define HDMI_VFRMT_1440x480i240_4_3	HDMI_VFRMT_720x480i240_4_3
#define HDMI_VFRMT_720x480i240_16_9	59
#define HDMI_VFRMT_1440x480i240_16_9	HDMI_VFRMT_720x480i240_16_9
#define HDMI_VFRMT_1280x720p24_16_9	60
#define HDMI_VFRMT_1280x720p25_16_9	61
#define HDMI_VFRMT_1280x720p30_16_9	62
#define HDMI_VFRMT_1920x1080p120_16_9	63
#define HDMI_VFRMT_1920x1080p100_16_9	64
/* Video Identification Codes from 65-127 are reserved for the future */
#define HDMI_VFRMT_END			127
#define HDMI_VFRMT_MAX			(HDMI_VFRMT_END + 1)

struct msm_hdmi_mode_timing_info {
	uint32_t	video_format;
	uint32_t	active_h;
	uint32_t	front_porch_h;
	uint32_t	pulse_width_h;
	uint32_t	back_porch_h;
	uint32_t	active_low_h;
	uint32_t	active_v;
	uint32_t	front_porch_v;
	uint32_t	pulse_width_v;
	uint32_t	back_porch_v;
	uint32_t	active_low_v;
	/* Must divide by 1000 to get the actual frequency in MHZ */
	uint32_t	pixel_freq;
	/* Must divide by 1000 to get the actual frequency in HZ */
	uint32_t	refresh_rate;
	uint32_t	is_supported;
};

static const struct msm_hdmi_mode_timing_info timings_db[HDMI_VFRMT_END] =
{
	[HDMI_VFRMT_640x480p60_4_3] = {HDMI_VFRMT_640x480p60_4_3,
	    640, 16, 96, 48, true, 480, 10, 2, 33, true, 25200, 60000, true},

	[HDMI_VFRMT_720x480p60_4_3] = {HDMI_VFRMT_720x480p60_4_3,
	    720, 16, 62, 60, true, 480, 9, 6, 30, true, 27030, 60000, true},

	[HDMI_VFRMT_720x480p60_16_9] = {HDMI_VFRMT_720x480p60_16_9,
	    720, 16, 62, 60, true, 480, 9, 6, 30, true, 27030, 60000, true},

	[HDMI_VFRMT_1280x720p60_16_9] = {HDMI_VFRMT_1280x720p60_16_9,
	1280, 110, 40, 220, false, 720, 5, 5, 20, false, 74250, 60000, true},

	[HDMI_VFRMT_1920x1080i60_16_9] = {HDMI_VFRMT_1920x1080i60_16_9,
	    1920, 88, 44, 148, false, 540, 2, 5, 5, false, 74250, 60000, false},

	[HDMI_VFRMT_1440x480i60_4_3] = {HDMI_VFRMT_1440x480i60_4_3,
	    1440, 38, 124, 114, true, 240, 4, 3, 15, true, 27000, 60000, false},

	[HDMI_VFRMT_1440x480i60_16_9] = {HDMI_VFRMT_1440x480i60_16_9,
	    1440, 38, 124, 114, true, 240, 4, 3, 15, true, 27000, 60000, false},

	[HDMI_VFRMT_1920x1080p60_16_9] = {HDMI_VFRMT_1920x1080p60_16_9,
	1920, 88, 44, 148, false, 1080, 4, 5, 36, false, 148500, 60000, true},

	[HDMI_VFRMT_720x576p50_4_3] = {HDMI_VFRMT_720x576p50_4_3,
	    720, 12, 64, 68, true, 576,  5, 5, 39, true, 27000, 50000, true},

	[HDMI_VFRMT_720x576p50_16_9] = {HDMI_VFRMT_720x576p50_16_9,
	    720, 12, 64, 68, true, 576,  5, 5, 39, true, 27000, 50000, true},

	[HDMI_VFRMT_1280x720p50_16_9] = {HDMI_VFRMT_1280x720p50_16_9,
	1280, 440, 40, 220, false, 720, 5, 5, 20, false, 74250, 50000, true},

	[HDMI_VFRMT_1440x576i50_4_3] = {HDMI_VFRMT_1440x576i50_4_3,
	    1440, 24, 126, 138, true, 288, 2, 3, 19, true, 27000, 50000, false},

	[HDMI_VFRMT_1440x576i50_16_9] = {HDMI_VFRMT_1440x576i50_16_9,
	    1440, 24, 126, 138, true, 288, 2, 3, 19, true, 27000, 50000, false},

	[HDMI_VFRMT_1920x1080p50_16_9] = {HDMI_VFRMT_1920x1080p50_16_9,
	1920, 528, 44, 148, false, 1080, 4, 5, 36, false, 148500, 50000, true},

	[HDMI_VFRMT_1920x1080p24_16_9] = {HDMI_VFRMT_1920x1080p24_16_9,
	1920, 638, 44, 148, false, 1080, 4, 5, 36, false, 74250, 24000, true},

	[HDMI_VFRMT_1920x1080p25_16_9] = {HDMI_VFRMT_1920x1080p25_16_9,
	1920, 528, 44, 148, false, 1080, 4, 5, 36, false, 74250, 25000, true},

	[HDMI_VFRMT_1920x1080p30_16_9] = {HDMI_VFRMT_1920x1080p30_16_9,
	    1920, 88, 44, 148, false, 1080, 4, 5, 36, false, 74250, 30000, true}
};

static struct msm_hdmi_mode_timing_info hdmi_msm_res_timing;

static uint8_t hdmi_msm_avi_iframe_lut[][16] = {
/*	480p60	480i60	576p50	576i50	720p60	 720p50	1080p60	1080i60	1080p50
	1080i50	1080p24	1080p30	1080p25	640x480p 480p60_16_9 576p50_4_3 */
	{0x10,	0x10,	0x10,	0x10,	0x10,	 0x10,	0x10,	0x10,	0x10,
	 0x10,	0x10,	0x10,	0x10,	0x10, 0x10, 0x10}, /*00*/
	{0x18,	0x18,	0x28,	0x28,	0x28,	 0x28,	0x28,	0x28,	0x28,
	 0x28,	0x28,	0x28,	0x28,	0x18, 0x28, 0x18}, /*01*/
	{0x00,	0x04,	0x04,	0x04,	0x04,	 0x04,	0x04,	0x04,	0x04,
	 0x04,	0x04,	0x04,	0x04,	0x88, 0x00, 0x04}, /*02*/
	{0x02,	0x06,	0x11,	0x15,	0x04,	 0x13,	0x10,	0x05,	0x1F,
	 0x14,	0x20,	0x22,	0x21,	0x01, 0x03, 0x11}, /*03*/
	{0x00,	0x01,	0x00,	0x01,	0x00,	 0x00,	0x00,	0x00,	0x00,
	 0x00,	0x00,	0x00,	0x00,	0x00, 0x00, 0x00}, /*04*/
	{0x00,	0x00,	0x00,	0x00,	0x00,	 0x00,	0x00,	0x00,	0x00,
	 0x00,	0x00,	0x00,	0x00,	0x00, 0x00, 0x00}, /*05*/
	{0x00,	0x00,	0x00,	0x00,	0x00,	 0x00,	0x00,	0x00,	0x00,
	 0x00,	0x00,	0x00,	0x00,	0x00, 0x00, 0x00}, /*06*/
	{0xE1,	0xE1,	0x41,	0x41,	0xD1,	 0xd1,	0x39,	0x39,	0x39,
	 0x39,	0x39,	0x39,	0x39,	0xe1, 0xE1, 0x41}, /*07*/
	{0x01,	0x01,	0x02,	0x02,	0x02,	 0x02,	0x04,	0x04,	0x04,
	 0x04,	0x04,	0x04,	0x04,	0x01, 0x01, 0x02}, /*08*/
	{0x00,	0x00,	0x00,	0x00,	0x00,	 0x00,	0x00,	0x00,	0x00,
	 0x00,	0x00,	0x00,	0x00,	0x00, 0x00, 0x00}, /*09*/
	{0x00,	0x00,	0x00,	0x00,	0x00,	 0x00,	0x00,	0x00,	0x00,
	 0x00,	0x00,	0x00,	0x00,	0x00, 0x00, 0x00}, /*10*/
	{0xD1,	0xD1,	0xD1,	0xD1,	0x01,	 0x01,	0x81,	0x81,	0x81,
	 0x81,	0x81,	0x81,	0x81,	0x81, 0xD1, 0xD1}, /*11*/
	{0x02,	0x02,	0x02,	0x02,	0x05,	 0x05,	0x07,	0x07,	0x07,
	 0x07,	0x07,	0x07,	0x07,	0x02, 0x02, 0x02}  /*12*/
};

static int hdmi_msm_res_priority[HDMI_VFRMT_MAX] = {
	[HDMI_VFRMT_720x240p60_4_3]    = 1,
	[HDMI_VFRMT_1440x480i60_16_9]  = 2,
	[HDMI_VFRMT_1440x576i50_4_3]   = 3,
	[HDMI_VFRMT_1440x576i50_16_9]  = 4,
	[HDMI_VFRMT_1920x1080i60_16_9] = 5,
	[HDMI_VFRMT_640x480p60_4_3]    = 6,
	[HDMI_VFRMT_720x480p60_4_3]    = 7,
	[HDMI_VFRMT_720x480p60_16_9]   = 8,
	[HDMI_VFRMT_720x576p50_4_3]    = 9,
	[HDMI_VFRMT_720x576p50_16_9]   = 10,
	[HDMI_VFRMT_1280x720p50_16_9]  = 11,
	[HDMI_VFRMT_1280x720p60_16_9]  = 12,
	[HDMI_VFRMT_1920x1080p24_16_9] = 13,
	[HDMI_VFRMT_1920x1080p25_16_9] = 14,
	[HDMI_VFRMT_1920x1080p30_16_9] = 15,
	[HDMI_VFRMT_1920x1080p50_16_9] = 16,
	[HDMI_VFRMT_1920x1080p60_16_9] = 17,
};
void hdmi_msm_set_mode(int on)
{
	uint32_t val = 0;
	if (on) {
		val |= 0x00000003;
		writel(val, HDMI_CTRL);
	} else {
		val &= ~0x00000002;
		writel(val, HDMI_CTRL);
	}
}

static void *hdmi_addr_base = NULL;
void hdmi_set_fb_addr(void *addr)
{
	hdmi_addr_base = addr;
}

void hdmi_msm_read_edid(uint32_t dev_addr, uint32_t offset, uint8_t *data_buf,
			uint32_t data_len, int retry, const char *what)
{
	uint32_t reg_val, ndx;

	/* INIT DDC */
	writel((10 << 16) | (2 << 0),	MSM_HDMI_BASE + 0x0220);
	writel(0xff000000,		MSM_HDMI_BASE + 0x0224);
	writel((1 << 16) | (27 << 0),	MSM_HDMI_BASE + 0x027C);

again:

	/* Reset DDC interrupts */
	writel((1 << 2) | (1 << 1), MSM_HDMI_BASE + 0x0214);

	/* Ensure Device Address has LSB set to 0 to indicate Slave addr read */
	dev_addr &= 0xFE;

	/* DDC read configurations */
	writel((0x1UL << 31) | (dev_addr << 8),	MSM_HDMI_BASE + 0x0238);
	writel(offset << 8,			MSM_HDMI_BASE + 0x0238);
	writel((dev_addr | 1) << 8,		MSM_HDMI_BASE + 0x0238);
	writel((1 << 12) | (1 << 16),		MSM_HDMI_BASE + 0x0228);
	writel(1 | (1 << 12) | (1 << 13) | (data_len << 16),
						MSM_HDMI_BASE + 0x022C);

	/* Kick off DDC read */
	writel( (1 << 0) | (1 << 20), MSM_HDMI_BASE + 0x020C);

	/*Wait for 500ms for read to complete*/
	mdelay(500);

	/* Clear interrupts */
	writel(0x2, MSM_HDMI_BASE + 0x0214);

	/* Read DDC status */
	reg_val = readl(MSM_HDMI_BASE + 0x0218);
	reg_val &= 0x00001000 | 0x00002000 | 0x00004000 | 0x00008000;

	/* Check if any NACK occurred */
	if (reg_val) {
		writel(BIT(3), MSM_HDMI_BASE + 0x020C); /* SW_STATUS_RESET */
		if (retry == 1)
			writel(BIT(1), MSM_HDMI_BASE + 0x020C); /* SOFT_RESET */
		if (retry-- > 0) {
			dprintf(CRITICAL, "Error EDID read, retry %d\n", retry);
			mdelay(10);
			goto again;
		}

		dprintf(CRITICAL, "Error EDID read, stopping\n");

		return;
	}

	/* Write this data to DDC buffer */
	writel(0x1 | (3 << 16) | (1 << 31), MSM_HDMI_BASE + 0x0238);

	/* Discard first byte */
	readl(MSM_HDMI_BASE + 0x0238);

	for (ndx = 0; ndx < data_len; ++ndx) {
		reg_val = readl(MSM_HDMI_BASE + 0x0238);
		data_buf[ndx] = (uint8_t) ((reg_val & 0x0000FF00) >> 8);
	}

	dprintf(INFO, "%s read success\n", what);
	return;
}

const uint8_t *hdmi_edid_find_block(const uint8_t *in_buf,
		uint32_t start_offset, uint8_t type, uint8_t *len)
{
	/* the start of data block collection, start of Video Data Block */
	uint32_t offset = start_offset;
	uint32_t end_dbc_offset = in_buf[2];

	*len = 0;

	/*edid buffer 1, byte 2 being 4 means no non-DTD/Data block collection
	  present.
	  edid buffer 1, byte 2 being 0 menas no non-DTD/DATA block collection
	  present and no DTD data present.*/
	if ((end_dbc_offset == 0) || (end_dbc_offset == 4)) {
		dprintf(INFO, "EDID: no DTD or non-DTD data present\n");
		return NULL;
	}

	while (offset < end_dbc_offset) {
		uint8_t block_len = in_buf[offset] & 0x1F;
		if ((in_buf[offset] >> 5) == type) {
			*len = block_len;
			dprintf(SPEW,
				"EDID: block=%d found @ %d with length=%d\n",
				type, offset, block_len);
			return in_buf + offset;
		}
		offset += 1 + block_len;
	}
	dprintf(CRITICAL,
		"EDID: type=%d block not found in EDID block\n", type);

	return NULL;
}

void hdmi_update_panel_info(struct msm_fb_panel_data *pdata)
{
	int block 			= 1;
	int block_size			= 0x80;
	uint8_t edid_buf[0x80]		= {0};
	uint8_t *b			= edid_buf;
	const uint8_t *svd		= NULL;
	struct msm_panel_info *pinfo	= NULL;
	uint32_t preferred_format	= 0;
	uint32_t video_format;
	uint8_t len, i;
	int ndx;

	if (!pdata)
		return;

	pinfo = &pdata->panel_info;

	hdmi_msm_read_edid(0xA0, block * block_size, edid_buf,
			block_size, 5, "EDID");

	for (ndx = 0; ndx < block_size; ndx += 16)
		dprintf(SPEW, "EDID[%02x-%02x] %02x %02x %02x %02x  "
			"%02x %02x %02x %02x    %02x %02x %02x %02x  "
			"%02x %02x %02x %02x\n", ndx, ndx+15,
			b[ndx+0], b[ndx+1], b[ndx+2], b[ndx+3],
			b[ndx+4], b[ndx+5], b[ndx+6], b[ndx+7],
			b[ndx+8], b[ndx+9], b[ndx+10], b[ndx+11],
			b[ndx+12], b[ndx+13], b[ndx+14], b[ndx+15]);

	svd = hdmi_edid_find_block(edid_buf, DBC_START_OFFSET,
				   SHORT_VIDEO_DESCRIPTOR, &len);

	if (svd != NULL) {
		++svd;
		for (i = 0; i < len; ++i, ++svd) {
			video_format = (*svd & 0x7F);
			dprintf(SPEW, "video format=%d\n",
					video_format);
			if ((timings_db[video_format].is_supported)
				&& hdmi_msm_res_priority[video_format]) {
				if (hdmi_msm_res_priority[video_format] >
							preferred_format)
					preferred_format = video_format;
			}
		}
	}

	if (preferred_format) {
		struct msm_hdmi_mode_timing_info timing =
			timings_db[preferred_format];

		hdmi_msm_res_timing = timing;

		dprintf(INFO, "%s: preferred format=%d\n",
			__func__, hdmi_msm_res_timing.video_format);

		pinfo->xres     = hdmi_msm_res_timing.active_h;
		pinfo->yres     = hdmi_msm_res_timing.active_v;
		pinfo->clk_rate = hdmi_msm_res_timing.pixel_freq * 1000;

		pinfo->hdmi.h_back_porch  = hdmi_msm_res_timing.back_porch_h;
		pinfo->hdmi.h_front_porch = hdmi_msm_res_timing.front_porch_h;
		pinfo->hdmi.h_pulse_width = hdmi_msm_res_timing.pulse_width_h;
		pinfo->hdmi.v_back_porch  = hdmi_msm_res_timing.back_porch_v;
		pinfo->hdmi.v_front_porch = hdmi_msm_res_timing.front_porch_v;
		pinfo->hdmi.v_pulse_width = hdmi_msm_res_timing.pulse_width_v;

		pdata->fb.width  =  pinfo->xres;
		pdata->fb.height =  pinfo->yres;
		pdata->fb.stride =  pinfo->xres;
	}
}

void hdmi_msm_panel_init(struct msm_panel_info *pinfo)
{
	struct msm_hdmi_mode_timing_info timing =
		timings_db[HDMI_VFRMT_1920x1080p60_16_9];


	hdmi_msm_res_timing = timing;

	dprintf(INFO, "%s: default format=%d\n",
		__func__, hdmi_msm_res_timing.video_format);

	if (!pinfo)
		return;

	pinfo->xres     = hdmi_msm_res_timing.active_h;
	pinfo->yres     = hdmi_msm_res_timing.active_v;
	pinfo->clk_rate = hdmi_msm_res_timing.pixel_freq * 1000;
	pinfo->bpp      = HDMI_MSM_BPP;
	pinfo->type     = HDMI_PANEL;

	pinfo->hdmi.h_back_porch  = hdmi_msm_res_timing.back_porch_h;
	pinfo->hdmi.h_front_porch = hdmi_msm_res_timing.front_porch_h;
	pinfo->hdmi.h_pulse_width = hdmi_msm_res_timing.pulse_width_h;
	pinfo->hdmi.v_back_porch  = hdmi_msm_res_timing.back_porch_v;
	pinfo->hdmi.v_front_porch = hdmi_msm_res_timing.front_porch_v;
	pinfo->hdmi.v_pulse_width = hdmi_msm_res_timing.pulse_width_v;
}

void hdmi_frame_ctrl_reg()
{
	uint32_t hdmi_frame_ctrl;

	hdmi_frame_ctrl  = ((0 << 31) & 0x80000000);
	hdmi_frame_ctrl |= ((0 << 29) & 0x20000000);
	hdmi_frame_ctrl |= ((0 << 28) & 0x10000000);
	writel(hdmi_frame_ctrl, HDMI_FRAME_CTRL);
}

void hdmi_video_setup()
{
	uint32_t hsync_total = 0;
	uint32_t vsync_total = 0;
	uint32_t hsync_start = 0;
	uint32_t hsync_end = 0;
	uint32_t vsync_start = 0;
	uint32_t vsync_end = 0;
	uint32_t hvsync_total = 0;
	uint32_t hsync_active = 0;
	uint32_t vsync_active = 0;
	uint32_t hdmi_frame_ctrl = 0;
	uint32_t val;
	struct msm_hdmi_mode_timing_info *hdmi_timing = &hdmi_msm_res_timing;

	hsync_total = hdmi_timing->active_h + hdmi_timing->front_porch_h
	    + hdmi_timing->back_porch_h + hdmi_timing->pulse_width_h - 1;
	vsync_total = hdmi_timing->active_v + hdmi_timing->front_porch_v
	    + hdmi_timing->back_porch_v + hdmi_timing->pulse_width_v - 1;

	hvsync_total = (vsync_total << 16) & 0x0FFF0000;
	hvsync_total |= (hsync_total << 0) & 0x00000FFF;
	writel(hvsync_total, HDMI_TOTAL);

	hsync_start = hdmi_timing->back_porch_h + hdmi_timing->pulse_width_h;
	hsync_end = (hsync_total + 1) - hdmi_timing->front_porch_h;
	hsync_active = (hsync_end << 16) & 0x0FFF0000;
	hsync_active |= (hsync_start << 0) & 0x00000FFF;
	writel(hsync_active, HDMI_ACTIVE_HSYNC);

	vsync_start =
	    hdmi_timing->back_porch_v + hdmi_timing->pulse_width_v - 1;
	vsync_end = vsync_total - hdmi_timing->front_porch_v;
	vsync_active = (vsync_end << 16) & 0x0FFF0000;
	vsync_active |= (vsync_start << 0) & 0x00000FFF;
	writel(vsync_active, HDMI_ACTIVE_VSYNC);

	writel(0, HDMI_VSYNC_TOTAL_F2);
	writel(0, HDMI_VSYNC_ACTIVE_F2);
	hdmi_frame_ctrl_reg();
}

void hdmi_msm_avi_info_frame(void)
{
	/* two header + length + 13 data */
	uint8_t  aviInfoFrame[16];
	uint8_t  checksum;
	uint32_t sum;
	uint32_t regVal;
	uint8_t  i;
	uint8_t  mode;

	switch (hdmi_msm_res_timing.video_format) {
	case HDMI_VFRMT_720x480p60_4_3:
		mode = 0;
		break;
	case HDMI_VFRMT_720x480i60_16_9:
		mode = 1;
		break;
	case HDMI_VFRMT_720x576p50_16_9:
		mode = 2;
		break;
	case HDMI_VFRMT_720x576i50_16_9:
		mode = 3;
		break;
	case HDMI_VFRMT_1280x720p60_16_9:
		mode = 4;
		break;
	case HDMI_VFRMT_1280x720p50_16_9:
		mode = 5;
		break;
	case HDMI_VFRMT_1920x1080p60_16_9:
		mode = 6;
		break;
	case HDMI_VFRMT_1920x1080i60_16_9:
		mode = 7;
		break;
	case HDMI_VFRMT_1920x1080p50_16_9:
		mode = 8;
		break;
	case HDMI_VFRMT_1920x1080i50_16_9:
		mode = 9;
		break;
	case HDMI_VFRMT_1920x1080p24_16_9:
		mode = 10;
		break;
	case HDMI_VFRMT_1920x1080p30_16_9:
		mode = 11;
		break;
	case HDMI_VFRMT_1920x1080p25_16_9:
		mode = 12;
		break;
	case HDMI_VFRMT_640x480p60_4_3:
		mode = 13;
		break;
	case HDMI_VFRMT_720x480p60_16_9:
		mode = 14;
		break;
	case HDMI_VFRMT_720x576p50_4_3:
		mode = 15;
		break;
	default:
		dprintf("%s: mode %d not supported\n", __func__,
			hdmi_msm_res_timing.video_format);
		return;
	}


	/* InfoFrame Type = 82 */
	aviInfoFrame[0]  = 0x82;
	/* Version = 2 */
	aviInfoFrame[1]  = 2;
	/* Length of AVI InfoFrame = 13 */
	aviInfoFrame[2]  = 13;

	/* Data Byte 01: 0 Y1 Y0 A0 B1 B0 S1 S0 */
	aviInfoFrame[3]  = hdmi_msm_avi_iframe_lut[0][mode];

	/* Setting underscan bit */
	aviInfoFrame[3] |= 0x02;

	/* Data Byte 02: C1 C0 M1 M0 R3 R2 R1 R0 */
	aviInfoFrame[4]  = hdmi_msm_avi_iframe_lut[1][mode];
	/* Data Byte 03: ITC EC2 EC1 EC0 Q1 Q0 SC1 SC0 */
	aviInfoFrame[5]  = hdmi_msm_avi_iframe_lut[2][mode];
	/* Data Byte 04: 0 VIC6 VIC5 VIC4 VIC3 VIC2 VIC1 VIC0 */
	aviInfoFrame[6]  = hdmi_msm_avi_iframe_lut[3][mode];
	/* Data Byte 05: 0 0 0 0 PR3 PR2 PR1 PR0 */
	aviInfoFrame[7]  = hdmi_msm_avi_iframe_lut[4][mode];
	/* Data Byte 06: LSB Line No of End of Top Bar */
	aviInfoFrame[8]  = hdmi_msm_avi_iframe_lut[5][mode];
	/* Data Byte 07: MSB Line No of End of Top Bar */
	aviInfoFrame[9]  = hdmi_msm_avi_iframe_lut[6][mode];
	/* Data Byte 08: LSB Line No of Start of Bottom Bar */
	aviInfoFrame[10] = hdmi_msm_avi_iframe_lut[7][mode];
	/* Data Byte 09: MSB Line No of Start of Bottom Bar */
	aviInfoFrame[11] = hdmi_msm_avi_iframe_lut[8][mode];
	/* Data Byte 10: LSB Pixel Number of End of Left Bar */
	aviInfoFrame[12] = hdmi_msm_avi_iframe_lut[9][mode];
	/* Data Byte 11: MSB Pixel Number of End of Left Bar */
	aviInfoFrame[13] = hdmi_msm_avi_iframe_lut[10][mode];
	/* Data Byte 12: LSB Pixel Number of Start of Right Bar */
	aviInfoFrame[14] = hdmi_msm_avi_iframe_lut[11][mode];
	/* Data Byte 13: MSB Pixel Number of Start of Right Bar */
	aviInfoFrame[15] = hdmi_msm_avi_iframe_lut[12][mode];

	sum = 0;
	for (i = 0; i < 16; i++)
		sum += aviInfoFrame[i];
	sum &= 0xFF;
	sum = 256 - sum;
	checksum = (uint8_t) sum;

	regVal = aviInfoFrame[5];
	regVal = regVal << 8 | aviInfoFrame[4];
	regVal = regVal << 8 | aviInfoFrame[3];
	regVal = regVal << 8 | checksum;
	writel(regVal, MSM_HDMI_BASE + 0x006C);

	regVal = aviInfoFrame[9];
	regVal = regVal << 8 | aviInfoFrame[8];
	regVal = regVal << 8 | aviInfoFrame[7];
	regVal = regVal << 8 | aviInfoFrame[6];
	writel(regVal, MSM_HDMI_BASE + 0x0070);

	regVal = aviInfoFrame[13];
	regVal = regVal << 8 | aviInfoFrame[12];
	regVal = regVal << 8 | aviInfoFrame[11];
	regVal = regVal << 8 | aviInfoFrame[10];
	writel(regVal, MSM_HDMI_BASE + 0x0074);

	regVal = aviInfoFrame[1];
	regVal = regVal << 16 | aviInfoFrame[15];
	regVal = regVal << 8 | aviInfoFrame[14];
	writel(regVal, MSM_HDMI_BASE + 0x0078);

	/* INFOFRAME_CTRL0[0x002C] */
	/* 0x3 for AVI InfFrame enable (every frame) */
	writel(readl(0x002C) | 0x00000003L, MSM_HDMI_BASE + 0x002C);
}

void hdmi_app_clk_init(int on)
{
	uint32_t val = 0;
	if (on) {
		/* Enable hdmi apps clock */
		val = readl(MISC_CC2_REG);
		val = BIT(11);
		writel(val, MISC_CC2_REG);
		udelay(10);

		/* Enable hdmi master clock */
		val = readl(MMSS_AHB_EN_REG);
		val |= BIT(14);
		writel(val, MMSS_AHB_EN_REG);
		udelay(10);

		/* Enable hdmi slave clock */
		val = readl(MMSS_AHB_EN_REG);
		val |= BIT(4);
		writel(val, MMSS_AHB_EN_REG);
		udelay(10);
	} else {
		// Disable clocks
		val = readl(MISC_CC2_REG);
		val &= ~(BIT(11));
		writel(val, MISC_CC2_REG);
		udelay(10);
		val = readl(MMSS_AHB_EN_REG);
		val &= ~(BIT(14));
		writel(val, MMSS_AHB_EN_REG);
		udelay(10);
		val = readl(MMSS_AHB_EN_REG);
		val &= ~(BIT(4));
		writel(val, MMSS_AHB_EN_REG);
		udelay(10);
	}
}

int hdmi_msm_turn_on(void)
{
	uint32_t hotplug_control;

	hdmi_msm_set_mode(0);

	hdmi_msm_reset_core();	// Reset the core
	hdmi_msm_init_phy();

	// Enable USEC REF timer
	writel(0x0001001B, HDMI_USEC_REFTIMER);

	// Write 1 to HDMI_CTRL to enable HDMI
	hdmi_msm_set_mode(1);

	// Video setup for HDMI
	hdmi_video_setup();

	// AVI info setup
	hdmi_msm_avi_info_frame();

	return 0;
}

int hdmi_dtv_init()
{
	uint32_t hsync_period;
	uint32_t hsync_ctrl;
	uint32_t hsync_start_x;
	uint32_t hsync_end_x;
	uint32_t display_hctl;
	uint32_t vsync_period;
	uint32_t display_v_start;
	uint32_t display_v_end;
	uint32_t hsync_polarity;
	uint32_t vsync_polarity;
	uint32_t data_en_polarity;
	uint32_t ctrl_polarity;
	uint32_t dtv_border_clr = 0;
	uint32_t dtv_underflow_clr = 0xff;
	uint32_t active_v_start = 0;
	uint32_t active_v_end = 0;
	uint32_t dtv_hsync_skew = 0;
	uint32_t intf, stage, snum, mask, data;
	unsigned char *rgb_base;
	unsigned char *overlay_base;
	uint32_t val;

	struct msm_hdmi_mode_timing_info *timing = &hdmi_msm_res_timing;

	// MDP E config
	writel((unsigned)hdmi_addr_base, MDP_BASE + 0xb0008);	//FB Address
	writel(((timing->active_v << 16) | timing->active_h),
		MDP_BASE + 0xb0004);
	writel((timing->active_h * HDMI_MSM_BPP / 8), MDP_BASE + 0xb000c);
	writel(0, MDP_BASE + 0xb0010);

	writel(DMA_PACK_PATTERN_RGB | DMA_DSTC0G_8BITS | DMA_DSTC1B_8BITS |
	       DMA_DSTC2R_8BITS, MDP_BASE + 0xb0000);
	writel(0xff0000, MDP_BASE + 0xb0070);
	writel(0xff0000, MDP_BASE + 0xb0074);
	writel(0xff0000, MDP_BASE + 0xb0078);

	// overlay rgb setup RGB2
	rgb_base = MDP_BASE + MDP4_RGB_BASE;
	rgb_base += (MDP4_RGB_OFF * 1);
	writel(((timing->active_v << 16) | timing->active_h), rgb_base + 0x0000);
	writel(0x0, rgb_base + 0x0004);
	writel(((timing->active_v << 16) | timing->active_h), rgb_base + 0x0008);
	writel(0x0, rgb_base + 0x000c);
	writel(hdmi_addr_base, rgb_base + 0x0010);	//FB address
	writel((timing->active_h * HDMI_MSM_BPP / 8), rgb_base + 0x0040);
	writel(0x2443F, rgb_base + 0x0050);	//format
	writel(0x20001, rgb_base + 0x0054);	//pattern
	writel(0x0, rgb_base + 0x0058);
	writel(0x20000000, rgb_base + 0x005c);	//phaseX
	writel(0x20000000, rgb_base + 0x0060);	// phaseY

	// mdp4 mixer setup MDP4_MIXER1
	data = readl(MDP_BASE + 0x10100);
	stage = 9;
	snum = 12;
	mask = 0x0f;
	mask <<= snum;
	stage <<= snum;
	data &= ~mask;
	data |= stage;
	writel(data, MDP_BASE + 0x10100);	// Overlay CFG conf
	data = readl(MDP_BASE + 0x10100);

	// Overlay cfg
	overlay_base = MDP_BASE + MDP4_OVERLAYPROC1_BASE;

	writel(0x0, MDP_BASE + 0x0038);	//EXternal interface select

	data = ((timing->active_v << 16) | timing->active_h);
	writel(data, overlay_base + 0x0008);
	writel(hdmi_addr_base, overlay_base + 0x000c);
	writel((timing->active_h * HDMI_MSM_BPP / 8), overlay_base + 0x0010);
	writel(0x10, overlay_base + 0x104);
	writel(0x10, overlay_base + 0x124);
	writel(0x10, overlay_base + 0x144);
	writel(0x01, overlay_base + 0x0004);	/* directout */

	hsync_period =
	    timing->pulse_width_h + timing->back_porch_h + timing->active_h +
	    timing->front_porch_h;
	hsync_ctrl = (hsync_period << 16) | timing->pulse_width_h;
	hsync_start_x = timing->pulse_width_h + timing->back_porch_h;
	hsync_end_x = hsync_period - timing->front_porch_h - 1;
	display_hctl = (hsync_end_x << 16) | hsync_start_x;

	vsync_period =
	    (timing->pulse_width_v + timing->back_porch_v + timing->active_v +
	     timing->front_porch_v) * hsync_period;
	display_v_start =
	    (timing->pulse_width_v + timing->back_porch_v) * hsync_period;
	display_v_end =
	    vsync_period - (timing->front_porch_v * hsync_period) - 1;

	dtv_underflow_clr |= 0x80000000;
	hsync_polarity     = 0;
	vsync_polarity     = 0;
	data_en_polarity   = 0;
	ctrl_polarity      =
	    (data_en_polarity << 2) | (vsync_polarity << 1) | (hsync_polarity);

	writel(hsync_ctrl,		MDP_BASE + DTV_BASE + 0x4);
	writel(vsync_period,		MDP_BASE + DTV_BASE + 0x8);
	writel(timing->pulse_width_v * hsync_period,
					MDP_BASE + DTV_BASE + 0xc);
	writel(display_hctl,		MDP_BASE + DTV_BASE + 0x18);
	writel(display_v_start,		MDP_BASE + DTV_BASE + 0x1c);
	writel(display_v_end,		MDP_BASE + DTV_BASE + 0x20);
	writel(dtv_border_clr,		MDP_BASE + DTV_BASE + 0x40);
	writel(dtv_underflow_clr,	MDP_BASE + DTV_BASE + 0x44);
	writel(dtv_hsync_skew,		MDP_BASE + DTV_BASE + 0x48);
	writel(ctrl_polarity,		MDP_BASE + DTV_BASE + 0x50);
	writel(0x0,			MDP_BASE + DTV_BASE + 0x2c);
	writel(active_v_start,		MDP_BASE + DTV_BASE + 0x30);
	writel(active_v_end,		MDP_BASE + DTV_BASE + 0x38);

	/* Enable DTV block */
	writel(0x01, MDP_BASE + DTV_BASE);

	/* Flush mixer/pipes configurations */
	val = BIT(1);
	val |= BIT(5);
	writel(val, MDP_BASE + 0x18000);

	return 0;
}

void hdmi_display_shutdown()
{
	writel(0x0, MDP_BASE + DTV_BASE);
	writel(0x8, MDP_BASE + 0x0038);
	writel(0x0, MDP_BASE + 0x10100);
}
