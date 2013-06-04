/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
 */

#include <debug.h>
#include <platform/gpio.h>
#include <platform/iomap.h>
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
#include <crypto5_wrapper.h>
#include <target/board.h>
#ifdef WITH_ENABLE_IDME
#include <idme.h>
#endif
#include "target_cert.h"

extern  bool target_use_signed_kernel(void);

static unsigned int target_id;
static uint32_t pmic_ver;

#define PMIC_ARB_CHANNEL_NUM    0
#define PMIC_ARB_OWNER_ID       0

#define WDOG_DEBUG_DISABLE_BIT  17

#define CE_INSTANCE             2
#define CE_EE                   1
#define CE_FIFO_SIZE            64
#define CE_READ_PIPE            3
#define CE_WRITE_PIPE           2
#define CE_ARRAY_SIZE           20

#define REBOOT_MODE_NONE     0x00000000
#define REBOOT_MODE_FASTBOOT 0x77665500
#define REBOOT_MODE_HLOS     0x77665501
#define REBOOT_MODE_RECOVERY 0x77665502

static uint32_t mmc_sdc_base[] =
	{ MSM_SDC1_BASE, MSM_SDC2_BASE, MSM_SDC3_BASE, MSM_SDC4_BASE };

void target_early_init(void)
{
#if WITH_DEBUG_UART
	if (board_hardware_version() == BOARD_REVISION_P0)
		uart_dm_init(7, 0, BLSP2_UART1_BASE);
	else
		uart_dm_init(1, 0, BLSP1_UART1_BASE);
#endif
}

#define VOL_UP_PMIC_GPIO 4
#define VOL_DN_PMIC_GPIO 1

/* Return 1 if the specified GPIO is pressed (low) */
static int target_pmic_gpio_button_pressed(int gpio_number)
{
	uint8_t status = 0;
	struct pm8x41_gpio gpio;

	/* Configure the GPIO */
	gpio.direction = PM_GPIO_DIR_IN;
	gpio.function  = 0;
	gpio.pull      = PM_GPIO_PULL_UP_30;
	gpio.vin_sel   = 2;

	pm8x41_gpio_config(gpio_number, &gpio);

	/* delay to work around slow rise on P0 volume up -- remove when fixed */
	udelay(200);

	/* Get status of the GPIO */
	pm8x41_gpio_get(gpio_number, &status);

	return !status; /* active low */
}

/* Return 1 if vol_up pressed */
int target_volume_up()
{
	return target_pmic_gpio_button_pressed(VOL_UP_PMIC_GPIO);
}

/* Return 1 if vol_down pressed */
int target_volume_down()
{
	return target_pmic_gpio_button_pressed(VOL_DN_PMIC_GPIO);
}

static void target_keystatus()
{
	keys_init();

	if(target_volume_down())
		keys_post_event(KEY_VOLUMEDOWN, 1);

	if(target_volume_up())
		keys_post_event(KEY_VOLUMEUP, 1);
}

/* Set up params for h/w CE. */
void target_crypto_init_params()
{
	struct crypto_init_params ce_params;

	/* Set up base addresses and instance. */
	ce_params.crypto_instance  = CE_INSTANCE;
	ce_params.crypto_base      = MSM_CE2_BASE;
	ce_params.bam_base         = MSM_CE2_BAM_BASE;

	/* Set up BAM config. */
	ce_params.bam_ee           = CE_EE;
	ce_params.pipes.read_pipe  = CE_READ_PIPE;
	ce_params.pipes.write_pipe = CE_WRITE_PIPE;

	/* Assign buffer sizes. */
	ce_params.num_ce           = CE_ARRAY_SIZE;
	ce_params.read_fifo_size   = CE_FIFO_SIZE;
	ce_params.write_fifo_size  = CE_FIFO_SIZE;

	crypto_init_params(&ce_params);
}

crypto_engine_type board_ce_type(void)
{
	return CRYPTO_ENGINE_TYPE_HW;
}

void target_init(void)
{
	uint32_t base_addr;
	uint8_t slot;

	dprintf(INFO, "target_init() - platform %d, board %d, board version %d\n",
		board_platform_id(), board_hardware_id(), board_hardware_version());

	spmi_init(PMIC_ARB_CHANNEL_NUM, PMIC_ARB_OWNER_ID);

	/* Save PM8941 version info. */
	pmic_ver = pm8x41_get_pmic_rev();

	target_keystatus();

	if (target_use_signed_kernel())
		target_crypto_init_params();
	/* Display splash screen if enabled */
#if DISPLAY_SPLASH_SCREEN
	dprintf(INFO, "Display Init: Start\n");
	display_init();
	dprintf(INFO, "Display Init: Done\n");
#endif

	/* Trying Slot 1*/
	slot = 1;
	base_addr = mmc_sdc_base[slot - 1];
	if (mmc_boot_main(slot, base_addr))
	{

		/* Trying Slot 2 next */
		slot = 2;
		base_addr = mmc_sdc_base[slot - 1];
		if (mmc_boot_main(slot, base_addr)) {
			dprintf(CRITICAL, "mmc init failed!");
			ASSERT(0);
		}
	}
}

