/*
 * (C) Copyright 2009
 * Texas Instruments, <www.ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define BQ27741_I2C_ADDR        (0x55)      /* I2C Addr of BQ27741 */
 
/* BQ27741 registers */
#define BQ27741_REG_CONTROL     (0x00)      /* Control register */
#define BQ27741_REG_TEMP        (0x06)      /* Temperature (in 0.1 K) */
#define BQ27741_REG_VOLTAGE     (0x08)      /* Voltage (in mV) */
#define BQ27741_REG_FLAGS       (0x0a)      /* Flags */
#define BQ27741_REG_FLAGS_DSG   (1 << 0)    /* Discharging */
#define BQ27741_REG_FLAGS_FC    (1 << 9)    /* Full charge */
#define BQ27741_REG_FLAGS_OTD   (1 << 14)   /* Over-temp under discharge */
#define BQ27741_REG_FLAGS_OTC   (1 << 15)   /* Over-temp under charge */
#define BQ27741_REG_CURRENT     (0x14)      /* Current (in mA) */
#define BQ27741_REG_SOC         (0x2c)      /* State of Charge (in %) */

/* Function*/

