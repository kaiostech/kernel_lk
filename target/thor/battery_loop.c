/*
 * battery_loop.c
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.
 * Donald Chan (hoiho@lab126.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <debug.h>
#include <platform/iomap.h>
#include <platform/gpio.h>
#include <platform/clock.h>
#include <reg.h>
#include <target.h>
#include <platform.h>
#include <uart_dm.h>
#include <mmc.h>
#include <spmi.h>
#include <board.h>
#include <smem.h>
#include <baseband.h>
#include <dev/keys.h>
#include <pm8x41.h>
#include <pm8x41_hw.h>
#include <crypto5_wrapper.h>
#include <hsusb.h>
#include <clock.h>
#include <i2c_qup.h>
#include <blsp_qup.h>
#include <partition_parser.h>
#include <scm.h>
#include <target/display.h>

/* display function */
extern void show_image(Image_types type);
extern void display_shutdown(void);

int Init_LCD(void)
{
	dprintf(INFO, "Init LCD\n");
	return 0;
}

void Power_on_LCD(void)
{
	dprintf(INFO, "Power on LCD\n");
	lp855x_bl_on();
}

void Power_off_LCD(void)
{
	dprintf(INFO, "Power off LCD\n");
	lp855x_bl_off();
}

void show_lowbattery(void)
{
	dprintf(INFO, "Show low battery\n");
	show_image(IMAGE_LOWBATTERY);
}

void show_charging(void)
{
	dprintf(INFO, "Show charging\n");
	show_image(IMAGE_CHARGING);
}

void show_devicetoohot(void)
{
	dprintf(INFO, "Show device too hot\n");
	show_image(IMAGE_DEVICEHOT);
}

/* battery function */
#if defined(CONFIG_BQ27741)
extern int bq27741_capacity(uint16_t *capacity);
extern int bq27741_voltage(uint16_t *voltage);
extern int bq27741_current(int16_t *current);
extern int bq27741_temperature(int16_t *temp);
#endif /* CONFIG_BQ27741 */

#ifdef CONFIG_SMB349
/* charger function */
extern int smb349_init(void);
extern int smb349_check_usb_vbus_connection(int *wall_charger);
#endif

#define TEMPERATURE_COLD_LIMIT -19 /* -19 C */
#define TEMPERATURE_HOT_LIMIT 59 /*  59 C */
#define LOW_BATTERY_CAPACITY_LIMIT 2 /*  2% */
#define LOW_BATTERY_VOLTAGE_LIMIT 3430 /* 3.43 V */
#define LOW_BATTERY_SPLASH_VOLTAGE_LIMIT 3100   /* 3.1 V */
#define LOW_CURRENT_LIMIT 1400 /* 1400 mA (Wall Charger) */

#define BATTERY_CHRG_ON 0x01
#define BATTERY_CHRG_OFF 0x00

#define DEVICE_TEMP_BOOT_LIMIT 64 /* in degres C */
#define MAX_CHARGING_TIME_TOO_LONG (30*60*5*2) /* in seconds */
#define DISCHARGING_COUNT 10
#define POWER_BUTTON_SCAN_LOOP_COUNT 20   /* # of for loop for 1 milliscecond */
#define BATTERY_READY_TO_BOOT_COUNT 5
#define READ_TEMP_ERR_COUNT_LIMIT 10

struct battery_charging_info {
    uint16_t  voltage;
    uint16_t capacity;
    int16_t current;
    int16_t temperature;
    uint8_t tmp103_temp;
};

enum pic_type {
    PIC_LOWBATTERY = 0,
    PIC_CHARGING,
    PIC_TOOHOT,
    NUM_OF_PIC
};

extern int tmp103_temperature_read(void *buffer, size_t size);

extern void shutdown_device();
extern void target_set_charging(int enable);

static void shut_down(void)
{
    dprintf(INFO, "Shutting down device...\n");

    shutdown_device();
    while (1);
}

