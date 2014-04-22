/* Copyright (c) 2014, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#ifndef __BOOTSELECT_PARSER_H
#define __BOOTSELECT_PARSER_H

#include <mmc.h>
#include <partition_parser.h>
#include "bootselect_parser.h"


#define BOOTSELECT_SIGNATURE ('B' | ('S' << 8) | ('e' << 16) | ('l' << 24))
#define BOOTSELECT_VERSION   0x00010001
#define BOOTSELECT_FORMAT    (1 << 31)
#define BOOTSELECT_FACTORY   (1 << 30)
#define FUSE_ADDR 0xFE80565C
#define FUSE_BIT  0

/* bootselect partition format structure */
struct boot_selection_info {
	uint32_t signature;                // Contains value BOOTSELECT_SIGNATURE defined above
	uint32_t version;
	uint32_t boot_partition_selection; // Decodes which partitions to boot: 0-Windows,1-Android
	uint32_t state_info;               // Contains factory and format bit as definded above
};

bool read_boot_select_partition();
bool needs_write_protection();
bool check_format_bit();
int write_protect_partitions(int enable);
#endif
