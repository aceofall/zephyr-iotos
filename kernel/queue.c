/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief dynamic-size QUEUE object.
 */


#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <ksched.h>
#include <misc/slist.h>
#include <init.h>

extern struct k_queue _k_queue_list_start[];
extern struct k_queue _k_queue_list_end[];

#ifdef CONFIG_OBJECT_TRACING // CONFIG_OBJECT_TRACING=n

// KID 20170717
struct k_queue *_trace_list_k_queue;

/*
 * Complete initialization of statically defined queues.
 */
static int init_queue_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_queue *queue;

	for (queue = _k_queue_list_start; queue < _k_queue_list_end; queue++) {
		SYS_TRACING_OBJ_INIT(k_queue, queue);
	}
	return 0;
}

SYS_INIT(init_queue_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

// KID 20170717
// &work_q->fifo: &(&k_sys_work_q)->fifo
void k_queue_init(struct k_queue *queue)
{
	// &queue->data_q: &(&(&k_sys_work_q)->fifo)->data_q
	sys_slist_init(&queue->data_q);

	// sys_slist_init 에서 한일:
	// (&(&(&k_sys_work_q)->fifo)->data_q)->head: NULL
	// (&(&(&k_sys_work_q)->fifo)->data_q)->tail: NULL

	// &queue->wait_q: &(&(&k_sys_work_q)->fifo)->wait_q
	sys_dlist_init(&queue->wait_q);

	// sys_dlist_init에서 한일:
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->head: &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->fifo)->wait_q

	// queue: &(&k_sys_work_q)->fifo
	_INIT_OBJ_POLL_EVENT(queue); // null function

	// queue: &(&k_sys_work_q)->fifo
	SYS_TRACING_OBJ_INIT(k_queue, queue); // null function
}

static void prepare_thread_to_run(struct k_thread *thread, void *data)
{
	_abort_thread_timeout(thread);
	_ready_thread(thread);
	_set_thread_return_value_with_data(thread, 0, data);
}

/* returns 1 if a reschedule must take place, 0 otherwise */
static inline int handle_poll_event(struct k_queue *queue)
{
#ifdef CONFIG_POLL
	u32_t state = K_POLL_STATE_DATA_AVAILABLE;

	return queue->poll_event ?
	       _handle_obj_poll_event(&queue->poll_event, state) : 0;
#else
	return 0;
#endif
}

void k_queue_cancel_wait(struct k_queue *queue)
{
	struct k_thread *first_pending_thread;
	unsigned int key;

	key = irq_lock();

	first_pending_thread = _unpend_first_thread(&queue->wait_q);

	if (first_pending_thread) {
		prepare_thread_to_run(first_pending_thread, NULL);
		if (!_is_in_isr() && _must_switch_threads()) {
			(void)_Swap(key);
			return;
		}
	} else {
		if (handle_poll_event(queue)) {
			(void)_Swap(key);
			return;
		}
	}

	irq_unlock(key);
}

void k_queue_insert(struct k_queue *queue, void *prev, void *data)
{
	struct k_thread *first_pending_thread;
	unsigned int key;

	key = irq_lock();

	first_pending_thread = _unpend_first_thread(&queue->wait_q);

	if (first_pending_thread) {
		prepare_thread_to_run(first_pending_thread, data);
		if (!_is_in_isr() && _must_switch_threads()) {
			(void)_Swap(key);
			return;
		}
	} else {
		sys_slist_insert(&queue->data_q, prev, data);
		if (handle_poll_event(queue)) {
			(void)_Swap(key);
			return;
		}
	}

	irq_unlock(key);
}

void k_queue_append(struct k_queue *queue, void *data)
{
	return k_queue_insert(queue, queue->data_q.tail, data);
}

void k_queue_prepend(struct k_queue *queue, void *data)
{
	return k_queue_insert(queue, NULL, data);
}

void k_queue_append_list(struct k_queue *queue, void *head, void *tail)
{
	__ASSERT(head && tail, "invalid head or tail");

	struct k_thread *first_thread, *thread;
	unsigned int key;

	key = irq_lock();

	first_thread = _peek_first_pending_thread(&queue->wait_q);
	while (head && ((thread = _unpend_first_thread(&queue->wait_q)))) {
		prepare_thread_to_run(thread, head);
		head = *(void **)head;
	}

	if (head) {
		sys_slist_append_list(&queue->data_q, head, tail);
	}

	if (first_thread) {
		if (!_is_in_isr() && _must_switch_threads()) {
			(void)_Swap(key);
			return;
		}
	} else {
		if (handle_poll_event(queue)) {
			(void)_Swap(key);
			return;
		}
	}

	irq_unlock(key);
}

void k_queue_merge_slist(struct k_queue *queue, sys_slist_t *list)
{
	__ASSERT(!sys_slist_is_empty(list), "list must not be empty");

	/*
	 * note: this works as long as:
	 * - the slist implementation keeps the next pointer as the first
	 *   field of the node object type
	 * - list->tail->next = NULL.
	 */
	k_queue_append_list(queue, list->head, list->tail);
	sys_slist_init(list);
}

// KID 20170719
// &work_q->fifo: &(&k_sys_work_q)->fifo, K_FOREVER: -1
void *k_queue_get(struct k_queue *queue, s32_t timeout)
{
	unsigned int key;
	void *data;

	// irq_lock(): eflags 값
	key = irq_lock();
	// key: eflags 값

	// &queue->data_q: &(&(&k_sys_work_q)->fifo)->data_q
	// sys_slist_is_empty(&(&(&k_sys_work_q)->fifo)->data_q): 1
	if (likely(!sys_slist_is_empty(&queue->data_q))) {
		data = sys_slist_get_not_empty(&queue->data_q);
		irq_unlock(key);
		return data;
	}

	// timeout: -1, K_NO_WAIT: 0
	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return NULL;
	}

	// &queue->wait_q: &(&(&k_sys_work_q)->fifo)->wait_q, timeout: -1
	_pend_current_thread(&queue->wait_q, timeout);

	// _pend_current_thread 에서 한일:
	// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next, &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail->next: (&(&(&k_sys_work_q)->fifo)->wait_q)->next: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->thread)->base.k_q_node
	//
	// (&(&k_sys_work_q)->thread)->base.thread_state: 0x2

	return _Swap(key) ? NULL : _current->base.swap_data;
}
