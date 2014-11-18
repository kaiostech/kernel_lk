/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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
#ifndef _TARGET_MSM8226_LP55X1_H
#define _TARGET_MSM8226_LP55X1_H

#define LP5521_HW_I2C_ADDRESS   0x32
#define LP55231_HW_I2C_ADDRESS  0x33

#define LP5523_REG_ENABLE       0x00
#define LP5523_REG_LED_PWM_BASE 0x16
#define LP5523_REG_CONFIG       0x36
#define LP5523_REG_RESET        0x3d
#define LP5523_ENABLE           0x40
#define LP5523_AUTO_INC         0x40
#define LP5523_PWR_SAVE         0x20
#define LP5523_PWM_PWR_SAVE     0x04
#define LP5523_CP_AUTO          0x18
#define LP5523_INT_CLK          0x01
#define LP5521_REG_ENABLE       0x00
#define LP5521_REG_OP_MODE      0x01
#define LP5521_REG_R_PWM        0x02
#define LP5521_REG_G_PWM        0x03
#define LP5521_REG_B_PWM        0x04
#define LP5521_REG_R_CURRENT    0x05
#define LP5521_REG_CONFIG       0x08
#define LP5521_REG_RESET        0x0D
#define LP5521_CMD_DIRECT       0x3f
#define LP5521_PWRSAVE_EN       0x20
#define LP5521_R_TO_BATT        4
#define LP5521_CLK_INT          1
#define LP5521_CP_MODE_AUTO     0x18
#define LP5521_MASTER_ENABLE    0x40
#define LP5521_REG_R_CURR_DEFAULT 0xAF

void lp55x1_start_leds(void);

#endif