static void delay_ms(int ms)
{
    int timeout;
    for (timeout = 0; timeout < ms; timeout++) {
        udelay(1000);
    }
}

static void show_error_logo(enum pic_type type, int pre_ms, int max_ms)
{
#define CHECK_VBUS_MS 500
    int ms, times, remain_ms;
    int check_vbus_flag = 0;
    int cable_status = 0;

    if(!Init_LCD())
        Power_on_LCD();

    switch(type) {
        case PIC_LOWBATTERY:
            show_lowbattery();
            dprintf(INFO, "Display low battery picture...\n");
        break;
        case PIC_CHARGING:
            show_charging();
            dprintf(INFO, "Display low battery charging picture...\n");
        break;
        case PIC_TOOHOT:
            show_devicetoohot();
            dprintf(INFO, "Display device too hot picture...\n");
        break;
        default:
            dprintf(INFO, "Error to display picture with type %d...\n", type);
        break;
    }

    if ((pre_ms > 0) || ((pre_ms == 0) && (type == PIC_CHARGING)))
        check_vbus_flag = 1;

    /* Delay for 5 seconds */
    delay_ms(5000);

    Power_off_LCD();
}

static int check_pwr_key_press(int *ms)
{
    int pressed = 0;
    int local_ms = 0;

    /* Delay for ~5 seconds, note: hand-tuned loop, be careful! */
    for (local_ms = 0; local_ms < 20; local_ms++) {

        /* Read whether a button was pressed/released */
        pressed = pm8x41_reg_read(0x810) & 0x1;
        delay_ms(50); //50ms

	if (pressed)
		break;
    }

    *ms = local_ms;

    dprintf(INFO, "check pwr key pressed = %d\n", pressed);

    return pressed;
}

int get_battery_charging_info(struct battery_charging_info *data)
{
    int i = 0;

    data->voltage = 0;
    data->capacity = 0;
    data->current = 0;
    data->temperature = 0;
    data->tmp103_temp = 0;

#if defined(CONFIG_BQ27741)
    if (bq27741_voltage(&data->voltage)) {
        dprintf(INFO, "Unable to determine battery voltage\n");
    }

    if (bq27741_capacity(&data->capacity)) {
        dprintf(INFO, "Unable to determine battery capacity\n");
    }

    if (bq27741_current(&data->current)) {
        dprintf(INFO, "Unable to determine battery current\n");
    }

    if (bq27741_temperature(&data->temperature)) {
        dprintf(INFO, "Unable to determine battery temperature\n");
    }

#endif /* CONFIG_BQ27741 */
    i = 0;
    do {
        if (!tmp103_temperature_read(&data->tmp103_temp, sizeof(uint8_t)))
            break;
        else {
            dprintf(INFO, "Unable to read temperature from TMP103 (%d)\n", i);
            delay_ms(100);
        }
    } while (++i < READ_TEMP_ERR_COUNT_LIMIT);

    if(i >= READ_TEMP_ERR_COUNT_LIMIT) {
        dprintf(INFO, "Too many error to read the temperature, shut down the device..\n");
        shut_down();
    }

#if defined(CONFIG_BQ27741)
    if ((data->voltage == 0) &&
            (data->capacity == 0) &&
            (data->current == 0) &&
            (data->temperature == 0)) {
        return -1;
    }
#endif

    return 0;
}

