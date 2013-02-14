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
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
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

#include <debug.h>
#include <err.h>
#include <stdlib.h>
#include <dev/fbcon.h>
#include <dev/fbgfx.h>
#include <platform.h>
#include <string.h>

#include "font.h"

/* tools for building an image and transferring to the frame buffer */
static int buffer_width = 0;
static int buffer_height = 0;
static int buffer_bytes_per_pixel = 0;
static int buffer_size = 0;
static int double_buffer_enabled = 0;
static unsigned char *frame_buffer = NULL;
static unsigned char *target_buffer = NULL;
static unsigned int dirt_begin_offset = 0;
static unsigned int dirt_end_offset = 0;

static unsigned int bg_color = 0x000000;
static unsigned int fg_color = 0xffffff;

static inline void fbgfx_put_rgb_pixels(unsigned char *buffer, unsigned int color, int count)
{
	for (; count > 0; count--) {
		buffer[0] = (char)((color      ) & 0xff);
		buffer[1] = (char)((color >>  8) & 0xff);
		buffer[2] = (char)((color >> 16) & 0xff);
		buffer += 3;
	}
}

#define PIXELS_TO_BYTES(_pixels_)      ((_pixels_) * buffer_bytes_per_pixel)
#define LINES_TO_BYTES(_lines_)        (PIXELS_TO_BYTES(_lines_) * buffer_width)
#define COORDINATES_TO_BYTES(_x_, _y_) (PIXELS_TO_BYTES(_x_) + LINES_TO_BYTES(_y_))

static int text_x = 0;
static int text_y = 0;
static int text_width = 0;
static int text_height = 0;
static int cursor_x = 0;
static int cursor_y = 0;

static void fbgfx_clear_dirt()
{
	if (double_buffer_enabled == 0) {
		return;
	}

	dirt_begin_offset = 0xffffffff;
	dirt_end_offset = 0;
}

static void fbgfx_add_dirt(int x, int y, int width, int height)
{
	unsigned int new_dirt;

	if (double_buffer_enabled == 0) {
		return;
	}

	new_dirt = COORDINATES_TO_BYTES(x, y);
	if (dirt_begin_offset > new_dirt) {
		dirt_begin_offset = new_dirt;
	}

	new_dirt = COORDINATES_TO_BYTES(x + width - 1, y + height - 1) + PIXELS_TO_BYTES(1);
	if (dirt_end_offset < new_dirt) {
		dirt_end_offset = new_dirt;
	}
}

void fbgfx_init()
{
	struct fbcon_config *fb;

	if (target_buffer != NULL) {
		return;
	}

	fb = fbcon_display();
	if (fb == NULL) {
		dprintf(CRITICAL, "%s: fbcon driver not initialized properly.\n", __func__);
		return;
	}

	if (fb->bpp != 24) {
		dprintf(CRITICAL, "%s: fbcon driver initialized with %d bpp (not supported).\n", __func__, fb->bpp);
		return;
	}

	buffer_width = fb->width;
	buffer_height = fb->height;
	buffer_bytes_per_pixel = fb->bpp / 8; // only 24 supported
	buffer_size = buffer_width * buffer_height * buffer_bytes_per_pixel;
	frame_buffer = fb->base;
	if (double_buffer_enabled) {
		target_buffer = (unsigned char *)memalign(4096, buffer_size);
		if (target_buffer == NULL) {
			dprintf(CRITICAL, "%s: failed to allocate a double buffer.\n", __func__);
			double_buffer_enabled = false;
			target_buffer = frame_buffer;
		} else {
			fbgfx_clear_dirt();
		}
	} else {
		target_buffer = frame_buffer;
	}

	text_x = 0;
	text_y = 0;
	text_width = buffer_width;
	text_height = buffer_height;

	fbcon_clear();
	fbgfx_clear();
}

void fbgfx_get_display_dimensions(unsigned int *width, unsigned int *height)
{
	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	if (width) {
		*width = buffer_width;
	}

	if (height) {
		*height = buffer_height;
	}
}

void fbgfx_get_background_color(unsigned int *rgb)
{
	if (rgb) {
		*rgb = bg_color;
	}
}

void fbgfx_set_background_color(unsigned int rgb)
{
	bg_color = rgb;
}

