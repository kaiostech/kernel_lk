/*
 * Copyright (c) 2014 Travis Geiselbrecht
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

#include <kernel/mp.h>

#include <stdlib.h>
#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <arch/mp.h>
#include <kernel/spinlock.h>

#define LOCAL_TRACE 0

struct mp_mailbox {
    bool active;
};

static struct mp_mailbox mbx[SMP_MAX_CPUS];

void mp_init(void)
{
}

void mp_mbx_reschedule(mp_cpu_num_t target)
{
#if WITH_SMP
    uint local_cpu = arch_curr_cpu_num();

    LTRACEF("target %d\n", target);

    switch (target) {
        case MP_CPU_ALL:
        case MP_CPU_ALL_BUT_LOCAL:
            break;
        default:
            if (target >= SMP_MAX_CPUS)
                goto out;
            if (target == local_cpu)
                PANIC_UNIMPLEMENTED;
            break;
    }

    arch_mp_send_ipi(target, MP_IPI_RESCHEDULE);

out:
    ;
#endif
}

void mp_set_curr_cpu_active(bool active)
{
    mbx[arch_curr_cpu_num()].active = active;
}

#if WITH_SMP
enum handler_return mp_mbx_reschedule_irq(void)
{
    LTRACEF("cpu %u\n", arch_curr_cpu_num());
    return mbx[arch_curr_cpu_num()].active ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}
#endif

