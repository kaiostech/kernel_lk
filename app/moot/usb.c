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

#include <app/moot/usb.h>

#include <dev/usb.h>
#include <dev/usbc.h>
#include <dev/udc.h>
#include <err.h>
#include <kernel/thread.h>
#include <platform.h>
#include <stdint.h>
#include <stdlib.h>
#include <trace.h>
#include <kernel/event.h>
#include <lib/bio.h>

#define LOCAL_TRACE 0
#define COMMAND_MAGIC (0x4d4f4f54)  // MOOT
#define RESP_MAGIC    (0x52455350)  // RESP

// TODO(gkalsi): These are all platform level constructs and need to be pulled
// 				 out into some platform specific code.
#define TOTAL_FLASH_LEN   (1024 * 1024)
#define BTLDR_BASE        (0x0)
#define BTLDR_LEN         (128 * 1024)
#define SYSTEM_BASE       (BTLDR_LEN)
#define SYSTEM_LEN        (TOTAL_FLASH_LEN - BTLDR_LEN)
#define SYSTEM_FLASH_NAME ("flash0")


///////////////////////////////////// BOGUS ////////////////////////////////////
// #define SYSTEM_BASE       (768 * 1024)
// #define SYSTEM_LEN        (256 * 1024)
///////////////////////////////////// BOGUS ////////////////////////////////////


#define USB_XFER_SIZE (512)
#define W(w) (w & 0xff), (w >> 8)
#define W3(w) (w & 0xff), ((w >> 8) & 0xff), ((w >> 16) & 0xff)

// How long should we wait for activity on USB before we continue to boot?
#define USB_BOOT_TIMEOUT (12000)

