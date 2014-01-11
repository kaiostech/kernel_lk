/*
 * bq27741.c
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.
 * Donald Chan (hoiho@lab126.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <bits.h>
#include <debug.h>
#include <reg.h>
#include <spmi.h>
#include <string.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <i2c_qup.h>
#include <blsp_qup.h>
#include <pm8x41_hw.h>
#include <tmp103.h>

#if defined(CONFIG_TMP103)

static struct qup_i2c_dev *dev = NULL;

static int tmp103_i2c_init()
{
    uint8_t ret = 0;
    /* enable i2c bus pull up voltage (LVS1) */
    pm8x41_reg_write(0x18046 /* LVS1_EN_CTL1 */, 0x80);

    /* configure i2c bus */
    dev = qup_blsp_i2c_init(BLSP_ID_2, QUP_ID_4, 100000, 24000000);
    if (!dev) {
		TMP103_LOG("Failed to initialize thermal sensor I2c\n");
        return -1;
    }

    mdelay(20); /* this delay is taken from kernel */

    return ret;
}

static int tmp103_i2c_write(uint8_t addr, uint8_t val)
{
	int ret = 0;
    uint8_t data_buf[] = { addr, val };

    /* Create a i2c_msg buffer, that is used to put the controller into write
      mode and then to write some data. */
    struct i2c_msg msg_buf[] = {
           {TMP103_ADDR, I2C_M_WR, 2, data_buf}
    };

    ret = qup_i2c_xfer(dev, msg_buf, 1);

	if (ret == 1)
        return 0;
	else
        return -1;
}

static int tmp103_i2c_read(uint8_t addr, uint8_t *val)
{
	int ret = 0;
    /* Create a i2c_msg buffer, that is used to put the controller into read
       mode and then to read some data. */
    struct i2c_msg msg_buf[] = {
        {TMP103_ADDR, I2C_M_WR, 1, &addr},
        {TMP103_ADDR, I2C_M_RD, 1, val}
    };

    ret = qup_i2c_xfer(dev, msg_buf, 2);

	if (ret == 2)
        return 0;
	else
        return -1;
}

int tmp103_temperature_read(void *buffer, size_t size)
{
    /* Switch to the I2C bus the gas gauge is connected to */
    if (tmp103_i2c_init()) {
        dprintf(INFO, "TMP103: Failed to init i2c device\n");
        return -1;
    }

    if (tmp103_i2c_read(TMP103_REG_CONTROL, (uint8_t*)buffer)) {
        dprintf(INFO, "tmp103_temperature_read read failed\n");
        return -1;
	}

	return 0;
}

#endif
