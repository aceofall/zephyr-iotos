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
#if defined(CONFIG_POLL)
	// queue: &(&k_sys_work_q)->fifo
	sys_dlist_init(&queue->poll_events);
#endif

	// queue: &(&k_sys_work_q)->fifo
	SYS_TRACING_OBJ_INIT(k_queue, queue); // null function
}

#if !defined(CONFIG_POLL)
static void prepare_thread_to_run(struct k_thread *thread, void *data)
{
	_abort_thread_timeout(thread);
	_ready_thread(thread);
	_set_thread_return_value_with_data(thread, 0, data);
}
#endif /* CONFIG_POLL */

/* returns 1 if a reschedule must take place, 0 otherwise */
static inline int handle_poll_events(struct k_queue *queue)
{
#ifdef CONFIG_POLL
	u32_t state = K_POLL_STATE_DATA_AVAILABLE;

	return _handle_obj_poll_events(&queue->poll_events, state);
#else
	return 0;
#endif
}

void k_queue_cancel_wait(struct k_queue *queue)
{
	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	struct k_thread *first_pending_thread;

	first_pending_thread = _unpend_first_thread(&queue->wait_q);

	if (first_pending_thread) {
		prepare_thread_to_run(first_pending_thread, NULL);
		if (!_is_in_isr() && _must_switch_threads()) {
			(void)_Swap(key);
			return;
		}
	}
#else
	if (handle_poll_events(queue)) {
		(void)_Swap(key);
		return;
	}
#endif /* !CONFIG_POLL */

	irq_unlock(key);
}

void k_queue_insert(struct k_queue *queue, void *prev, void *data)
{
	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	struct k_thread *first_pending_thread;

	first_pending_thread = _unpend_first_thread(&queue->wait_q);

	if (first_pending_thread) {
		prepare_thread_to_run(first_pending_thread, data);
		if (!_is_in_isr() && _must_switch_threads()) {
			(void)_Swap(key);
			return;
		}
		irq_unlock(key);
		return;
	}
#endif /* !CONFIG_POLL */

	sys_slist_insert(&queue->data_q, prev, data);

#if defined(CONFIG_POLL)
	if (handle_poll_events(queue)) {
		(void)_Swap(key);
		return;
	}
#endif /* CONFIG_POLL */

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

	unsigned int key = irq_lock();
#if !defined(CONFIG_POLL)
	struct k_thread *first_thread, *thread;

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
	}
#else
	sys_slist_append_list(&queue->data_q, head, tail);
	if (handle_poll_events(queue)) {
		(void)_Swap(key);
		return;
	}
#endif /* !CONFIG_POLL */

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

#if defined(CONFIG_POLL)
// KID 20170719
// &work_q->fifo: &(&k_sys_work_q)->fifo, K_FOREVER: -1
static void *k_queue_poll(struct k_queue *queue, s32_t timeout)
{
	struct k_poll_event event;
	int err;

	k_poll_event_init(&event, K_POLL_TYPE_FIFO_DATA_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, queue);

	event.state = K_POLL_STATE_NOT_READY;

	err = k_poll(&event, 1, timeout);
	if (err) {
		return NULL;
	}

	__ASSERT_NO_MSG(event.state == K_POLL_STATE_FIFO_DATA_AVAILABLE);

	return sys_slist_get(&queue->data_q);
}
#endif /* CONFIG_POLL */

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

#if defined(CONFIG_POLL)
	irq_unlock(key);

	// &queue->wait_q: &(&(&k_sys_work_q)->fifo)->wait_q, timeout: -1
	return k_queue_poll(queue, timeout);

#else
	_pend_current_thread(&queue->wait_q, timeout);

	// _pend_current_thread 에서 한일:
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

	// key: eflags 값
	return _Swap(key) ? NULL : _current->base.swap_data;
#endif /* CONFIG_POLL */
}
