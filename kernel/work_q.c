/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Workqueue support functions
 */

#include <kernel_structs.h>
#include <wait_q.h>
#include <errno.h>

// KID 20170717
static void work_q_main(void *work_q_ptr, void *p2, void *p3)
{
	struct k_work_q *work_q = work_q_ptr;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct k_work *work;
		k_work_handler_t handler;

		work = k_fifo_get(&work_q->fifo, K_FOREVER);

		handler = work->handler;

		/* Reset pending state so it can be resubmitted by handler */
		if (atomic_test_and_clear_bit(work->flags,
					       K_WORK_STATE_PENDING)) {
			handler(work);
		}

		/* Make sure we don't hog up the CPU if the FIFO never (or
		 * very rarely) gets empty.
		 */
		k_yield();
	}
}

// KID 20170715
// &k_sys_work_q, sys_work_q_stack, K_THREAD_STACK_SIZEOF(sys_work_q_stack): 1024, CONFIG_SYSTEM_WORKQUEUE_PRIORITY: -1
void k_work_q_start(struct k_work_q *work_q, char *stack,
		    size_t stack_size, int prio)
{
	// &work_q->fifo: &(&k_sys_work_q)->fifo
	k_fifo_init(&work_q->fifo);

	// k_fifo_init 에서 한일:
	// (&(&(&k_sys_work_q)->fifo)->data_q)->head: NULL
	// (&(&(&k_sys_work_q)->fifo)->data_q)->tail: NULL
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->head: &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->fifo)->wait_q

	// &work_q->thread: &(&k_sys_work_q)->thread, stack: sys_work_q_stack, stack_size: 1024, work_q: &k_sys_work_q, prio: -1
	k_thread_create(&work_q->thread, stack, stack_size, work_q_main,
			work_q, 0, 0, prio, 0, 0);
}

#ifdef CONFIG_SYS_CLOCK_EXISTS
static void work_timeout(struct _timeout *t)
{
	struct k_delayed_work *w = CONTAINER_OF(t, struct k_delayed_work,
						   timeout);

	/* submit work to workqueue */
	k_work_submit_to_queue(w->work_q, &w->work);
	/* detach from workqueue, for cancel to return appropriate status */
	w->work_q = NULL;
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
	k_work_init(&work->work, handler);
	_init_timeout(&work->timeout, work_timeout);
	work->work_q = NULL;
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
				   struct k_delayed_work *work,
				   s32_t delay)
{
	int key = irq_lock();
	int err;

	/* Work cannot be active in multiple queues */
	if (work->work_q && work->work_q != work_q) {
		err = -EADDRINUSE;
		goto done;
	}

	/* Cancel if work has been submitted */
	if (work->work_q == work_q) {
		err = k_delayed_work_cancel(work);
		if (err < 0) {
			goto done;
		}
	}

	/* Attach workqueue so the timeout callback can submit it */
	work->work_q = work_q;

	if (!delay) {
		/* Submit work if no ticks is 0 */
		k_work_submit_to_queue(work_q, &work->work);
	} else {
		/* Add timeout */
		_add_timeout(NULL, &work->timeout, NULL,
				_TICK_ALIGN + _ms_to_ticks(delay));
	}

	err = 0;

done:
	irq_unlock(key);

	return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
	int key = irq_lock();

	if (k_work_pending(&work->work)) {
		irq_unlock(key);
		return -EINPROGRESS;
	}

	if (!work->work_q) {
		irq_unlock(key);
		return -EINVAL;
	}

	/* Abort timeout, if it has expired this will do nothing */
	_abort_timeout(&work->timeout);

	/* Detach from workqueue */
	work->work_q = NULL;

	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */
