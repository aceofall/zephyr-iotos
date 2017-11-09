/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel thread support
 *
 * This module provides general purpose thread support.
 */

#include <kernel.h>

#include <toolchain.h>
#include <linker/sections.h>

#include <kernel_structs.h>
#include <misc/printk.h>
#include <sys_clock.h>
#include <drivers/system_timer.h>
#include <ksched.h>
#include <wait_q.h>
#include <atomic.h>
#include <syscall_handler.h>

// KID 20170728
// KID 20170807
extern struct _static_thread_data _static_thread_data_list_start[];
extern struct _static_thread_data _static_thread_data_list_end[];

#define _FOREACH_STATIC_THREAD(thread_data)              \
	for (struct _static_thread_data *thread_data =   \
	     _static_thread_data_list_start;             \
	     thread_data < _static_thread_data_list_end; \
	     thread_data++)


int k_is_in_isr(void)
{
	return _is_in_isr();
}

/*
 * This function tags the current thread as essential to system operation.
 * Exceptions raised by this thread will be treated as a fatal system error.
 */
void _thread_essential_set(void)
{
	_current->base.user_options |= K_ESSENTIAL;
}

/*
 * This function tags the current thread as not essential to system operation.
 * Exceptions raised by this thread may be recoverable.
 * (This is the default tag for a thread.)
 */
void _thread_essential_clear(void)
{
	_current->base.user_options &= ~K_ESSENTIAL;
}

/*
 * This routine indicates if the current thread is an essential system thread.
 *
 * Returns non-zero if current thread is essential, zero if it is not.
 */
int _is_thread_essential(void)
{
	return _current->base.user_options & K_ESSENTIAL;
}

