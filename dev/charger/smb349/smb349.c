/*
 * smb349.c
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
#include <smb349.h>

#if defined(CONFIG_SMB349)

#define SMB349_ADDR_ORIG 	   (0x06)	/* I2C Addr of SMB349 (Orig) */
#define SMB349_ADDR	 		   (0x5F)	/* I2C Addr of SMB349 */

#define SMB349_LOG(fmt, args...) dprintf(INFO, "SMB349: " fmt, ## args)

static struct qup_i2c_dev *dev = NULL;
static int smb349_addr = 0;
static int smb349_init_flag = 0;

static int smb349_i2c_init()
{
    /* enable i2c bus pull up voltage (LVS1) */
    pm8x41_reg_write(0x18046 /* LVS1_EN_CTL1 */, 0x80);

    /* configure i2c bus */
    dev = qup_blsp_i2c_init(BLSP_ID_2, QUP_ID_4, 100000, 24000000);

    if (!dev) {
        SMB349_LOG("Failed to initialize thermal sensor I2c\n");
        return -1;
    }

    mdelay(20); /* this delay is taken from kernel */

    return 0;
}

static int smb349_i2c_probe()
{
    int ret = 0;

    uint8_t addr = SMB349_COMMAND_REG_A;
    uint8_t val = 0;

    struct i2c_msg msg_buf[] = {
        {SMB349_ADDR, I2C_M_WR, 1, &addr},
        {SMB349_ADDR, I2C_M_RD, 1, &val}
    };

    ret = qup_i2c_xfer(dev, msg_buf, 2);

    if (ret == 2) {
        smb349_addr = SMB349_ADDR;
    } else {
        msg_buf[0].addr = SMB349_ADDR_ORIG;
        ret = qup_i2c_xfer(dev, msg_buf, 2);

        if (ret == 2) {
            smb349_addr = SMB349_ADDR_ORIG;
        }
        else
            return -1;
    }

    return smb349_addr;
}

static int smb349_i2c_write(uint8_t addr, uint8_t val)
{
    int ret = 0;
    uint8_t data_buf[] = { addr, val };

    /* Create a i2c_msg buffer, that is used to put the controller into write
       mode and then to write some data. */
    struct i2c_msg msg_buf[] = {
        {smb349_addr, I2C_M_WR, 2, data_buf}
    };

    ret = qup_i2c_xfer(dev, msg_buf, 1);

    if (ret == 1)
        return 0;
    else
        return -1;
}

static int smb349_i2c_read(uint8_t addr, uint8_t *val)
{
    int ret = 0;
    /* Create a i2c_msg buffer, that is used to put the controller into read
       mode and then to read some data. */
    struct i2c_msg msg_buf[] = {
        {smb349_addr, I2C_M_WR, 1, &addr},
        {smb349_addr, I2C_M_RD, 1, val}
    };

    ret = qup_i2c_xfer(dev, msg_buf, 2);

    if (ret == 2)
        return 0;
    else
        return -1;
}

inline int smb349_aicl_current(int reg)
{
    const int aicl_currents[] = {
        300, 500, 700, 900,
        1200, 1500, 1800, 2000,
        2200, 2500, 2500, 2500,
        2500, 2500, 2500, 2500
    };

    if (SMB349_IS_AICL_DONE(reg)) {
        int value = SMB349_AICL_RESULT(reg);

        if (value >= 0 && value <= 15) {
            return aicl_currents[value];
        }
        return -1;

    } else {
        return -1;
    }
}

inline int smb349_charging_current(int reg)
{
    int current = -1;

    if (reg & (1 << 5)) {
        switch (reg & 0x7) {
            case 0:
                current = 700;
                break;
            case 1:
                current = 900;
                break;
            case 2:
                current = 1200;
                break;
            case 3:
                current = 1500;
                break;
            case 4:
                current = 1800;
                break;
            case 5:
                current = 2000;
                break;
            case 6:
                current = 2200;
                break;
            case 7:
                current = 2500;
                break;
        }
    } else {
        switch ((reg >> 3) & 0x3) {
            case 0:
                current = 100;
                break;
            case 1:
                current = 150;
                break;
            case 2:
                current = 200;
                break;
            case 3:
                current = 250;
                break;
        }
    }

    return current;
}

