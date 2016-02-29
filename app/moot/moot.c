

#include <app.h>
#include <assert.h>
#include <debug.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <err.h>
#include <lk/init.h>
#include <stdlib.h>
#include <trace.h>
#include <kernel/event.h>

static void moot_init(const struct app_descriptor *app) {
	init_moot_usb();
}

static void moot_entry(const struct app_descriptor *app, void *args)
{
	do {
		// Wait a few seconds for the host to try to talk to us over USB.
		if (attempt_usb_boot())
			break;

		// Check the SPIFlash for an upgrade image.
		if (attempt_spifs_upgrade())
			break;

	} while (false);


	// Ensure the integrity of the system image.
	verify_system_image();

	// Boot the main system image.
	do_boot();
}

APP_START(moot)
	.init = moot_init,
	.entry = moot_entry,
APP_END