void k_busy_wait(u32_t usec_to_wait)
{
#if defined(CONFIG_TICKLESS_KERNEL) && \
	    !defined(CONFIG_BUSY_WAIT_USES_ALTERNATE_CLOCK)
int saved_always_on = k_enable_sys_clock_always_on();
#endif
	/* use 64-bit math to prevent overflow when multiplying */
	u32_t cycles_to_wait = (u32_t)(
		(u64_t)usec_to_wait *
		(u64_t)sys_clock_hw_cycles_per_sec /
		(u64_t)USEC_PER_SEC
	);
	u32_t start_cycles = k_cycle_get_32();

	for (;;) {
		u32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
#if defined(CONFIG_TICKLESS_KERNEL) && \
	    !defined(CONFIG_BUSY_WAIT_USES_ALTERNATE_CLOCK)
	_sys_clock_always_on = saved_always_on;
#endif
}

#ifdef CONFIG_THREAD_CUSTOM_DATA
void _impl_k_thread_custom_data_set(void *value)
{
	_current->custom_data = value;
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_thread_custom_data_set, data)
{
	_impl_k_thread_custom_data_set((void *)data);
	return 0;
}
#endif

void *_impl_k_thread_custom_data_get(void)
{
	return _current->custom_data;
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER0_SIMPLE(k_thread_custom_data_get);
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_THREAD_CUSTOM_DATA */

#if defined(CONFIG_THREAD_MONITOR)
/*
 * Remove a thread from the kernel's list of active threads.
 */
void _thread_monitor_exit(struct k_thread *thread)
{
	unsigned int key = irq_lock();

	if (thread == _kernel.threads) {
		_kernel.threads = _kernel.threads->next_thread;
	} else {
		struct k_thread *prev_thread;

		prev_thread = _kernel.threads;
		while (thread != prev_thread->next_thread) {
			prev_thread = prev_thread->next_thread;
		}
		prev_thread->next_thread = thread->next_thread;
	}

	irq_unlock(key);
}
#endif /* CONFIG_THREAD_MONITOR */

#ifdef CONFIG_STACK_SENTINEL
/* Check that the stack sentinel is still present
 *
 * The stack sentinel feature writes a magic value to the lowest 4 bytes of
 * the thread's stack when the thread is initialized. This value gets checked
 * in a few places:
 *
 * 1) In k_yield() if the current thread is not swapped out
 * 2) After servicing a non-nested interrupt
 * 3) In _Swap(), check the sentinel in the outgoing thread
 *
 * Item 2 requires support in arch/ code.
 *
 * If the check fails, the thread will be terminated appropriately through
 * the system fatal error handler.
 */
void _check_stack_sentinel(void)
{
	u32_t *stack;

	if (_current->base.thread_state == _THREAD_DUMMY) {
		return;
	}

	stack = (u32_t *)_current->stack_info.start;
	if (*stack != STACK_SENTINEL) {
		/* Restore it so further checks don't trigger this same error */
		*stack = STACK_SENTINEL;
		_k_except_reason(_NANO_ERR_STACK_CHK_FAIL);
	}
}
#endif

/*
 * Common thread entry point function (used by all threads)
 *
 * This routine invokes the actual thread entry point function and passes
 * it three arguments. It also handles graceful termination of the thread
 * if the entry point function ever returns.
 *
 * This routine does not return, and is marked as such so the compiler won't
 * generate preamble code that is only used by functions that actually return.
 */
// KID 20170522
FUNC_NORETURN void _thread_entry(k_thread_entry_t entry,
				 void *p1, void *p2, void *p3)
{
	entry(p1, p2, p3);

#ifdef CONFIG_MULTITHREADING
	k_thread_abort(k_current_get());
#else
	for (;;) {
		k_cpu_idle();
	}
#endif

	/*
	 * Compiler can't tell that k_thread_abort() won't return and issues a
	 * warning unless we tell it that control never gets this far.
	 */

	CODE_UNREACHABLE;
}

#ifdef CONFIG_MULTITHREADING // CONFIG_MULTITHREADING=y
// KID 20170717
// KID 20170726
// thread: &(&k_sys_work_q)->thread
void _impl_k_thread_start(struct k_thread *thread)
{
	// irq_lock(): eflags 값
	int key = irq_lock(); /* protect kernel queues */
	// key: eflags 값

	// thread: &(&k_sys_work_q)->thread
	if (_has_thread_started(thread)) {
		irq_unlock(key);
		return;
	}

	_mark_thread_as_started(thread);

	// _mark_thread_as_started 에서 한일:
	// (&(&k_sys_work_q)->thread)->base.thread_state: 0

	// thread: &(&k_sys_work_q)->thread, _is_thread_ready(&(&k_sys_work_q)->thread): 1
	if (_is_thread_ready(thread)) {
		// thread: &(&k_sys_work_q)->thread
		_add_thread_to_ready_q(thread);

		// _add_thread_to_ready_q 에서 한일:
		// _kernel.ready_q.prio_bmap[0]: 0x80018000
		//
		// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next: &_kernel.ready_q.q[15]
		// (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: (&_kernel.ready_q.q[15])->tail
		// (&_kernel.ready_q.q[15])->tail->next: &(&(&k_sys_work_q)->thread)->base.k_q_node
		// (&_kernel.ready_q.q[15])->tail: &(&(&k_sys_work_q)->thread)->base.k_q_node
		//
		// _kernel.ready_q.cache: &(&k_sys_work_q)->thread

		// _must_switch_threads(): 1
		if (_must_switch_threads()) {
			// key: eflags 값
			_Swap(key);

			// _Swap 에서 한일:
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

			return;
			// return 수행
		}
	}

	irq_unlock(key);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE_VOID(k_thread_start, K_OBJ_THREAD, struct k_thread *);
#endif
#endif

#ifdef CONFIG_MULTITHREADING // CONFIG_MULTITHREADING=y
// KID 20170717
// KID 20170726
// new_thread: &(&k_sys_work_q)->thread, delay: 0
static void schedule_new_thread(struct k_thread *thread, s32_t delay)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS // CONFIG_SYS_CLOCK_EXISTS=y
	// delay: 0
	if (delay == 0) {
		// thread: &(&k_sys_work_q)->thread
		k_thread_start(thread);

		// start_thread 에서 한일:
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
	} else {
		s32_t ticks = _TICK_ALIGN + _ms_to_ticks(delay);
		int key = irq_lock();

		_add_thread_timeout(thread, NULL, ticks);
		irq_unlock(key);
	}
#else
	ARG_UNUSED(delay);
	k_thread_start(thread);
#endif
}
#endif

// KID 20170717
// KID 20170726
// &work_q->thread: &(&k_sys_work_q)->thread, stack: sys_work_q_stack, stack_size: 1024, work_q_main,
// work_q: &k_sys_work_q, 0, 0, prio: -1, 0, 0
void _setup_new_thread(struct k_thread *new_thread,
		       k_thread_stack_t *stack, size_t stack_size,
		       k_thread_entry_t entry,
		       void *p1, void *p2, void *p3,
		       int prio, u32_t options)
{
	_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3,
		    prio, options);
#ifdef CONFIG_USERSPACE
	_k_object_init(new_thread);
	_k_object_init(stack);
	new_thread->stack_obj = stack;

	/* Any given thread has access to itself */
	k_object_access_grant(new_thread, new_thread);

	/* New threads inherit any memory domain membership by the parent */
	if (_current->mem_domain_info.mem_domain) {
		k_mem_domain_add_thread(_current->mem_domain_info.mem_domain,
					new_thread);
	}

	if (options & K_INHERIT_PERMS) {
		_thread_perms_inherit(_current, new_thread);
	}
#endif
}

#ifdef CONFIG_MULTITHREADING // CONFIG_MULTITHREADING=y
k_tid_t _impl_k_thread_create(struct k_thread *new_thread,
			      k_thread_stack_t *stack,
			      size_t stack_size, k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, u32_t options, s32_t delay)
{
	// _is_in_isr(): 0
	__ASSERT(!_is_in_isr(), "Threads may not be created in ISRs");

	// new_thread: &(&k_sys_work_q)->thread, stack: sys_work_q_stack, stack_size: 1024, entry: work_q_main,
	// p1: &k_sys_work_q, p2: 0, p3: 0, prio: -1, options: 0
	_setup_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3,
			  prio, options);
	// new_thread: &(&k_sys_work_q)->thread

	// _new_thread 에서 한일:
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

	if (delay != K_FOREVER) {
    // new_thread: &(&k_sys_work_q)->thread, delay: 0
		schedule_new_thread(new_thread, delay);

    // schedule_new_thread 에서 한일:
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
	return new_thread;
	// return &(&k_sys_work_q)->thread
}


#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_thread_create,
		 new_thread_p, stack_p, stack_size, entry, p1, more_args)
{
	int prio;
	u32_t options, delay, guard_size, total_size;
	struct _k_object *stack_object;
	struct k_thread *new_thread = (struct k_thread *)new_thread_p;
	volatile struct _syscall_10_args *margs =
		(volatile struct _syscall_10_args *)more_args;
	k_thread_stack_t *stack = (k_thread_stack_t *)stack_p;

	/* The thread and stack objects *must* be in an uninitialized state */
	_SYSCALL_OBJ_NEVER_INIT(new_thread, K_OBJ_THREAD);
	stack_object = _k_object_find(stack);
	_SYSCALL_VERIFY_MSG(!_obj_validation_check(stack_object, stack,
						   K_OBJ__THREAD_STACK_ELEMENT,
						   _OBJ_INIT_FALSE),
			    "bad stack object");

	/* Verify that the stack size passed in is OK by computing the total
	 * size and comparing it with the size value in the object metadata
	 */
	guard_size = (u32_t)K_THREAD_STACK_BUFFER(stack) - (u32_t)stack;
	_SYSCALL_VERIFY_MSG(!__builtin_uadd_overflow(guard_size, stack_size,
						     &total_size),
			    "stack size overflow (%u+%u)", stack_size,
			    guard_size);
	/* They really ought to be equal, make this more strict? */
	_SYSCALL_VERIFY_MSG(total_size <= stack_object->data,
			    "stack size %u is too big, max is %u",
			    total_size, stack_object->data);

	/* Verify the struct containing args 6-10 */
	_SYSCALL_MEMORY_READ(margs, sizeof(*margs));

	/* Stash struct arguments in local variables to prevent switcheroo
	 * attacks
	 */
	prio = margs->arg8;
	options = margs->arg9;
	delay = margs->arg10;
	compiler_barrier();

	/* User threads may only create other user threads and they can't
	 * be marked as essential
	 */
	_SYSCALL_VERIFY(options & K_USER);
	_SYSCALL_VERIFY(!(options & K_ESSENTIAL));

	/* Check validity of prio argument; must be the same or worse priority
	 * than the caller
	 */
	_SYSCALL_VERIFY(_VALID_PRIO(prio, NULL));
	_SYSCALL_VERIFY(_is_prio_lower_or_equal(prio, _current->base.prio));

	_setup_new_thread((struct k_thread *)new_thread, stack, stack_size,
			  (k_thread_entry_t)entry, (void *)p1,
			  (void *)margs->arg6, (void *)margs->arg7, prio,
			  options);

	if (delay != K_FOREVER) {
		schedule_new_thread(new_thread, delay);
	}

	return new_thread_p;
}
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_MULTITHREADING */

