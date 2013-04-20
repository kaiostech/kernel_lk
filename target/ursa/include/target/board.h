/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#ifndef __TARGET_BOARD_H
#define __TARGET_BOARD_H

/* ASCII(U+R+S+A)*10 = 3150 */
#define HW_PLATFORM_URSA_P0        3150
#define LINUX_MACHTYPE_URSA_P0     HW_PLATFORM_URSA_P0
#define HW_PLATFORM_URSA_PRE_P1    3151
#define LINUX_MACHTYPE_URSA_PRE_P1 HW_PLATFORM_URSA_PRE_P1
#define HW_PLATFORM_URSA_P1        3152
#define LINUX_MACHTYPE_URSA_P1     HW_PLATFORM_URSA_P1

/* Board ID Macros */
#define BOARD_REVISION_INVALID     0x0
#define BOARD_REVISION_P0          0x1
#define BOARD_REVISION_PRE_P1      0x3
#define BOARD_REVISION_P1          0x4
#define BOARD_REVISION_P0_5        0x5
#define BOARD_REVISION_MAX         BOARD_REVISION_P0_5

#endif
