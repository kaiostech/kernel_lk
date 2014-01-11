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
#include "bq27741.h"

#if defined(CONFIG_BQ27741)

static struct qup_i2c_dev *dev = NULL;

static int bq27741_i2c_init()
{
    /* enable i2c bus pull up voltage (LVS1) */
    pm8x41_reg_write(0x18046 /* LVS1_EN_CTL1 */, 0x80);

    /* configure i2c bus */
    dev = qup_blsp_i2c_init(BLSP_ID_2, QUP_ID_4, 100000, 24000000);

    if (!dev) {
        dprintf(CRITICAL, "Failed to initialize thermal sensor I2c\n");
        return -1;
    }

    mdelay(20); /* this delay is taken from kernel */

    return 0;
}

static int bq27741_i2c_write(uint8_t addr, uint8_t val)
{
    int ret = 0;
    uint8_t data_buf[] = { addr, val };

    /* Create a i2c_msg buffer, that is used to put the controller into write
       mode and then to write some data. */
    struct i2c_msg msg_buf[] = {
        {BQ27741_I2C_ADDR, I2C_M_WR, 2, data_buf}
    };

    ret = qup_i2c_xfer(dev, msg_buf, 1);

    if (ret == 1)
        return 0;
    else
        return -1;
}

static int bq27741_i2c_read(uint8_t addr, void *val, int count)
{
    /* Create a i2c_msg buffer, that is used to put the controller into read
       mode and then to read some data. */
    int ret = 0;
    struct i2c_msg msg_buf[] = {
        {BQ27741_I2C_ADDR, I2C_M_WR, 1, &addr},
        {BQ27741_I2C_ADDR, I2C_M_RD, count, val}
    };

    ret = qup_i2c_xfer(dev, msg_buf, 2);

    if (ret == 2)
        return 0;
    else
        return -1;
}

int bq27741_device_type(void)
{
    uint8_t cmd[2] = { 0x01, 0x00 };

    /* Switch to the I2C bus the gas gauge is connected to */
    if (bq27741_i2c_init()) {
        dprintf(INFO, "BQ27741: Failed to init i2c device\n");
        return -1;
    }

    if (bq27741_i2c_write(BQ27741_REG_CONTROL, cmd)) {
        printf("BQ27741: Error writing to control register\n");
        return -1;
    }

    if (bq27741_i2c_read(BQ27741_REG_CONTROL, cmd, sizeof(cmd))) {
        printf("BQ27741: Error reading from control register\n");
        return -1;
    }

    if (cmd[1] == 0x05 && cmd[0] == 0x41) {
        printf("BQ27741: Device type is 0x0541\n");
        return 0;
    } else {
        printf("BQ27741: Device type is invalid\n");
        return -1;
    }
}

int bq27741_capacity(uint16_t *capacity)
{
    if (!capacity)
        return -1;

    /* Switch to the I2C bus the gas gauge is connected to */
    if (bq27741_i2c_init()) {
        dprintf(INFO, "BQ27741: Failed to init i2c device\n");
        return -1;
    }

    if (bq27741_i2c_read(BQ27741_REG_SOC, capacity, sizeof(*capacity)))
        return -1;

    return 0;
}

int bq27741_temperature(int16_t *temp)
{
    uint16_t value = 0;

    if (!temp)
        return -1;

    /* Switch to the I2C bus the gas gauge is connected to */
    if (bq27741_i2c_init()) {
        dprintf(INFO, "BQ27741: Failed to init i2c device\n");
        return -1;
    }

    if (bq27741_i2c_read(BQ27741_REG_TEMP, &value, sizeof(value)))
        return -1;

    /* Convert 0.1 K to 0.1 C */
    value = value - 2731;
    *temp = value / 10;

    return 0;
}

int bq27741_voltage(uint16_t *voltage)
{
    if (!voltage)
        return -1;

    /* Switch to the I2C bus the gas gauge is connected to */
    if (bq27741_i2c_init()) {
        dprintf(INFO, "BQ27741: Failed to init i2c device\n");
        return -1;
    }

    if (bq27741_i2c_read(BQ27741_REG_VOLTAGE, voltage, sizeof(*voltage)))
        return -1;

    return 0;
}

int bq27741_current(int16_t *current)
{
    if (!current)
        return -1;

    /* Switch to the I2C bus the gas gauge is connected to */
    if (bq27741_i2c_init()) {
        dprintf(INFO, "BQ27741: Failed to init i2c device\n");
        return -1;
    }

    if (bq27741_i2c_read(BQ27741_REG_CURRENT, current, sizeof(*current)))
        return -1;

    return 0;
}

int bq27741_flags(uint16_t *flags)
{
    if (!flags)
        return -1;

    /* Switch to the I2C bus the gas gauge is connected to */
    if (bq27741_i2c_init()) {
        dprintf(INFO, "BQ27741: Failed to init i2c device\n");
        return -1;
    }

    if (bq27741_i2c_read(BQ27741_REG_FLAGS, flags, sizeof(*flags)))
        return -1;

    return 0;
}

#endif