unsigned board_machtype(void)
{
	return target_id;
}

/* Do any target specific intialization needed before entering fastboot mode */
void target_fastboot_init(void)
{
	/* Set the BOOT_DONE flag in PM8921 */
	pm8x41_set_boot_done();

	static char board_revision_string[9];

	snprintf(board_revision_string, 9, "%08x", board_hardware_version());
	fastboot_publish("revision", board_revision_string);
}

/* Detect the target type.
 * This function sets platform_hw which is the board type and the target type.
 * Board version is read through ADC in SBL1 and passed-in through SMEM */
void target_detect(struct board_data *board)
{
	switch (board_hardware_version())
	{
	case BOARD_REVISION_P0:
		board->platform_hw = HW_PLATFORM_URSA_P0;
		board->target = LINUX_MACHTYPE_URSA_P0;
		break;

	case BOARD_REVISION_PRE_P1:
		if (board_soc_version() == BOARD_SOC_VERSION2)
		{
			/* *HACK* Support for pre-P1 plus
			 * Load P1 device-tree for pre-P1 with v2 SOCs.
			 */
			board->platform_hw = HW_PLATFORM_URSA_P1;
			board->target = LINUX_MACHTYPE_URSA_P1;
		}
		else
		{
			board->platform_hw = HW_PLATFORM_URSA_PRE_P1;
			board->target = LINUX_MACHTYPE_URSA_PRE_P1;
		}
		break;

	case BOARD_REVISION_P1:
	case BOARD_REVISION_P0_5:
		board->platform_hw = HW_PLATFORM_URSA_P1;
		board->target = LINUX_MACHTYPE_URSA_P1;
		break;

	case BOARD_REVISION_P2:
		board->platform_hw = HW_PLATFORM_URSA_P2;
		board->target = LINUX_MACHTYPE_URSA_P2;
		break;

	default:
		/* Board version is invalid. Will result in boot failure */
		board->platform_hw = board->target = 0;
		break;
	}

	target_id = board->target;
}

/* Detect the modem type */
void target_baseband_detect(struct board_data *board)
{
	uint32_t platform;
	uint32_t platform_subtype;

	platform = board->platform;
	platform_subtype = board->platform_subtype;

	/*
	 * Look for platform subtype if present, else
	 * check for platform type to decide on the
	 * baseband type
	 */
	switch(platform_subtype) {
	case HW_PLATFORM_SUBTYPE_UNKNOWN:
		break;
	default:
		dprintf(CRITICAL, "Platform Subtype : %u is not supported\n",platform_subtype);
		ASSERT(0);
	};

	switch(platform) {
	case MSM8974:
		board->baseband = BASEBAND_MSM;
		break;
	case APQ8074:
		board->baseband = BASEBAND_APQ;
		break;
	default:
		dprintf(CRITICAL, "Platform type: %u is not supported\n",platform);
		ASSERT(0);
	};
}

void target_serialno(unsigned char *buf)
{
	unsigned int serialno;
// ACOS_MOD_BEGIN
#ifdef WITH_ENABLE_IDME
	int rc = idme_get_var_external("serial", (char *)buf, 16);
	buf[16] = '\0';

	dprintf(INFO, "Serial %s, %u, %u, rc=%d\n", buf, buf[0], buf[1], rc);
	if(!rc && buf[0] != '0' && buf[1] != '\0')
		return;
#endif
// ACOS_MOD_END
	if (target_is_emmc_boot()) {
		serialno = mmc_get_psn();
		snprintf((char *)buf, 13, "%x", serialno);
	}
}

unsigned check_reboot_mode(void)
{
	static int restart_reason_valid = 0;
	static uint32_t restart_reason = 0;
	uint32_t soc_ver = 0;
	uint32_t restart_reason_addr;

	soc_ver = board_soc_version();

	if (soc_ver >= BOARD_SOC_VERSION2)
		restart_reason_addr = RESTART_REASON_ADDR_V2;
	else
		restart_reason_addr = RESTART_REASON_ADDR;

	if (!restart_reason_valid) {
		/* Read reboot reason and scrub it */
		restart_reason = readl(restart_reason_addr);
		writel(0x00, restart_reason_addr);
		restart_reason_valid = 1;
	}

	return restart_reason;
}

