/*
 * Copyright (c) 2016 Gurjant Kalsi <me@gurjantkalsi.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <app.h>
#include <app/moot/usb.h>
#include <assert.h>
#include <debug.h>
#include <err.h>
#include <kernel/event.h>
#include <lk/init.h>
#include <stdlib.h>
#include <trace.h>


static void moot_init(const struct app_descriptor *app)
{
    // Initialize our boot subsystems.
    init_usb_boot();

}

static void moot_entry(const struct app_descriptor *app, void *args)
{
    do {
        // Wait a few seconds for the host to try to talk to us over USB.
        if (attempt_usb_boot())
            break;

        // // Check the SPIFlash for an upgrade image.
        // if (attempt_spifs_upgrade())
        //  break;

    } while (false);


    // Ensure the integrity of the system image.
    // verify_system_image();

    // Boot the main system image.
    // do_boot();
}

APP_START(moot)
.init = moot_init,
 .entry = moot_entry,
  APP_END
