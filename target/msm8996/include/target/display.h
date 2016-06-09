/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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
#ifndef _TARGET_DISPLAY_H
#define _TARGET_DISPLAY_H

/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#include <display_resource.h>
#include <msm_panel.h>

/*---------------------------------------------------------------------------*/
/* Target Physical configuration                                             */
/*---------------------------------------------------------------------------*/

static const uint32_t panel_strength_ctrl[] = {
  0xFF, 0x06,
  0xFF, 0x06,
  0xFF, 0x06,
  0xFF, 0x06,
  0xFF, 0x00
};

static const uint32_t panel_regulator_settings[] = {
  0x1d, 0x1d, 0x1d, 0x1d, 0x1d
};

static const char panel_lane_config[] = {
  0x00, 0x00, 0x10, 0x0f,
  0x00, 0x00, 0x10, 0x0f,
  0x00, 0x00, 0x10, 0x0f,
  0x00, 0x00, 0x10, 0x0f,
  0x00, 0x00, 0x10, 0x8f,
};

static const char panel_bist_ctrl[] = { };

static const uint32_t panel_physical_ctrl[] = { };
/*---------------------------------------------------------------------------*/
/* Other Configuration                                                       */
/*---------------------------------------------------------------------------*/
#define DISPLAY_CMDLINE_PREFIX " mdss_mdp.panel="

#define MIPI_FB_ADDR  0x83401900
#define HDMI_FB_ADDR  0xB1C00000
#define KERNEL_TRIGGER_VALUE         0xFEFEFEFE
#define MDSS_SCRATCH_REG_0           0x00900014
#define MDSS_SCRATCH_REG_1           0x00900018

#define MIPI_HSYNC_PULSE_WIDTH       16
#define MIPI_HSYNC_BACK_PORCH_DCLK   32
#define MIPI_HSYNC_FRONT_PORCH_DCLK  76

#define MIPI_VSYNC_PULSE_WIDTH       2
#define MIPI_VSYNC_BACK_PORCH_LINES  2
#define MIPI_VSYNC_FRONT_PORCH_LINES 4

#define PWM_BL_LPG_CHAN_ID           4	/* lpg_out<3> */

#define HDMI_PANEL_NAME              "hdmi"
#define HDMI_CONTROLLER_STRING       "hdmi:"
#define HDMI_VIC_LEN                 5

#define NUM_TARGET_DISPLAYS          3
#define NUM_TARGET_LAYERS            10

#define NUM_VIG_PIPES                1
#define VIG_ID_START                 3
#define VIG_PIPE_START               7

#define NUM_RGB_PIPES                3
#define RGB_ID_START                 0
#define RGB_PIPE_START               0

#define NUM_DMA_PIPES                2
#define DMA_PIPE_START               8


/*---------------------------------------------------------------------------*/
/* Functions		                                                     */
/*---------------------------------------------------------------------------*/
int target_display_pre_on();
int target_display_pre_off();
int target_display_post_on();
int target_display_post_off();
int target_cont_splash_screen();
int target_display_get_base_offset(uint32_t base);
void target_force_cont_splash_disable(uint8_t override);
uint8_t target_panel_auto_detect_enabled();
void target_set_switch_gpio(int enable_dsi2hdmibridge);
int target_hdmi_pll_clock(uint8_t enable, struct msm_panel_info *pinfo);
int target_hdmi_panel_clock(uint8_t enable, struct msm_panel_info *pinfo);
int target_hdmi_regulator_ctrl(uint8_t enable);
int target_hdmi_gpio_ctrl(uint8_t enable);

enum LayerBufferFormat {
  /*
   * All RGB formats, Any new format will be added towards end of this group to maintain backward
   * compatibility.
   */
  kFormatRGB888,        //!< 8-bits Red, Green, Blue interleaved in RGB order. No Alpha.

  /* All YUV-Planar formats, Any new format will be added towards end of this group to maintain
     backward compatibility.
  */

  kFormatYCbCr422H2V1Packed = 0x300,  //!< Y-plane interleaved with horizontally subsampled U/V by  186
                                      //!< factor of 2  187
                                      //!<    y(0), u(0), y(1), v(0), y(2), u(2), y(3), v(2)  188
                                      //!<    y(n-1), u(n-1), y(n), v(n-1)


  kFormatInvalid = 0xFFFFFFFF,
};

enum pipe_type {
  VIG_TYPE,
  RGB_TYPE,
  DMA_TYPE,
};

struct target_display {
  uint32_t display_id;
  uint32_t width;
  uint32_t height;
  float    fps;
};

struct target_layer_int {
  uint32_t      layer_id;
  uint32_t      layer_type;
  bool            assigned;
  struct target_display *disp;
};

struct target_layer {
  void                *layer; /* layer pointer returned by assign */
  uint32_t            src_rect_x;
  uint32_t            src_rect_y;
  uint32_t            dst_rect_x;
  uint32_t            dst_rect_y;
  uint32_t            z_order;
  uint32_t            global_aplha;
  uint32_t            width;
  uint32_t            height;
  struct fbcon_config *fb;
};

struct target_display_update {
  void              *disp;
  struct target_layer *layer_list;
  uint32_t          num_layers;
};

/* DYNAMIC APIS */
void * target_display_open (uint32 display_id);
struct target_display * target_get_display_info(void *disp);
void *target_display_acquire_layer(struct target_display * disp, char *client_name, int color_format);
struct fbcon_config* target_display_get_fb(uint32_t disp_id);
int target_display_update(struct target_display_update * update, uint32_t size, uint32_t disp_id);
int target_release_layer(struct target_layer *layer);
int target_display_close(struct target_display * disp);
int target_get_max_display();
bool target_display_is_init_done();

#endif