void reboot_device(unsigned reboot_reason)
{
	uint32_t soc_ver = 0;

	soc_ver = board_soc_version();

	/* Write the reboot reason */
	if (soc_ver >= BOARD_SOC_VERSION2)
		writel(reboot_reason, RESTART_REASON_ADDR_V2);
	else
		writel(reboot_reason, RESTART_REASON_ADDR);

	/* Configure PMIC for warm reset */
	if (pmic_ver == PMIC_VERSION_V2)
		pm8x41_v2_reset_configure(PON_PSHOLD_WARM_RESET);
	else
		pm8x41_reset_configure(PON_PSHOLD_WARM_RESET);

	/* Disable Watchdog Debug.
	 * Required becuase of a H/W bug which causes the system to
	 * reset partially even for non watchdog resets.
	 */
	writel(readl(GCC_WDOG_DEBUG) & ~(1 << WDOG_DEBUG_DISABLE_BIT), GCC_WDOG_DEBUG);

	dsb();

	/* Wait until the write takes effect. */
	while(readl(GCC_WDOG_DEBUG) & (1 << WDOG_DEBUG_DISABLE_BIT));

	/* Drop PS_HOLD for MSM */
	writel(0x00, MPM2_MPM_PS_HOLD);

	mdelay(5000);

	dprintf(CRITICAL, "Rebooting failed\n");
}

#define PARAMETER_OVERRIDE_C 0x82
/* Do URSA specific usb initialization */
void target_usb_init(void)
{
	uint32_t boardrev = board_hardware_version();
	switch (boardrev)
	{
	case BOARD_REVISION_P1:
	case BOARD_REVISION_P0_5:
		/* set PARAMETER_OVERRIDE_C register value to 0x2b */
		ulpi_write(0x2b, PARAMETER_OVERRIDE_C);
		break;

	default:
		/* Board version is invalid or no need for special USB init work */
		break;
	}
}

/* Returns 1 if target supports continuous splash screen. */
int target_cont_splash_screen()
{
	dprintf(CRITICAL, "Continuous splash screen disabled, because it is not supported in CMD mode\n");
	return 0;

	switch(board_hardware_version())
	{
		case BOARD_REVISION_P0:
			dprintf(SPEW, "Target_cont_splash=0\n");
			return 0;
		default:
			dprintf(SPEW, "Target_cont_splash=1\n");
			return 1;
			break;
	}
}

unsigned target_pause_for_battery_charge(void)
{
#if !FACTORY_MODE
	unsigned reboot_mode = check_reboot_mode();
	uint8_t pon_reason = pm8x41_get_pon_reason();

	dprintf(INFO, "REBOOT_INFO: %08X:%02X\n", reboot_mode, pon_reason);

#ifdef WITH_ENABLE_IDME
	if (idme_boot_mode() == IDME_BOOTMODE_DIAG) {
		/* don't do charging for factory builds */
		return 0;
	}
#endif

	if (reboot_mode == REBOOT_MODE_NONE ||
	    reboot_mode == REBOOT_MODE_FASTBOOT ||
	    reboot_mode == REBOOT_MODE_HLOS ||
	    reboot_mode == REBOOT_MODE_RECOVERY) {
		/* this boot has a specific purpose, don't divert to charging */
		return 0;
	}

	if (keys_get_state(KEY_HOME) ||
	    keys_get_state(KEY_VOLUMEUP) ||
	    keys_get_state(KEY_BACK) ||
	    keys_get_state(KEY_VOLUMEDOWN)) {
		/* keys are down, aboot is going to divert into fastboot or recovery */
		return 0;
	}

	/* note: USB_CHG is also returned for wall charging; DC_CHG seems unused */
	if ((pon_reason == USB_CHG) || (pon_reason == DC_CHG)) {
		/* no boot purpose and USB is plugged in -- divert to charging */
		if (board_hardware_version() != BOARD_REVISION_P0)
			return 1;
	}
#endif // !FACTORY_MODE

	return 0;
}

void shutdown_device()
{
	dprintf(CRITICAL, "Going down for shutdown.\n");

	/* Configure PMIC for shutdown. */
	if (pmic_ver == PMIC_VERSION_V2)
		pm8x41_v2_reset_configure(PON_PSHOLD_SHUTDOWN);
	else
		pm8x41_reset_configure(PON_PSHOLD_SHUTDOWN);

	/* Drop PS_HOLD for MSM */
	writel(0x00, MPM2_MPM_PS_HOLD);

	mdelay(5000);

	dprintf(CRITICAL, "Shutdown failed\n");

}

void target_enter_emergency_download(void)
{
    unsigned int *p1 = 0xfe800000 + 0x5000 + 0x1000 - 0x20;
    unsigned int *p2 = p1 + 1;
    unsigned int *p3 = p1 + 2;

    const unsigned int cookie1 = 0x188d442f;
    const unsigned int cookie2 = 0x06d73e09;
    const unsigned int cookie3 = 0x88888888;

    dprintf(CRITICAL, "Setting up emergency download mode...\n");

    writel(cookie1, p1);
    writel(cookie2, p2);
    writel(cookie3, p3);

    dprintf(CRITICAL, "Rebooting into emergency download mode...\n");

    thread_sleep(10);
    dmb();

    while (1)
        reboot_device(0);
}

const unsigned char *target_certificate(void)
{
	return certificate_data;
}

int target_certificate_size(void)
{
	return sizeof(certificate_data);
}