void fbgfx_get_foreground_color(unsigned int *rgb)
{
	if (rgb) {
		*rgb = fg_color;
	}
}

void fbgfx_set_foreground_color(unsigned int rgb)
{
	fg_color = rgb;
}

void fbgfx_clear()
{
	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	if (((bg_color      ) & 0xff) == ((bg_color >>  8) & 0xff) &&
	    ((bg_color      ) & 0xff) == ((bg_color >> 16) & 0xff)) {
		memset(target_buffer, bg_color & 0xff, buffer_size);
	} else {
		fbgfx_put_rgb_pixels(target_buffer, bg_color, buffer_width * buffer_height);
	}

	fbgfx_add_dirt(0, 0, buffer_width, buffer_height);
}

void fbgfx_scroll_up(int lines, int x, int y, int width, int height)
{
	unsigned char *source, *target;
	int retained_lines;

	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	if (lines < 1) {
		return;
	}
	if (x < 0 || x >= buffer_width) {
		return;
	}
	if (y < 0 || y >= buffer_height) {
		return;
	}

	if (width == FBGFX_MAX) {
		width = buffer_width - x;
	} else if (width < 1) {
		return;
	}

	if (height == FBGFX_MAX) {
		height = buffer_height - y;
	} else if (height < 1) {
		return;
	}

	if (x + width > buffer_width) {
		return;
	}
	if (y + height > buffer_height) {
		return;
	}

	if (lines > height) {
		lines = height;
	}
	retained_lines = height - lines;

	source = target_buffer + COORDINATES_TO_BYTES(x, y + lines);
	target = target_buffer + COORDINATES_TO_BYTES(x, y);

	if (width == buffer_width) {
		if (retained_lines > 0) {
			memmove(target, source, LINES_TO_BYTES(retained_lines));
			target += LINES_TO_BYTES(retained_lines);
		}
		fbgfx_put_rgb_pixels(target, bg_color, lines * buffer_width);
	} else {
		for (; retained_lines > 0; retained_lines--) {
			memmove(target, source, PIXELS_TO_BYTES(width));
			target += LINES_TO_BYTES(1);
			source += LINES_TO_BYTES(1);
		}
		for (; lines > 0; lines--) {
			fbgfx_put_rgb_pixels(target, bg_color, width);
			target += LINES_TO_BYTES(1);
		}
	}

	fbgfx_add_dirt(x, y, width, height);
}

static void fbgfx_rle_decode(unsigned char *decoded_buffer, const unsigned char *rle_buffer, unsigned int decoded_size, unsigned int bytes_per_pixel)
{
	unsigned char *decoded_buffer_end;
	unsigned int codon;

	decoded_buffer_end = decoded_buffer + decoded_size;

	while (decoded_buffer < decoded_buffer_end) {
		codon = *(rle_buffer++);
		if (codon & 128) { // repeated pixel: count = codon - 128
			for (codon = codon - 128; codon > 0; codon--) {
				memcpy(decoded_buffer, rle_buffer, bytes_per_pixel);
				decoded_buffer += bytes_per_pixel;
			}
			rle_buffer += bytes_per_pixel;
		} else { // run of unencoded pixels: count = codon
			codon *= bytes_per_pixel;
			memcpy(decoded_buffer, rle_buffer, codon);
			decoded_buffer += codon;
			rle_buffer += codon;
		}
	}
}

void fbgfx_preload_image(struct fbgfx_image *image)
{
	unsigned int i;
	unsigned char swap;
	unsigned int pixel_data_size;

	if (image->expanded_pixel_data != NULL) {
		return;
	}

	if (image->bytes_per_pixel != 3 && image->bytes_per_pixel != 4) {
		dprintf(CRITICAL, "%s: loading image with %d bytes per pixel (not supported).\n", __func__, image->bytes_per_pixel);
		return;
	}

	pixel_data_size = image->width * image->height * image->bytes_per_pixel;
	image->expanded_pixel_data = (unsigned char *)memalign(4096, pixel_data_size);
	if (image->expanded_pixel_data == NULL) {
		return;
	}

	fbgfx_rle_decode(image->expanded_pixel_data, image->rle_pixel_data, pixel_data_size, image->bytes_per_pixel);

	for (i = 0; i < pixel_data_size; i += image->bytes_per_pixel) {
		/* swap to BGR for the display */
		swap = image->expanded_pixel_data[i];
		image->expanded_pixel_data[i] = image->expanded_pixel_data[i + 2];
		image->expanded_pixel_data[i + 2] = swap;
	}
}