inline int smb349_check_apsd_result(uint8_t value)
{
    switch (value) {
        case SMB349_APSD_RESULT_NONE:
        case SMB349_APSD_RESULT_OTHER:
        case SMB349_APSD_RESULT_SDP:
        case SMB349_APSD_RESULT_DCP:
        case SMB349_APSD_RESULT_CDP:
        case SMB349_APSD_RESULT_ACA_A:
        case SMB349_APSD_RESULT_ACA_B:
        case SMB349_APSD_RESULT_ACA_C:
        case SMB349_APSD_RESULT_ACA:
             return value;
        default:
             return -1;
    }
}

static const char *smb349_apsd_result_string(uint8_t value)
{
    switch (smb349_check_apsd_result(value)) {
        case SMB349_APSD_RESULT_ACA:
            return "ACA charger";
            break;
        case SMB349_APSD_RESULT_ACA_A:
            return "ACA_A";
            break;
        case SMB349_APSD_RESULT_ACA_B:
            return "ACA_B";
            break;
        case SMB349_APSD_RESULT_ACA_C:
            return "ACA_C";
            break;
        case SMB349_APSD_RESULT_CDP:
            return "CDP";
            break;
        case SMB349_APSD_RESULT_DCP:
            return "DCP";
            break;
        case SMB349_APSD_RESULT_OTHER:
            return "Other Downstream Port";
            break;
        case SMB349_APSD_RESULT_SDP:
            return "SDP";
            break;
        case -1:
            return "not run";
            break;
        case SMB349_APSD_RESULT_NONE:
        default:
            return "unknown";
            break;
    }
}

int smb349_config_enable(int flag)
{
    int status = -1;
    uint8_t value = 0;

    if (smb349_i2c_read(SMB349_COMMAND_REG_A, &value)) {
        SMB349_LOG("Unable to read command register A\n");
        goto done;
    }

    if (flag) {
        /* Enable bit 7 */
        value |= (1 << 7);
    } else {
        /* Disable bit 7 */
        value &= ~(1 << 7);
    }

    if (smb349_i2c_write(SMB349_COMMAND_REG_A, value)) {
        SMB349_LOG("Unable to write command register A\n");
        goto done;
    }

    status = 0;
done:
    return status;
}

static inline void smb349_parse_status_reg_a(uint8_t value)
{
    SMB349_LOG("Status Register A = 0x%02x\n", value);

    SMB349_LOG("Thermal Regulation Status: %s\n",
            (value & (1 << 7)) ? "Active" : "Inactive");

    SMB349_LOG("THERM Soft Limit Regulation Status: %s\n",
            (value & (1 << 6)) ? "Active" : "Inactive");

    int voltage = 3500 + (value & 0x3f) * 20;

    /* Max out at 4500 mV */
    if (voltage > 4500) voltage = 4500;

    SMB349_LOG("Actual Float Voltage after compensation: %d mV\n", voltage);
}

static inline void smb349_parse_status_reg_b(uint8_t value)
{
    SMB349_LOG("Status Register B = 0x%02x\n", value);

    SMB349_LOG("USB Suspend Mode: %s\n",
            (value & (1 << 7)) ? "Active" : "Inactive");

    int current = smb349_charging_current(value);

    if (current != -1) {
        SMB349_LOG("Actual Charge Current after "
                "compensation: %d mA\n", current);
    } else {
        SMB349_LOG("Actual Charge Current after "
                "compensation: Unknown\n");
    }
}

