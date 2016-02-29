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

#include <app/moot/cmd.h>
#include <trace.h>

#define MOOT_MAGIC (0x6d6f6f74) // "moot"

/* List of supported bootloader commands */
static status_t cmd_boot(void *, size_t);	     // Boot the operating system.
static status_t cmd_flash(void *, size_t); 	     // Flash the operating system.
static status_t cmd_query(void *, size_t);		 // Query device info. 

#define CMD_BOOT       (0x01)
#define CMD_FLASH      (0x02)
#define CMD_QUERY      (0x03)

status_t cmd_dispatch(void *data, size_t len)
{
	if (!data)
		return ERR_INVALID_ARGS;

	if (len < sizeof(cmd_header_t))
		return ERR_BAD_LEN;

	cmd_header_t *cmd = (cmd_header_t *)data;
	if (cmd->moot_magic != MOOT_MAGIC) 
		return ERR_NOT_VALID;

	status_t rc = NO_ERROR;
	switch (cmd->cmd_id) {
		case CMD_BOOT:
			rc = cmd_boot();
			break;
		case CMD_FLASH:
			rc = cmd_flash();
			break;
		case CMD_QUERY:
			rc = cmd_query();
			break;
		default:
			rc = ERR_CMD_UNKNOWN;
			break;
	}
	return rc;
}

static status_t cmd_boot(void *data, size_t len)
{
	return NO_ERROR;
}

static status_t cmd_flash(void *data, size_t len)
{
	return NO_ERROR;
}

static status_t cmd_query(void *data, size_t len)
{
	return NO_ERROR;
}