extern int target_volume_down();
void check_battery_condition(void)
{
    struct battery_charging_info info;
    int wall_charger = -1;
    int charge_screen_displayed = 0;
    int pwr_key_pressed = 0;
    int discharge_count = 0, charge_enough = 0;
    int sec = 0, powerkey_ms = 0;
    int cable_status = 0;
    int ret = 0;
    int delay_time = 0;

#ifdef CONFIG_SMB349
    if (smb349_init() == -1)
        dprintf(INFO, "SMB349 initial failed\n");

    cable_status = smb349_check_usb_vbus_connection(&wall_charger);
    if (cable_status == -1)
		dprintf(INFO, "SMB349 detect usb vbus fasiled \n");
#endif

    if (cable_status == 1) {
        if(wall_charger == 1) {
            dprintf(INFO, "Detect wall charger.\n");
        } else {
            dprintf(INFO, "Detect usb.\n");
        }
    } else {
        dprintf(INFO, "No usb connected \n");
    }

    ret = get_battery_charging_info(&info);

    if (ret == -1) {
        dprintf(INFO, "Battery gas gauge not responding, skipping charge loop...\n");
        return;
    }

    /* Display battery information */
    dprintf(INFO, "Battery: voltage = %4d mV, current = %4d mA,\n", info.voltage, info.current);
    dprintf(INFO, "         capacity = %4d %%,  temperature = %3d C, tmp103 = %3d C\n", info.capacity, info.temperature, info.tmp103_temp);

    if ((info.temperature > TEMPERATURE_HOT_LIMIT) || (info.tmp103_temp >= DEVICE_TEMP_BOOT_LIMIT)) {
        dprintf(INFO, "Device is too hot (battery:%d C, temp103:%d C). It is not safe to boot\n", info.temperature, info.tmp103_temp);

        //if (voltage > LOW_BATTERY_SPLASH_VOLTAGE_LIMIT)
        show_error_logo(PIC_TOOHOT, 0, 4000);

        shut_down();
    }

    if ((info.voltage >= LOW_BATTERY_VOLTAGE_LIMIT) && (info.capacity >= LOW_BATTERY_CAPACITY_LIMIT)) {
        dprintf(INFO, "cond: voltage = %d, capacity = %d can start to boot..\n", info.voltage, info.capacity);
    }
    else {
        if ((info.voltage > LOW_BATTERY_SPLASH_VOLTAGE_LIMIT) || (wall_charger == 1) ) {
            if (info.current > 0)
                show_error_logo(PIC_CHARGING, 0, 5000);
            else {
                if (cable_status == 1) {
                    enum pic_type pic = PIC_LOWBATTERY;
                    delay_time = 30000;
                    int i = 0;
                    while (++i <= 30) {
                        get_battery_charging_info(&info);
                        dprintf(INFO, "cable_status = 1, current = %d, retry = %d\n", info.current, i);
                        if (info.current > 0) {
                            pic = PIC_CHARGING;
                            delay_time = 5000;
                            break;
                        }
                        delay_ms(50);
                    }

                    show_error_logo(pic, 0, delay_time);
                }
                else {
                    show_error_logo(PIC_LOWBATTERY, 0, 30000);
                }
            }

            charge_screen_displayed = 1;
        }
        else {
            dprintf(INFO, "Unable to dispay low battery (charging) notification\n");
        }

        dprintf(INFO, "Entering charge loop...\n");

        while(1) {

#ifdef CONFIG_SMB349
            /* USB/Charger connection check */
            cable_status = smb349_check_usb_vbus_connection(NULL);
            if (cable_status == -1) {
                dprintf(INFO, "SMB349 detect usb vbus failed \n");
                shut_down();
            }

            if(cable_status == 0) {
                dprintf(INFO, "USB Cable have been plug out \n");
                shut_down();
            }
#endif
            get_battery_charging_info(&info);

            /* Display status */
            dprintf(INFO, "t = %5d, voltage = %4d mV, current = %4d mA, capacity = %2d %%, temperature = %3d C, temp103 = %3d C\n",
                        sec, info.voltage, info.current, info.capacity, info.temperature, info.tmp103_temp );

            if (target_volume_down()) {
                dprintf(INFO, "Detect volume down key pressed, break the charging loop \n");
                break;
            }

            /* Temperature check */
            if ((info.temperature > TEMPERATURE_HOT_LIMIT) ||
                (info.temperature < TEMPERATURE_COLD_LIMIT)) {
                dprintf(INFO, "Battery temperature threshold reached: %d C", info.temperature);

                /* FIXME: do we need to show something in the dispay here!! */
                shut_down();
            }

            /* Charging current check */
            if (info.current <= 0 && ++discharge_count > DISCHARGING_COUNT) {
                dprintf(INFO, "Battery is being discharged for too long\n");
                shut_down();
            }
            else if (info.current > 0) {
                /* Reset discharge count */
                discharge_count = 0;
            }

            /* Software charging time check */
            if (sec > MAX_CHARGING_TIME_TOO_LONG) {
                dprintf(INFO, "Battery has been charged for too long: %d s\n", sec);
                shut_down();
            }

            /* Check if battery is good enough to boot into system */
            if ((info.voltage >= LOW_BATTERY_VOLTAGE_LIMIT) &&
                (info.capacity >= LOW_BATTERY_CAPACITY_LIMIT)) {
                if (++charge_enough > BATTERY_READY_TO_BOOT_COUNT) {
                    dprintf(INFO, "Battery is enough to boot into system, breaking out of loop\n");
                    break;
                }
            }
            else {
                charge_enough = 0;
            }

            /* Delay for ~5 seconds and check the power key status */
            pwr_key_pressed = check_pwr_key_press(&powerkey_ms);

            if ((pwr_key_pressed || !charge_screen_displayed) &&
                ((info.voltage > LOW_BATTERY_SPLASH_VOLTAGE_LIMIT) || (wall_charger == 1) ) ) {
                if(info.current > 0)
                    show_error_logo(PIC_CHARGING, powerkey_ms, 5000);
                else
                    show_error_logo(PIC_LOWBATTERY, powerkey_ms, 30000);

                charge_screen_displayed = 1;
                sec += 5;
            }
            else {
                sec += 1;
            }
        }

        dprintf(INFO, "Leaving charge loop...\n");
    }
}