static const uint8_t if_descriptor[] = {
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

/* Response Codes to signal back to the host. */
#define USB_RESP_NO_ERROR             (0x0)
#define USB_RESP_BAD_DATA_LEN         (0x1)
#define USB_RESP_SYS_IMAGE_TOO_BIG    (0x2)
#define USB_RESP_READY_FOR_DATA       (0x3)
#define USB_RESP_ERR_OPEN_SYS_FLASH   (0x4)
#define USB_RESP_ERR_ERASE_SYS_FLASH  (0x5)
#define USB_RESP_ERR_WRITE_SYS_FLASH  (0x6)
#define USB_RESP_ERR_WRITE_SYS_FLASH  (0x6)
#define USB_RESP_UNKNOWN_COMMAND      (0x7)
#define USB_RESP_BAD_MAGIC            (0x8)

/* Bootloader commands */
#define USB_CMD_WRITE_IMAGE (0x1)
#define USB_CMD_BOOT        (0x2)
#define USB_CMD_QUERY       (0x3)
#define USB_CMD_REBOOT      (0x4)

typedef struct cmd_header {
	uint32_t magic;
	uint32_t cmd;
	uint32_t arg;
} cmd_header_t;

typedef struct cmd_response {
	uint32_t magic;
	uint32_t code;
} cmd_response_t;

// USB Functions
static void usb_xmit(void *data, size_t len);
static status_t usb_recv(void *data, size_t len, lk_time_t timeout, size_t *actual);
static status_t usb_xmit_cplt_cb(ep_t endpoint, usbc_transfer_t *t);
static status_t usb_recv_cplt_cb(ep_t endpoint, usbc_transfer_t *t);
static status_t usb_register_cb(
	void* cookie, usb_callback_op_t op, const union usb_callback_args *args);

static uint8_t buffer[4096];
static event_t txevt = EVENT_INITIAL_VALUE(txevt, 0, EVENT_FLAG_AUTOUNSIGNAL);
static event_t rxevt = EVENT_INITIAL_VALUE(rxevt, 0, EVENT_FLAG_AUTOUNSIGNAL);
static volatile bool online = false;

// Command processor that handles USB boot commands.
static void handle(const void *data, const size_t n, cmd_response_t *resp)
{
	// TODO(gkalsi): Fail harder here.
	if (!resp) {
		return;
	}

	resp->magic = RESP_MAGIC;

	// Make sure we have enough data.
	if (n < sizeof(cmd_header_t)) {
		resp->code = USB_RESP_BAD_DATA_LEN;
		return;
	}

	cmd_header_t *header = (cmd_header_t *)data;
	if (header->magic != COMMAND_MAGIC) {
		resp->code = USB_RESP_BAD_MAGIC;
		return;
	}


	size_t image_length;

	switch (header->cmd) {
		case USB_CMD_WRITE_IMAGE:
			image_length = header->arg;
			if (image_length > SYSTEM_LEN) {
				resp->code = USB_RESP_SYS_IMAGE_TOO_BIG;
				break;
			}

			// Make space on the flash for the data.
			bdev_t *dev = bio_open(SYSTEM_FLASH_NAME);
			if (!dev) {
				resp->code = USB_RESP_ERR_OPEN_SYS_FLASH;
				break;
			}

			ssize_t n_bytes_erased = bio_erase(dev, SYSTEM_BASE, image_length);
			if (n_bytes_erased < image_length) {
				resp->code = USB_RESP_ERR_OPEN_SYS_FLASH;
				break;
			}

			// Signal to the host to start sending the image over.
			printf("Signal to send data\n");
			resp->code = USB_RESP_READY_FOR_DATA;
			usb_xmit((void *)resp, sizeof(*resp));
			off_t addr = SYSTEM_BASE;
			printf("Signal sent!\n");

			while (image_length > 0) {
				size_t xfer = (image_length > sizeof(buffer)) ? 
									sizeof(buffer) : image_length;

				size_t bytes_received;
				printf("Get %u bytes\n", xfer);
				usb_recv(buffer, xfer, INFINITE_TIME, &bytes_received);
				printf("Got %u bytes\n", bytes_received);

				ssize_t written = bio_write(dev, buffer, addr, bytes_received);
				if (written != bytes_received) {
					printf("Expected %u, actual %ld, dev %p, buffer %p, addr %lld\n", bytes_received, written, dev, buffer, addr);
					resp->code = USB_RESP_ERR_WRITE_SYS_FLASH;
					break; // TODO(gkalsi): This only breaks us out of the while loop
				}

				addr += written;
				image_length -= written;
			}
 
			resp->code = USB_RESP_NO_ERROR;

			break;
		case USB_CMD_BOOT:

			break;
		case USB_CMD_QUERY:

			break;
		case USB_CMD_REBOOT:

			break;
		default:
			resp->code = USB_RESP_UNKNOWN_COMMAND;
			break;
	}

	return;
}

void init_usb_boot(void)
{
	usb_append_interface_lowspeed(if_descriptor, sizeof(if_descriptor));
	usb_append_interface_highspeed(if_descriptor, sizeof(if_descriptor));

	usb_register_callback(&usb_register_cb, NULL);
}

bool attempt_usb_boot(void)
{
	printf("attempting usb boot\n");
	uint8_t *buf = malloc(USB_XFER_SIZE);
	bool should_continue_boot = true;

	lk_time_t start = current_time();
	lk_time_t timeout = USB_BOOT_TIMEOUT;
	size_t bytes_received;

	while (current_time() - start < timeout) {
		if (!online) {
			thread_yield();
			continue;
		}

		status_t r = usb_recv(buf, USB_XFER_SIZE, timeout, &bytes_received);
		if (r == ERR_TIMED_OUT) {
			should_continue_boot = true;
			printf("recv timed out\n");
			goto finish;
		} else if (r == NO_ERROR) {
			printf("Got %u bytes\n", bytes_received);
			// Somebody tried to talk to us over USB, they own the boot now.
			cmd_response_t response;
			handle(buf, bytes_received, &response);
			usb_xmit((void *)&response, sizeof(response));
			timeout = INFINITE_TIME;
		}
	}	

finish:
	printf("Booting!\n");
	free(buf);
	return should_continue_boot;
}

static status_t usb_register_cb(
	void* cookie, 
	usb_callback_op_t op, 
	const union usb_callback_args *args
) {
	if (op == USB_CB_ONLINE) {
		usbc_setup_endpoint(1, USB_IN, 0x40);
		usbc_setup_endpoint(1, USB_OUT, 0x40);
		online = true;
	}
	// else if (op == UDC_EVENT_OFFLINE) {
	// 	online = 0;
	// }
	return NO_ERROR;
}

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

static void usb_xmit(void *data, size_t len)
{
	usbc_transfer_t transfer = {
		.callback = &usb_xmit_cplt_cb,
		.result   = 0,
		.buf      = data,
		.buflen   = len,
		.bufpos   = 0,
		.extra    = 0,
	};

	usbc_queue_tx(1, &transfer);
	event_wait(&txevt);
}

static status_t usb_recv(void *data, size_t len, lk_time_t timeout, size_t *actual)
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
		// TODO(gkalsi): Cancel the USB txn?
		return res;
	}

	*actual = transfer.bufpos;
	return NO_ERROR;
}