void fbgfx_apply_image(struct fbgfx_image *image, int x, int y)
{
	unsigned char *source, *target, *target_baseline;
	unsigned int i, j;

	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	fbgfx_preload_image(image);
	if (image->expanded_pixel_data == NULL) {
		dprintf(CRITICAL, "%s: failed to preload the image.\n", __func__);
		return;
	}

	if (x == FBGFX_CENTERED) {
		x = (buffer_width - image->width) / 2;
	}
	if (y == FBGFX_CENTERED) {
		y = (buffer_height - image->height) / 2;
	}

	if (x + image->width > buffer_width) {
		return;
	}
	if (y + image->height > buffer_height) {
		return;
	}

	source = image->expanded_pixel_data;

	target_baseline = target_buffer + COORDINATES_TO_BYTES(x, y);

	for (i = 0; i < image->height; i++) {
		target = target_baseline;
		for (j = 0; j < image->width; j++) {
			if (image->bytes_per_pixel == 3 || source[3] != 0) {
				memcpy(target, source, 3);
			}
			target += buffer_bytes_per_pixel;
			source += image->bytes_per_pixel;
		}
		target_baseline += LINES_TO_BYTES(1);
	}

	fbgfx_add_dirt(x, y, image->width, image->height);
}

void fbgfx_get_font_dimensions(unsigned int *width, unsigned int *width_with_kern, unsigned int *height)
{
	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	if (width) {
		*width = FBGFX_FONT_DATA_WIDTH * FBGFX_FONT_SCALE;
	}

	if (width_with_kern) {
		*width_with_kern = FBGFX_FONT_SCREEN_WIDTH * FBGFX_FONT_SCALE;
	}

	if (height) {
		*height = FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE;
	}
}

void fbgfx_set_text_window(int x, int y, int width, int height)
{
	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	if (width == FBGFX_MAX) {
		width = buffer_width - x;
	} else if (width < 1) {
		return;
	}

	if (height == FBGFX_MAX) {
		height = buffer_height - y;
	} else if (height < 1) {
		return;
	}

	if (x == FBGFX_CENTERED) {
		x = (buffer_width - width) / 2;
	} else if (x < 0 || x >= buffer_width) {
		return;
	}

	if (y == FBGFX_CENTERED) {
		y = (buffer_height - height) / 2;
	} else if (y < 0 || y > buffer_height) {
		return;
	}

	if (x + width > buffer_width) {
		return;
	}
	if (y + height > buffer_height) {
		return;
	}

	text_x = x;
	text_y = y;
	text_width = width;
	text_height = height;
	fbgfx_set_text_cursor(0, 0);
}

void fbgfx_scroll_up_text_window(int lines)
{
	fbgfx_scroll_up(lines, text_x, text_y, text_width, text_height);
}

void fbgfx_get_text_cursor(unsigned int *row, unsigned int *col)
{
	if (row) {
		*row = cursor_y / (FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE);
	}

	if (col) {
		*row = cursor_x / (FBGFX_FONT_SCREEN_WIDTH * FBGFX_FONT_SCALE);
	}
}

void fbgfx_set_text_cursor(unsigned int row, unsigned int col)
{
	unsigned int offset;

	offset = row * FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE;
	if (offset <= (text_height - (FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE))) {
		cursor_y = offset;
	}

	offset = col * FBGFX_FONT_SCREEN_WIDTH * FBGFX_FONT_SCALE;
	if (offset <= (text_width - (FBGFX_FONT_SCREEN_WIDTH * FBGFX_FONT_SCALE))) {
		cursor_x = offset;
	}
}