static inline void smb349_parse_status_reg_c(uint8_t value)
{
    SMB349_LOG("Status Register C = 0x%02x\n", value);

    SMB349_LOG("Charging Enable/Disable: %s\n",
            (value & 0x1) ? "Enabled" : "Disabled");

    switch (SMB349_CHARGING_STATUS(value)) {
    case SMB349_CHARGING_STATUS_NOT_CHARGING:
        SMB349_LOG("Charging Status: Not charging\n");
        break;
    case SMB349_CHARGING_STATUS_PRE_CHARGING:
        SMB349_LOG("Charging Status: Pre-charging\n");
        break;
    case SMB349_CHARGING_STATUS_FAST_CHARGING:
        SMB349_LOG("Charging Status: Fast-charging\n");
        break;
    case SMB349_CHARGING_STATUS_TAPER_CHARGING:
        SMB349_LOG("Charging Status: Taper-charging\n");
        break;
    default:
        SMB349_LOG("Charging Status: Unknown\n");
	    break;
	}

		SMB349_LOG("Charger %s hold-off status\n",
			(value & (1 << 3)) ? "in" : "not in");

	SMB349_LOG("Vbatt %c 2.1 V\n", (value & (1 << 4)) ? '<' : '>');

	if (value & (1 << 5)) {
		SMB349_LOG("At least one charging cycle has terminated\n");
	} else {
		SMB349_LOG("No full charge cycle has occurred\n");
	}

	if (value & (1 << 6))
		SMB349_LOG("Charger has encountered an error\n");

	SMB349_LOG("Charger error %s an IRQ signal\n",
			(value & (1 << 7)) ? "asserts" : "does not assert");
}

static inline void smb349_parse_status_reg_e(uint8_t value)
{
	int current = -1;

	SMB349_LOG("Status Register E = 0x%02x\n", value);

	SMB349_LOG("USBIN Input: %s\n",
		(value & (1 << 7)) ? "In Use" : "Not In Use");

	switch (SMB349_USB1_5_HC_MODE(value)) {
	case SMB349_HC_MODE:
		SMB349_LOG("In HC mode\n");
		break;
	case SMB349_USB1_MODE:
		SMB349_LOG("In USB1 mode\n");
		break;
	case SMB349_USB5_MODE:
		SMB349_LOG("In USB5 mode\n");
		break;
	}

	current = smb349_aicl_current(value);

	if (current != -1) {
		SMB349_LOG("AICL Completed, Result = %d mA\n", current);
	} else {
		SMB349_LOG("AICL not completed\n");
	}
}

static int smb349_set_current_setting(void)
{
	int ret = -1;
	uint8_t value = 0xff;

	if ((ret = smb349_i2c_read(0x0, &value))) {
		SMB349_LOG("%s: Unable to read SMB349_CHARGE_CURRENT: %d\n", __func__, ret);
		goto done;
	}

	if (smb349_config_enable(1)) {
		ret = -1;
		goto done;
	}

	value &= ~(0xFF);
	/* Set bit 7 (Fast charge current = 1800 mA) */
	value |= 0x80;

	/* Set bits 3 and 4 (Pre-charge current = 250 mA) */
	value |= 0x18;

	/* Set bits 0 and 2 (Termination current = 250 mA) */
	value |= 0x05;

	if ((ret = smb349_i2c_write(0x0, value))) {
		SMB349_LOG("%s: Unable to write SMB349_CHARGE_CURRENT: %d\n", __func__, ret);
	}

	if (smb349_config_enable(0)) {
		ret = -1;
	}

done:
	return ret;
}

static int smb349_set_current_compensation(void)
{
	int ret = -1;
	uint8_t value = 0xff;

	if ((ret = smb349_i2c_read(0x0A, &value))) {
		SMB349_LOG("%s: Unable to read SUMMIT_SMB349_OTG_THERM_CONTROL: %d\n", __func__, ret);
		goto done;
	}

	if (smb349_config_enable(1)) {
		ret = -1;
		goto done;
	}

	value &= ~(0xC0);

	/* Set bit 7 (Charge current compensation = 900 mA) */
	value |= 0x80;

	if ((ret = smb349_i2c_write(0x0A, value))) {
		SMB349_LOG("%s: Unable to write SUMMIT_SMB349_OTG_THERM_CONTROL: %d\n", __func__, ret);
	}

	if (smb349_config_enable(0)) {
		ret = -1;
	}

done:
	return ret;
}

