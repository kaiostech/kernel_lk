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
#include <lp855x.h>
#include "target_cert.h"
#include "target_production_cert.h"
#include <target/display.h>
#if WITH_FBGFX_SPLASH
#include <dev/fbgfx.h>
struct fbgfx_image splash;
extern struct fbgfx_image image_boot_Kindle;
#endif
#ifdef WITH_ENABLE_IDME
#include <idme.h> // ACOS_MOD_ONELINE
#endif
#define FASTBOOT_MODE   0x77665500

#define HW_PLATFORM_APOLLO     20 /* these needs to match with apollo.dts */
#define LINUX_MACHTYPE_APOLLO  20

#define MIN_BOOT_BATTERY_CAPACITY 5

extern struct udc_device *get_udc_device(void);
extern  bool target_use_signed_kernel(void);
extern void check_battery_condition(int min_capacity);
extern void charge_mode_loop(void);
extern int get_display_image_type();
extern void show_image(Image_types type);

void check_charge_mode(void);
void shutdown_device(void);
void target_enter_emergency_download(void);

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

static uint32_t mmc_sdc_base[] =
	{ MSM_SDC1_BASE, MSM_SDC2_BASE, MSM_SDC3_BASE, MSM_SDC4_BASE };

void target_early_init(void)
{
#if WITH_DEBUG_UART
	uart_dm_init(1, 0, BLSP1_UART1_BASE);
#endif
}

#define VOL_UP_PMIC_GPIO 4
#define VOL_DN_PMIC_GPIO 1

static char emmc_size[16];
static char emmc_mid[8];

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

	/* BAM is initialized by TZ for this platform.
	 * Do not do it again as the initialization address space
	 * is locked.
	 */
	ce_params.do_bam_init      = 0;

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
	int i = 0;

	dprintf(INFO, "target_init()\n");

	spmi_init(PMIC_ARB_CHANNEL_NUM, PMIC_ARB_OWNER_ID);

	/* Save PM8941 version info. */
	pmic_ver = pm8x41_get_pmic_rev();

	target_keystatus();

	/* Workaround for S2 bite bug (PMIC V2 10sec reset instead of shutdown) */
	if ((pm8x41_reg_read(0x80c) & (1 << 7)) &&
			(pmic_ver <= PM8X41_VERSION_V2)) {
		dprintf(INFO, "Shutting off device due to S2 bite bug...\n");
		for (i = 0; i < 50; i++)
			mdelay(100);
		dprintf(INFO, "Actually shutting down SoC now...\n");
		shutdown_device();
	}

	check_battery_condition(MIN_BOOT_BATTERY_CAPACITY);

	check_charge_mode();

	if (target_use_signed_kernel())
		target_crypto_init_params();

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

	/* Display splash screen if enabled */
#if DISPLAY_SPLASH_SCREEN

#if WITH_FBGFX_SPLASH
	/* if fbcon.c does not hardcode 'splash' we can skip this memcpy */
	memcpy(&splash, &image_boot_Kindle, sizeof(struct fbgfx_image));
#endif
	dprintf(INFO, "Display Init: Start\n");

	if (IMAGE_NONE == get_display_image_type())
		show_image(IMAGE_LOGO);

	dprintf(INFO, "Backlight Start\n");
	lp855x_bl_on();

#endif

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

	struct mmc_card *card = get_mmc_card();

	sprintf(emmc_size, "%lld", card->capacity);
	fastboot_publish("emmc_size", emmc_size);

	sprintf(emmc_mid, "0x%x", card->cid.mid);
	fastboot_publish("emmc_mid", emmc_mid);
}

/* Detect the target type */
void target_detect(struct board_data *board)
{
//	board->platform = MSM8974;
	board->platform_hw = HW_PLATFORM_APOLLO;
	board->target = LINUX_MACHTYPE_APOLLO;
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

int emmc_recovery_init(void)
{
	int rc;
	rc = _emmc_recovery_init();
	return rc;
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
	if (pmic_ver == PM8X41_VERSION_V2)
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

/* Returns 1 if target supports continuous splash screen. */
int target_cont_splash_screen()
{
	switch(board_hardware_id())
	{
		case HW_PLATFORM_SURF:
		case HW_PLATFORM_MTP:
		case HW_PLATFORM_FLUID:
			dprintf(SPEW, "Target_cont_splash=1\n");
			return 1;
			break;
		default:
			dprintf(SPEW, "Target_cont_splash=0\n");
			return 0;
	}
}

void check_charge_mode(void)
{
	unsigned reboot_mode = check_reboot_mode();

	if (keys_get_state(KEY_HOME) ||
	    target_volume_up() ||
	    keys_get_state(KEY_BACK) ||
	    target_volume_down()) {
		/* keys are down, aboot is going to divert into fastboot or recovery */
		return;
	}

	/* Divert to charging */
	if (reboot_mode == REBOOT_MODE_CHARGE) {
		charge_mode_loop();
	} else if (reboot_mode == REBOOT_MODE_EMERGENCY) {
		target_enter_emergency_download();
	}
}

unsigned target_pause_for_battery_charge(void)
{
	return 0;
}

void shutdown_device(void)
{
	dprintf(CRITICAL, "Going down for shutdown.\n");

	/* Configure PMIC for shutdown. */
	if (pmic_ver == PM8X41_VERSION_V2)
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

const unsigned char *target_production_certificate(void)
{
	return production_certificate_data;
}

int target_production_certificate_size(void)
{
	return sizeof(production_certificate_data);
}

int target_production_gpio(void)
{
	return 92;
}

#define SMB349_CHG_ENABLE_GPIO	10
int charge_enabled = 1;

void target_set_charging(int enable)
{
	struct pm8x41_gpio gpio;

	/* Configure the GPIO */
	gpio.direction = PM_GPIO_DIR_OUT;
	gpio.function  = 0;

	pm8x41_gpio_config(SMB349_CHG_ENABLE_GPIO, &gpio);

	/* Set GPIO value */
	pm8x41_gpio_set(SMB349_CHG_ENABLE_GPIO,
			enable ? PM_GPIO_FUNC_LOW : PM_GPIO_FUNC_HIGH);

	charge_enabled = enable;
}

int target_get_charging(void)
{
	return charge_enabled;
}