static void fbgfx_draw_glyph(unsigned int *glyph)
{
	unsigned int x, y, s, data;
	unsigned char *target;

	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	target = target_buffer + COORDINATES_TO_BYTES(text_x + cursor_x, text_y + cursor_y);

	data = glyph[0];
	for (y = 0; y < (FBGFX_FONT_HEIGHT / 2); y++) {
		for (x = 0; x < FBGFX_FONT_DATA_WIDTH; x++) {
			for (s = 0; s < FBGFX_FONT_SCALE; s++) {
				if (data & 1) {
					fbgfx_put_rgb_pixels(target + LINES_TO_BYTES(s), fg_color, FBGFX_FONT_SCALE);
				} else {
					fbgfx_put_rgb_pixels(target + LINES_TO_BYTES(s), bg_color, FBGFX_FONT_SCALE);
				}
			}
			data >>= 1;
			target += PIXELS_TO_BYTES(1) * FBGFX_FONT_SCALE;
		}
		target -= PIXELS_TO_BYTES(FBGFX_FONT_DATA_WIDTH) * FBGFX_FONT_SCALE;
		target += LINES_TO_BYTES(FBGFX_FONT_SCALE);
	}

	data = glyph[1];
	for (y = 0; y < (FBGFX_FONT_HEIGHT / 2); y++) {
		for (x = 0; x < FBGFX_FONT_DATA_WIDTH; x++) {
			for (s = 0; s < FBGFX_FONT_SCALE; s++) {
				if (data & 1) {
					fbgfx_put_rgb_pixels(target + LINES_TO_BYTES(s), fg_color, FBGFX_FONT_SCALE);
				} else {
					fbgfx_put_rgb_pixels(target + LINES_TO_BYTES(s), bg_color, FBGFX_FONT_SCALE);
				}
			}
			data >>= 1;
			target += PIXELS_TO_BYTES(1) * FBGFX_FONT_SCALE;
		}
		target -= PIXELS_TO_BYTES(FBGFX_FONT_DATA_WIDTH) * FBGFX_FONT_SCALE;
		target += LINES_TO_BYTES(FBGFX_FONT_SCALE);
	}
}

static void fbgfx_nextline()
{
	cursor_x = 0;
	cursor_y += FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE;

	if(cursor_y > text_height - (FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE)) {
		cursor_y -= FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE;
		fbgfx_scroll_up_text_window(FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE);
	}
}

static void fbgfx_nextchar()
{
	cursor_x += FBGFX_FONT_SCREEN_WIDTH * FBGFX_FONT_SCALE;

	if (cursor_x > text_width - (FBGFX_FONT_DATA_WIDTH * FBGFX_FONT_SCALE)) {
		fbgfx_nextline();
	}
}

static void fbgfx_put_character(char c)
{
	if (c == '\n') {
		fbgfx_nextline();
		return;
	} else if (c == '\r') {
		cursor_x = 0;
		return;
	} else if (c < 32) {
		return;
	}

	fbgfx_draw_glyph(font + ((c - 32) * 2));
	fbgfx_nextchar();
}

void fbgfx_printf(const char *format, ...)
{
	va_list args;
	char buffer[256];
	int original_cursor_x, original_cursor_y;
	char *cursor;

	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	va_start(args, format);
	vsnprintf(buffer, 256, format, args);
	va_end(args);

	original_cursor_x = cursor_x;
	original_cursor_y = cursor_y;

	for (cursor = &(buffer[0]); *cursor != 0; cursor++) {
		fbgfx_put_character(*cursor);
	}

	fbgfx_add_dirt(
		text_x + original_cursor_x,
		text_y + original_cursor_y,
		cursor_y > original_cursor_y ? text_width : cursor_x - original_cursor_x,
		cursor_y == original_cursor_y ? FBGFX_FONT_HEIGHT * FBGFX_FONT_SCALE : cursor_y - original_cursor_y);
}

void fbgfx_enable_double_buffer()
{
	if (target_buffer != NULL) {
		return;
	}

	double_buffer_enabled = 1;
}

void fbgfx_flip()
{
	if (target_buffer == NULL) {
		dprintf(CRITICAL, "%s: fbgfx not initialized properly.\n", __func__);
		return;
	}

	if (double_buffer_enabled == 0) {
		return;
	}

	if (dirt_begin_offset > dirt_end_offset) {
		return;
	}

	memcpy(frame_buffer + dirt_begin_offset,
	       target_buffer + dirt_begin_offset,
	       dirt_end_offset - dirt_begin_offset);
	fbgfx_clear_dirt();
}