static int smb349_set_temperature_threshold(void)
{
	int ret = -1;
	uint8_t value = 0xff;

	if (smb349_config_enable(1)) {
		ret = -1;
		goto done;
	}

	/* Set bit 7 (Hard Cold temp = 0 degree C) */
	/* Set bit 5 (Hard Hot temp = 60 degree C) */
	/* Clear bits 2 and 3 (Soft Cold temp = 15 degree C) */
	/* Set bit 0 (Soft Hot temp = 45 degree C) */
	value = 0xA1;

	if ((ret = smb349_i2c_write(0x0B, value))) {
		SMB349_LOG("%s: Unable to write SUMMIT_SMB349_CELL_TEMP: %d\n", __func__, ret);
	}

	if (smb349_config_enable(0)) {
		ret = -1;
		goto done;
	}

done:
	return ret;
}

static int smb349_switch_mode(int mode)
{
	int ret = 0;
	uint8_t value = 0xff;

	if ((ret = smb349_i2c_read(SMB349_COMMAND_REG_B, &value))) {
		SMB349_LOG("%s: Unable to read SMB349_COMMAND_REG_B: %d\n",__func__, ret);
		goto done;
	} else {
		switch (mode) {
		case SMB349_USB1_MODE:
			SMB349_LOG("Switching to USB1 mode\n");
			value &= ~0x01;
			value &= ~0x02;
			break;
		case SMB349_USB5_MODE:
			SMB349_LOG("Switching to USB5 mode\n");
			value &= ~0x01;
			value |= 0x02;
			break;
		case SMB349_HC_MODE:
			SMB349_LOG("Switching to HC mode\n");
			value |= 0x01;
			break;
		default:
			SMB349_LOG("Unknown USB mode: %d\n", mode);
			return -1;
		}

		if ((ret = smb349_i2c_write(SMB349_COMMAND_REG_B, value))) {
			SMB349_LOG("%s: Unable to write SMB349_COMMAND_REG_B: %d\n", __func__, ret);
			goto done;
		}
		ret = 0;
	}

done:
	return ret;
}

/*
 * Return value type
 *
 *   0: No Usb Connected
 *   1: Usb/AC Charger Connected
 *  -1: Read failed
 *
 */

int smb349_check_usb_vbus_connection(int *wall_charger)
{
	uint8_t usb_status = 0;

	if (!smb349_init_flag) {
		SMB349_LOG("Please call smb349_init firstly \n");
		return -1;
	}

	if (smb349_i2c_read(SMB349_STATUS_REG_D, &usb_status)) {
		SMB349_LOG("Read SMB349_STATUS_REG_D failed \n");
		return -1;
	}

	dprintf(INFO, "Read SMB349_STATUS_REG_D value = [%d] \n", usb_status);

	switch(usb_status) {
		case SMB349_APSD_RESULT_NONE: return 0; break;
		case SMB349_APSD_RESULT_OTHER:
		case SMB349_APSD_RESULT_SDP:
		case SMB349_APSD_RESULT_CDP: return 1; break;
		case SMB349_APSD_RESULT_DCP: {
				if (wall_charger != NULL)
					*wall_charger = 1;
				return 1;
				break;
			}
		default: return 0;
	}
}

int smb349_init(int *wall_charger)
{
	uint8_t status = -1;
	uint8_t val = 0, mode = 0, tmp = 0;
	int vusb = 0;

	if (smb349_init_flag) {
		return 1; //initialized.
	}

	if (smb349_i2c_init()) {
		SMB349_LOG("Init i2c failed\n");
		goto done;
	}

	if (smb349_i2c_probe() != -1) {
		SMB349_LOG("SMB349 at I2C Address %02x\n", smb349_addr);
	} else {
		SMB349_LOG("Can't detect SMB349 IC \n");
		goto done;
	}
	smb349_init_flag = 1;

	/* Check if we have VBUS */
	vusb = smb349_check_usb_vbus_connection(wall_charger);
	if (vusb == -1) {
		SMB349_LOG("Detect vusb failed \n");
		goto done;
	}
	if (vusb != 1) {
		/* Nothing on USB, skip USB detection */
		SMB349_LOG("Nothing on USB\n");
		status = 0;
		goto done;
	}

	status = 0;
done:

	return status;
}

#endif
