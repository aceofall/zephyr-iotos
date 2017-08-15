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
// KID 20170718
// &k_sys_work_q, NULL, NULL
static void work_q_main(void *work_q_ptr, void *p2, void *p3)
{
	// work_q_ptr: &k_sys_work_q
	struct k_work_q *work_q = work_q_ptr;
	// work_q: &k_sys_work_q

	// p2: NULL
	ARG_UNUSED(p2);

	// p3: NULL
	ARG_UNUSED(p3);

	while (1) {
		struct k_work *work;
		k_work_handler_t handler;

		// &work_q->fifo: &(&k_sys_work_q)->fifo, K_FOREVER: -1
		work = k_queue_get(&work_q->queue, K_FOREVER);
		if (!work) {
			continue;
		}

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
// KID 20170726
// &k_sys_work_q, sys_work_q_stack, K_THREAD_STACK_SIZEOF(sys_work_q_stack): 1024, CONFIG_SYSTEM_WORKQUEUE_PRIORITY: -1
void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t stack,
		    size_t stack_size, int prio)
{
	// &work_q->fifo: &(&k_sys_work_q)->fifo
	k_queue_init(&work_q->queue);

	// k_fifo_init 에서 한일:
	// (&(&(&k_sys_work_q)->fifo)->data_q)->head: NULL
	// (&(&(&k_sys_work_q)->fifo)->data_q)->tail: NULL
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->head: &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->fifo)->wait_q

	// &work_q->thread: &(&k_sys_work_q)->thread, stack: sys_work_q_stack, stack_size: 1024, work_q: &k_sys_work_q, prio: -1
	// k_thread_create(&(&k_sys_work_q)->thread, sys_work_q_stack, 1024, &k_sys_work_q, 0, 0, -1, 0, 0): &(&k_sys_work_q)->thread
	k_thread_create(&work_q->thread, stack, stack_size, work_q_main,
			work_q, 0, 0, prio, 0, 0);

	// k_thread_create 에서 한일:
	// 신규 쓰레드를 생성함
	// (&(&(&k_sys_work_q)->thread)->base)->user_options: 0
	// (&(&(&k_sys_work_q)->thread)->base)->thread_state: 0x4
	// (&(&(&k_sys_work_q)->thread)->base)->prio: -1
	// (&(&(&k_sys_work_q)->thread)->base)->sched_locked: 0
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->wait_q: NULL
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->thread: NULL
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->func: NULL
	//
	// (&(&k_sys_work_q)->thread)->init_data: NULL
	// (&(&k_sys_work_q)->thread)->fn_abort: NULL
	//
	// *(unsigned long *)(sys_work_q_stack + 1024): &k_sys_work_q
	// *(unsigned long *)(sys_work_q_stack + 1020): NULL
	// *(unsigned long *)(sys_work_q_stack + 1016): NULL
	// *(unsigned long *)(sys_work_q_stack + 1016), pEntry: work_q_main
	// *(unsigned long *)(sys_work_q_stack + 1012): eflag 레지스터 값 | 0x00000200
	// *(unsigned long *)(sys_work_q_stack + 1008): _thread_entry_wrapper
	//
	// (&(&k_sys_work_q)->thread)->callee_saved.esp: sys_work_q_stack + 980
	//
	// 생성된 쓰레드 &(&k_sys_work_q)->thread 을 시작 시키기 위해 레디큐에 등록함
	//
	// (&(&k_sys_work_q)->thread)->base.thread_state: 0
	//
	// _kernel.ready_q.prio_bmap[0]: 0x80018000
	//
	// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next: &_kernel.ready_q.q[15]
	// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: (&_kernel.ready_q.q[15])->tail
	// (&_kernel.ready_q.q[15])->tail->next: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// (&_kernel.ready_q.q[15])->tail: &(&(&k_sys_work_q)->thread)->base.k_q_node
	//
	// _kernel.ready_q.cache: &(&k_sys_work_q)->thread
	//
	// 현재 실행 중인 &_main_thread_s 보다 우선 순위가 높은 &(&k_sys_work_q)->thread 가 실행하여
	// &_main_thread_s 는 선점됨, work_q_main이 실행 되다가 &(&k_sys_work_q)->fifo에 있는 큐에
	// 연결되어 있는 잡이 비어 있는 상태를 확인 후 &(&(&k_sys_work_q)->fifo)->wait_q 인 웨이트 큐에
	// &(&(&k_sys_work_q)->thread)->base.k_q_node 를 등록하고 현재 쓰레드 &(&k_sys_work_q)->thread 는
	// _THREAD_PENDING 상태롤 변경, 변경된 이후 다시 &_main_thread_s 로 쓰레드 복귀 하여 수행 됨
	//
	// &_kernel.ready_q.q[15] 에 연결된 list node 인 &(&(&k_sys_work_q)->thread)->base.k_q_node 을 제거함
	//
	// (&_kernel.ready_q.q[15])->next: &_kernel.ready_q.q[15]
	// (&_kernel.ready_q.q[15])->prev: &_kernel.ready_q.q[15]
	//
	// _kernel.ready_q.prio_bmap[0]: 0x80010000
	// _kernel.ready_q.cache: &_main_thread_s
	//
	// &(&(&k_sys_work_q)->fifo)->wait_q 에 list node 인 &(&(&k_sys_work_q)->thread)->base.k_q_node 을 tail로 추가함
	//
	// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next, &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail->next: (&(&(&k_sys_work_q)->fifo)->wait_q)->next: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->thread)->base.k_q_node
	//
	// (&(&k_sys_work_q)->thread)->base.thread_state: 0x2
}

#ifdef CONFIG_SYS_CLOCK_EXISTS
static void work_timeout(struct _timeout *t)
{
	struct k_delayed_work *w = CONTAINER_OF(t, struct k_delayed_work,
						   timeout);

	/* submit work to workqueue */
	k_work_submit_to_queue(w->work_q, &w->work);
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

	if (!work->work_q) {
		irq_unlock(key);
		return -EINVAL;
	}

	if (k_work_pending(&work->work)) {
		/* Remove from the queue if already submitted */
		if (!k_queue_remove(&work->work_q->queue, &work->work)) {
			irq_unlock(key);
			return -EINVAL;
		}
	} else {
		_abort_timeout(&work->timeout);
	}

	/* Detach from workqueue */
	work->work_q = NULL;

	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */
