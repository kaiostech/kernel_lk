/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
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
#include <assert.h>
#include <debug.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <err.h>
#include <lk/init.h>
#include <stdlib.h>
#include <trace.h>
#include <kernel/event.h>

#define USB_XFER_SIZE (512)
#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

static event_t txevt = EVENT_INITIAL_VALUE(txevt, 0, EVENT_FLAG_AUTOUNSIGNAL);
static event_t rxevt = EVENT_INITIAL_VALUE(rxevt, 0, EVENT_FLAG_AUTOUNSIGNAL);

static volatile bool online = false;

static status_t usb_xmit_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
	event_signal(&txevt, false);
	return 0;
}

static status_t usb_recv_cplt_cb(ep_t endpoint, usbc_transfer_t *t)
{
	event_signal(&rxevt, false);
	return 0;
}

void usb_xmit(void *data, uint len)
{
	usbc_transfer_t transfer = {
		.callback = &usb_xmit_cplt_cb,
		.result   = 0,
		.buf      = data,
		.buflen   = len,
		.bufpos   = 0,
		.extra    = 0 
	};

	usbc_queue_tx(1, &transfer);
    event_wait(&txevt);
}

ssize_t usb_recv(void *data, uint len, lk_time_t timeout)
{
    usbc_transfer_t transfer = {
    	.callback = &usb_recv_cplt_cb,
    	.result = 0,
    	.buf = data,
    	.buflen = len,
    	.bufpos = 0,
    	.extra = 0,
	};

    usbc_queue_rx(1, &transfer);
    status_t res = event_wait_timeout(&rxevt, timeout);

    if (res != NO_ERROR) {
    	return res;
    }

   	char *ascii = transfer.buf;
   	ascii[transfer.bufpos] = 0;
    printf("Data: %s\n", ascii);

    return NO_ERROR;
}

static status_t register_callback(
	void *cookie, usb_callback_op_t op, 
	const union usb_callback_args *args
) {
    printf("cookie %p, op %u, args %p\n", cookie, op, args);

    if (op == USB_CB_ONLINE) {
        usbc_setup_endpoint(1, USB_IN, 0x40);
        usbc_setup_endpoint(1, USB_OUT, 0x40);
        printf("USB Online.\n");
        online = true;
    } else {
        // printf("USB Offline.\n");
    	// online = false;
    }
    return NO_ERROR;
}


static void moot_init(const struct app_descriptor *app)
{
	/* build a descriptor for it */
	uint8_t if_descriptor[] = {
	    0x09,           /* length */
	    INTERFACE,      /* type */
	    1,  /* interface num */ // TODO(gkalsi)
	    0x00,           /* alternates */
	    0x02,           /* endpoint count */
	    0xff,           /* interface class */
	    0xff,           /* interface subclass */
	    0x00,           /* interface protocol */
	    0x00,           /* string index */

	    /* endpoint 1 IN */
	    0x07,           /* length */
	    ENDPOINT,       /* type */
	    0x1 | 0x80,     /* address: 1 IN */ // TODO(gkalsi)
	    0x02,           /* type: bulk */
	    W(64),          /* max packet size: 64 */
	    00,             /* interval */

	    /* endpoint 1 OUT */
	    0x07,           /* length */
	    ENDPOINT,       /* type */
	    0x1,            /* address: 1 OUT */ // TODO(gkalsi)
	    0x02,           /* type: bulk */
	    W(64),          /* max packet size: 64 */
	    00,             /* interval */
	};

	usb_append_interface_lowspeed(if_descriptor, sizeof(if_descriptor));
	usb_append_interface_highspeed(if_descriptor, sizeof(if_descriptor));

	usb_register_callback(&register_callback, NULL);
}

static void moot_entry(const struct app_descriptor *app, void *args)
{
	uint8_t *buf = malloc(USB_XFER_SIZE);
	while(true) {
		if (!online) {
			thread_yield();
			continue;
		}

		printf("Waiting for USB recv\n");
		status_t rc = usb_recv(buf, USB_XFER_SIZE, INFINITE_TIME);
		printf("usb_recv returned %d, read %d bytes.\n", rc, USB_XFER_SIZE);
	}
}

APP_START(moot)
	.init = moot_init,
	.entry = moot_entry,
APP_END
