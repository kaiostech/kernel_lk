/*
 * Copyright (c) 2012, Amazon.com, Inc. or its affiliates. All rights reserved.
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

typedef enum {
	FBGFX_IMAGE_FORMAT_RAW,
	FBGFX_IMAGE_FORMAT_RLE,
	FBGFX_IMAGE_FORMAT_GZIP /* Gzip of the raw image */
} FBGFX_IMAGE_FORMAT;
	
/*
 * rle_pixel_data must be in 1-byte RLE format, which can be exported from
 * (e.g.) GIMP and used with only minor (structure definition) changes.
 * expanded_pixel_data should be defined as 0 at compile time so that the image
 * will be properly decoded at run time.
 *
 * Alternately, the image may be statically defined unencoded, by filling out
 * expanded_pixel_data and leaving rle_pixel_data alone.
 */
struct fbgfx_image {
	unsigned int   width;
	unsigned int   height;
	unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
	FBGFX_IMAGE_FORMAT   format;
	unsigned char *expanded_pixel_data;
	unsigned char *pixel_data;
};

/* the font is 5x12; set this constant to scale up the font so it is easily
 * readable on the device.
 */
#ifndef FBGFX_FONT_SCALE
#define FBGFX_FONT_SCALE 3
#endif

/* these constants can be passed in to:
 *   fbgfx_scroll_up
 *   fbgfx_set_text_window
 *   fbgfx_apply_image
 */
#define FBGFX_CENTERED -1 /* for x and y */
#define FBGFX_MAX      -2 /* for width and height */

extern void fbgfx_init(); /* call first, but after fbcon is initialized */
extern void fbgfx_get_display_dimensions(unsigned int *width, unsigned int *height);
extern void fbgfx_get_background_color(unsigned int *rgb);
extern void fbgfx_set_background_color(unsigned int rgb);
extern void fbgfx_get_foreground_color(unsigned int *rgb);
extern void fbgfx_set_foreground_color(unsigned int rgb);
extern void fbgfx_clear(); /* set to background_color; default black */
extern void fbgfx_scroll_up(int lines /* in pixels */, int x, int y, int width, int height);

/* see README for image notes */
extern void fbgfx_preload_image(struct fbgfx_image *image);
extern void fbgfx_apply_image(struct fbgfx_image *image, int x, int y);

extern void fbgfx_get_font_dimensions(unsigned int *width, unsigned int *width_with_kern, unsigned int *height);

/* see README for text window notes */
extern void fbgfx_set_text_window(int x, int y, int width, int height);
extern void fbgfx_scroll_up_text_window(int lines /* in pixels */);
extern void fbgfx_get_text_cursor(unsigned int *row, unsigned int *col);
extern void fbgfx_set_text_cursor(unsigned int row, unsigned int col);
extern void fbgfx_printf(const char *format, ...);

/* see README for double-buffering notes */
extern void fbgfx_enable_double_buffer(); /* call before fbgfx_init */
extern void fbgfx_flip(); /* no-op if direct buffering */