int _impl_k_thread_cancel(k_tid_t tid)
{
	struct k_thread *thread = tid;

	int key = irq_lock();

	if (_has_thread_started(thread) ||
	    !_is_thread_timeout_active(thread)) {
		irq_unlock(key);
		return -EINVAL;
	}

	_abort_thread_timeout(thread);
	_thread_monitor_exit(thread);

	irq_unlock(key);

	return 0;
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE(k_thread_cancel, K_OBJ_THREAD, struct k_thread *);
#endif

static inline int is_in_any_group(struct _static_thread_data *thread_data,
				  u32_t groups)
{
	return !!(thread_data->init_groups & groups);
}

void _k_thread_group_op(u32_t groups, void (*func)(struct k_thread *))
{
	unsigned int  key;

	__ASSERT(!_is_in_isr(), "");

	_sched_lock();

	/* Invoke func() on each static thread in the specified group set. */

	_FOREACH_STATIC_THREAD(thread_data) {
		if (is_in_any_group(thread_data, groups)) {
			key = irq_lock();
			func(thread_data->init_thread);
			irq_unlock(key);
		}
	}

	/*
	 * If the current thread is still in a ready state, then let the
	 * "unlock scheduler" code determine if any rescheduling is needed.
	 */
	if (_is_thread_ready(_current)) {
		k_sched_unlock();
		return;
	}

	/* The current thread is no longer in a ready state--reschedule. */
	key = irq_lock();
	_sched_unlock_no_reschedule();
	_Swap(key);
}

void _k_thread_single_start(struct k_thread *thread)
{
	_mark_thread_as_started(thread);

	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
	}
}

