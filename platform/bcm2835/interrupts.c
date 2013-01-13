/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <err.h>
#include <sys/types.h>
#include <debug.h>
#include <reg.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <platform/bcm2835.h>
#include "platform_p.h"

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table[MAX_INT];

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
	if (vector >= MAX_INT)
		panic("register_int_handler: vector out of range %d\n", vector);

	enter_critical_section();

	int_handler_table[vector].handler = handler;
	int_handler_table[vector].arg = arg;

	exit_critical_section();
}

void platform_init_interrupts(void)
{
#if 0
	GICREG(0, SETENABLE0) = 0;
	GICREG(0, SETENABLE1) = 0;
	GICREG(0, SETENABLE2) = 0;
	GICREG(0, CLRPEND0) = 0xffffffff;
	GICREG(0, CLRPEND1) = 0xffffffff;
	GICREG(0, CLRPEND2) = 0xffffffff;

	GICREG(0, CONTROL) = 1; // enable GIC0
	GICREG(0, DISTCONTROL) = 1; // enable GIC0
#endif
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	enter_critical_section();

//	gic_set_enable(vector, false);

	exit_critical_section();

	return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	enter_critical_section();

//	gic_set_enable(vector, true);

	exit_critical_section();

	return NO_ERROR;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
	unsigned int vector = 0;
#if 0
	// get the current vector
	unsigned int vector = GICREG(0, INTACK) & 0x3ff;

	// see if it's spurious
	if (vector == 0x3ff) {
		GICREG(0, EOI) = 0x3ff; // XXX is this necessary?
		return INT_NO_RESCHEDULE;
	}
#endif

	THREAD_STATS_INC(interrupts);
	KEVLOG_IRQ_ENTER(vector);

//	printf("platform_irq: spsr 0x%x, pc 0x%x, currthread %p, vector %d\n", frame->spsr, frame->pc, current_thread, vector);

	// deliver the interrupt
	enum handler_return ret;

	ret = INT_NO_RESCHEDULE;
	if (int_handler_table[vector].handler)
		ret = int_handler_table[vector].handler(int_handler_table[vector].arg);

//	GICREG(0, EOI) = vector;

//	printf("platform_irq: exit %d\n", ret);

	KEVLOG_IRQ_EXIT(vector);

	return ret;
}

void platform_fiq(struct arm_iframe *frame)
{
	PANIC_UNIMPLEMENTED;
}