void charge_mode_loop(void)
{
	struct battery_charging_info info;
	int pwr_key_pressed = 0;
	int powerkey_ms = 0;
	int cable_status = 0;

#ifdef CONFIG_SMB349
	if (smb349_init() == -1) {
		dprintf(INFO, "SMB349 init failed\n");
		shut_down();
	}
#endif

	dprintf(INFO, "Entering charge mode loop...\n");
	Power_off_LCD();

	while(1) {

#ifdef CONFIG_SMB349
		/* USB/Charger connection check */
		cable_status = smb349_check_usb_vbus_connection(NULL);
		if (cable_status == -1) {
			dprintf(INFO, "SMB349 detect usb vbus failed \n");
			shut_down();
		}
#endif
		if (cable_status <= 0) {
			dprintf(INFO, "Charger has been removed, halting \n");
			shut_down();
		}

		get_battery_charging_info(&info);

		/* Display status */
		dprintf(INFO,
			"v=%4dmV current=%4dmA cap=%2d%% temp=%3dC %s\n",
			info.voltage, info.current,
			info.capacity, info.temperature,
			target_get_charging() ?
			"Charging enabled" : "Charging disabled");

		/* Temperature check */
		if ((info.temperature > TEMPERATURE_HOT_LIMIT) ||
			(info.temperature < TEMPERATURE_COLD_LIMIT)) {
			dprintf(INFO,
				"Battery temperature threshold reached: %d C",
				info.temperature);

			/* Turn off charging but stay in loop */
			if (target_get_charging())
				target_set_charging(0);
		} else {
			/* Temp back to normal, turn on charging if off */
			if (!target_get_charging())
				target_set_charging(1);
		}

		/* Delay for ~5 seconds and check the power key status */
		pwr_key_pressed = check_pwr_key_press(&powerkey_ms);
		if (pwr_key_pressed) {
			break;
		}
        }
}