void _k_thread_single_suspend(struct k_thread *thread)
{
	if (_is_thread_ready(thread)) {
		_remove_thread_from_ready_q(thread);
	}

	_mark_thread_as_suspended(thread);
}

void _impl_k_thread_suspend(struct k_thread *thread)
{
	unsigned int  key = irq_lock();

	_k_thread_single_suspend(thread);

	if (thread == _current) {
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE_VOID(k_thread_suspend, K_OBJ_THREAD, k_tid_t);
#endif

void _k_thread_single_resume(struct k_thread *thread)
{
	_mark_thread_as_not_suspended(thread);

	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
	}
}

void _impl_k_thread_resume(struct k_thread *thread)
{
	unsigned int  key = irq_lock();

	_k_thread_single_resume(thread);

	_reschedule_threads(key);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE_VOID(k_thread_resume, K_OBJ_THREAD, k_tid_t);
#endif

void _k_thread_single_abort(struct k_thread *thread)
{
	if (thread->fn_abort != NULL) {
		thread->fn_abort();
	}

	if (_is_thread_ready(thread)) {
		_remove_thread_from_ready_q(thread);
	} else {
		if (_is_thread_pending(thread)) {
			_unpend_thread(thread);
		}
		if (_is_thread_timeout_active(thread)) {
			_abort_thread_timeout(thread);
		}
	}

	thread->base.thread_state |= _THREAD_DEAD;
#ifdef CONFIG_KERNEL_EVENT_LOGGER_THREAD
	_sys_k_event_logger_thread_exit(thread);
#endif

#ifdef CONFIG_USERSPACE
	/* Clear initailized state so that this thread object may be re-used
	 * and triggers errors if API calls are made on it from user threads
	 */
	_k_object_uninit(thread->stack_obj);
	_k_object_uninit(thread);

	/* Revoke permissions on thread's ID so that it may be recycled */
	_thread_perms_all_clear(thread);
#endif
}

#ifdef CONFIG_MULTITHREADING // CONFIG_MULTITHREADING=y
#ifdef CONFIG_USERSPACE
extern char __object_access_start[];
extern char __object_access_end[];

static void grant_static_access(void)
{
	struct _k_object_assignment *pos;

	for (pos = (struct _k_object_assignment *)__object_access_start;
	     pos < (struct _k_object_assignment *)__object_access_end;
	     pos++) {
		for (int i = 0; pos->objects[i] != NULL; i++) {
			k_object_access_grant(pos->objects[i],
					      pos->thread);
		}
	}
}
#endif /* CONFIG_USERSPACE */

// KID 20170728
// KID 20170807
void _init_static_threads(void)
{
	unsigned int  key;

	_FOREACH_STATIC_THREAD(thread_data) {
	// for (struct _static_thread_data *thread_data = _static_thread_data_list_start;
	//      thread_data < _static_thread_data_list_end; thread_data++)
		_setup_new_thread(
			thread_data->init_thread,
			thread_data->init_stack,
			thread_data->init_stack_size,
			thread_data->init_entry,
			thread_data->init_p1,
			thread_data->init_p2,
			thread_data->init_p3,
			thread_data->init_prio,
			thread_data->init_options);

		thread_data->init_thread->init_data = thread_data;
	}

#ifdef CONFIG_USERSPACE
	grant_static_access();
#endif
	_sched_lock();

	/*
	 * Non-legacy static threads may be started immediately or after a
	 * previously specified delay. Even though the scheduler is locked,
	 * ticks can still be delivered and processed. Lock interrupts so
	 * that the countdown until execution begins from the same tick.
	 *
	 * Note that static threads defined using the legacy API have a
	 * delay of K_FOREVER.
	 */
	key = irq_lock();
	_FOREACH_STATIC_THREAD(thread_data) {
		if (thread_data->init_delay != K_FOREVER) {
			schedule_new_thread(thread_data->init_thread,
					    thread_data->init_delay);
		}
	}
	irq_unlock(key);
	k_sched_unlock();
}
#endif

// KID 20170522
// &thread->base: &(&_main_thread_s)->base, prio: 0, _THREAD_PRESTART: 0x4, options: 0x1
// KID 20170525
// &thread->base: &(&_idle_thread_s)->base, prio: 15, _THREAD_PRESTART: 0x4, options: 0x1
// KID 20170605
// &async_msg[0].thread, 0, _THREAD_DUMMY: 0x1, 0
// KID 20170717
// &thread->base: &(&(&k_sys_work_q)->thread)->base, prio: -1, _THREAD_PRESTART: 0x4, options: 0
void _init_thread_base(struct _thread_base *thread_base, int priority,
		       u32_t initial_state, unsigned int options)
{
	/* k_q_node is initialized upon first insertion in a list */

	// thread_base->user_options: (&(&_main_thread_s)->base)->user_options: 0, options: 0x1
	thread_base->user_options = (u8_t)options;
	// thread_base->user_options: (&(&_main_thread_s)->base)->user_options: 0x1

	// thread_base->thread_state: (&(&_main_thread_s)->base)->thread_state: 0, initial_state: 0x4
	thread_base->thread_state = (u8_t)initial_state;
	// thread_base->thread_state: (&(&_main_thread_s)->base)->thread_state: 0x4

	// thread_base->prio: (&(&_main_thread_s)->base)->prio: 0, priority: 0
	thread_base->prio = priority;
	// thread_base->prio: (&(&_main_thread_s)->base)->prio: 0

	// thread_base->sched_locked: (&(&_main_thread_s)->base)->sched_locked: 0
	thread_base->sched_locked = 0;
	// thread_base->sched_locked: (&(&_main_thread_s)->base)->sched_locked: 0

	/* swap_data does not need to be initialized */

	// thread_base: &(&_main_thread_s)->base
	_init_thread_timeout(thread_base);

	// _init_thread_timeout 에서 한일:
	// (&(&(&_main_thread_s)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&_main_thread_s)->base)->timeout)->wait_q: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->thread: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->func: NULL
}

u32_t _k_thread_group_mask_get(struct k_thread *thread)
{
	struct _static_thread_data *thread_data = thread->init_data;

	return thread_data->init_groups;
}

void _k_thread_group_join(u32_t groups, struct k_thread *thread)
{
	struct _static_thread_data *thread_data = thread->init_data;

	thread_data->init_groups |= groups;
}

void _k_thread_group_leave(u32_t groups, struct k_thread *thread)
{
	struct _static_thread_data *thread_data = thread->init_data;

	thread_data->init_groups &= ~groups;
}

void k_thread_access_grant(struct k_thread *thread, ...)
{
#ifdef CONFIG_USERSPACE
	va_list args;
	va_start(args, thread);

	while (1) {
		void *object = va_arg(args, void *);
		if (object == NULL) {
			break;
		}
		k_object_access_grant(object, thread);
	}
	va_end(args);
#else
	ARG_UNUSED(thread);
#endif
}

FUNC_NORETURN void k_thread_user_mode_enter(k_thread_entry_t entry,
					    void *p1, void *p2, void *p3)
{
	_current->base.user_options |= K_USER;
	_thread_essential_clear();
#ifdef CONFIG_USERSPACE
	_arch_user_mode_enter(entry, p1, p2, p3);
#else
	/* XXX In this case we do not reset the stack */
	_thread_entry(entry, p1, p2, p3);
#endif
}
