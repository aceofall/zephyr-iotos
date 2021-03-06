/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _kernel_structs__h_
#define _kernel_structs__h_

#include <kernel.h>

#if !defined(_ASMLANGUAGE)
#include <atomic.h>
#include <misc/dlist.h>
#include <string.h>
#endif

/*
 * Bitmask definitions for the struct k_thread.thread_state field.
 *
 * Must be before kerneL_arch_data.h because it might need them to be already
 * defined.
 */


/* states: common uses low bits, arch-specific use high bits */

/* Not a real thread */
// KID 20170519
// KID 20170601
// KID 20170605
// KID 20170717
// _THREAD_DUMMY: 0x1
#define _THREAD_DUMMY (1 << 0)

/* Thread is waiting on an object */
// KID 20170717
// KID 20170721
// _THREAD_PENDING: 0x2
#define _THREAD_PENDING (1 << 1)

/* Thread has not yet started */
// KID 20170522
// KID 20170523
// KID 20170717
// _THREAD_PRESTART: 0x4
#define _THREAD_PRESTART (1 << 2)

/* Thread has terminated */
// KID 20170717
// _THREAD_DEAD: 0x8
#define _THREAD_DEAD (1 << 3)

/* Thread is suspended */
// KID 20170717
// _THREAD_SUSPENDED: 0x10
#define _THREAD_SUSPENDED (1 << 4)

/* Thread is actively looking at events to see if they are ready */
#define _THREAD_POLLING (1 << 5)

/* end - states */

#ifdef CONFIG_STACK_SENTINEL
/* Magic value in lowest bytes of the stack */
#define STACK_SENTINEL 0xF0F0F0F0
#endif

/* lowest value of _thread_base.preempt at which a thread is non-preemptible */
// KID 20170718
// _NON_PREEMPT_THRESHOLD: 0x0080
#define _NON_PREEMPT_THRESHOLD 0x0080

/* highest value of _thread_base.preempt at which a thread is preemptible */
// KID 20170718
// _NON_PREEMPT_THRESHOLD: 0x0080
// _PREEMPT_THRESHOLD: 0x7F
#define _PREEMPT_THRESHOLD (_NON_PREEMPT_THRESHOLD - 1)

#include <kernel_arch_data.h>

#if !defined(_ASMLANGUAGE)

// KID 20170518
// KID 20170519
// KID 20170524
// KID 20170526
// sizeof(struct _ready_q): 264 bytes
struct _ready_q {

	/* always contains next thread to run: cannot be NULL */
	struct k_thread *cache;

	/* bitmap of priorities that contain at least one ready thread */
	// K_NUM_PRIO_BITMAPS: 1
	u32_t prio_bmap[K_NUM_PRIO_BITMAPS];

	/* ready queues, one per priority */
	// K_NUM_PRIORITIES: 32
	sys_dlist_t q[K_NUM_PRIORITIES];
};

typedef struct _ready_q _ready_q_t;

// KID 20170518
// KID 20170519
// KID 20170524
// KID 20170526
// KID 20170527
// KID 20170626
// KID 20170711
// sizeof(struct _kernel): 284 bytes
struct _kernel {

	/* nested interrupt count */
	u32_t nested;

	/* interrupt stack pointer base */
	char *irq_stack;

	/* currently scheduled thread */
	struct k_thread *current;

#ifdef CONFIG_SYS_CLOCK_EXISTS // CONFIG_SYS_CLOCK_EXISTS=y
	/* queue of timeouts */
	sys_dlist_t timeout_q;
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT // CONFIG_SYS_POWER_MANAGEMENT=n
	s32_t idle; /* Number of ticks for kernel idling */
#endif

	/*
	 * ready queue: can be big, keep after small fields, since some
	 * assembly (e.g. ARC) are limited in the encoding of the offset
	 */
	struct _ready_q ready_q;

#ifdef CONFIG_FP_SHARING // CONFIG_FP_SHARING=n
	/*
	 * A 'current_sse' field does not exist in addition to the 'current_fp'
	 * field since it's not possible to divide the IA-32 non-integer
	 * registers into 2 distinct blocks owned by differing threads.  In
	 * other words, given that the 'fxnsave/fxrstor' instructions
	 * save/restore both the X87 FPU and XMM registers, it's not possible
	 * for a thread to only "own" the XMM registers.
	 */

	/* thread that owns the FP regs */
	struct k_thread *current_fp;
#endif

#if defined(CONFIG_THREAD_MONITOR) // CONFIG_THREAD_MONITOR=n
	struct k_thread *threads; /* singly linked list of ALL threads */
#endif

	/* arch-specific part of _kernel */
	struct _kernel_arch arch;
};

typedef struct _kernel _kernel_t;

// KID 20170518
extern struct _kernel _kernel;

// KID 20170518
// KID 20170718
// _current: _kernel.current
#define _current _kernel.current
// KID 20170519
// KID 20170523
// KID 20170719
// KID 20170720
// _ready_q: _kernel.ready_q
#define _ready_q _kernel.ready_q
// KID 20170527
// _timeout_q: _kernel.timeout_q
#define _timeout_q _kernel.timeout_q
#define _threads _kernel.threads

#include <kernel_arch_func.h>

static ALWAYS_INLINE void
_set_thread_return_value_with_data(struct k_thread *thread,
				   unsigned int value,
				   void *data)
{
	_set_thread_return_value(thread, value);
	thread->base.swap_data = data;
}

extern void _init_thread_base(struct _thread_base *thread_base,
			      int priority, u32_t initial_state,
			      unsigned int options);

// KID 20170519
// thread: &_main_thread_s, pStackMem: _main_stack, stackSize: 1024, priority: 0, options: 0x1
// KID 20170525
// thread: &_idle_thread_s, pStackMem: _idle_stack, stackSize: 256, priority: 15, options: 0x1
// KID 20170717
// thread: &(&k_sys_work_q)->thread, pStackMem: sys_work_q_stack, stackSize: 1024, priority: -1, options: 0
static ALWAYS_INLINE void _new_thread_init(struct k_thread *thread,
					    char *pStack, size_t stackSize,
					    int prio, unsigned int options)
{
#if !defined(CONFIG_INIT_STACKS) && !defined(CONFIG_THREAD_STACK_INFO) // CONFIG_INIT_STACKS=n, CONFIG_THREAD_STACK_INFO=n
	// ARG_UNUSED(_main_stack): (void)(_main_stack)
	// ARG_UNUSED(_idle_thread_s): (void)(_idle_thread_s)
	// ARG_UNUSED(sys_work_q_stack): (void)(sys_work_q_stack)
	ARG_UNUSED(pStack);

	// ARG_UNUSED(1024): (void)(1024)
	// ARG_UNUSED(256): (void)(256)
	// ARG_UNUSED(1024): (void)(1024)
	ARG_UNUSED(stackSize);
#endif

#ifdef CONFIG_INIT_STACKS // CONFIG_INIT_STACKS=n
	memset(pStack, 0xaa, stackSize);
#endif
#ifdef CONFIG_STACK_SENTINEL // CONFIG_STACK_SENTINEL=n
	/* Put the stack sentinel at the lowest 4 bytes of the stack area.
	 * We periodically check that it's still present and kill the thread
	 * if it isn't.
	 */
	*((u32_t *)pStack) = STACK_SENTINEL;
#endif /* CONFIG_STACK_SENTINEL */
	/* Initialize various struct k_thread members */
	// &thread->base: &(&_main_thread_s)->base, prio: 0, _THREAD_PRESTART: 0x4, options: 0x1
	// &thread->base: &(&_idle_thread_s)->base, prio: 15, _THREAD_PRESTART: 0x4, options: 0x1
	// &thread->base: &(&(&k_sys_work_q)->thread)->base, prio: -1, _THREAD_PRESTART: 0x4, options: 0
	_init_thread_base(&thread->base, prio, _THREAD_PRESTART, options);

	// _init_thread_base 에서 한일:
	// (&(&_main_thread_s)->base)->user_options: 0x1
	// (&(&_main_thread_s)->base)->thread_state: 0x4
	// (&(&_main_thread_s)->base)->prio: 0
	// (&(&_main_thread_s)->base)->sched_locked: 0
	// (&(&(&_main_thread_s)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&_main_thread_s)->base)->timeout)->wait_q: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->thread: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->func: NULL

	// _init_thread_base 에서 한일:
	// (&(&_idle_thread_s)->base)->user_options: 0x1
	// (&(&_idle_thread_s)->base)->thread_state: 0x4
	// (&(&_idle_thread_s)->base)->prio: 15
	// (&(&_idle_thread_s)->base)->sched_locked: 0
	// (&(&(&_idle_thread_s)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&_idle_thread_s)->base)->timeout)->wait_q: NULL
	// (&(&(&_idle_thread_s)->base)->timeout)->thread: NULL
	// (&(&(&_idle_thread_s)->base)->timeout)->func: NULL

	// _init_thread_base 에서 한일:
	// (&(&(&k_sys_work_q)->thread)->base)->user_options: 0
	// (&(&(&k_sys_work_q)->thread)->base)->thread_state: 0x4
	// (&(&(&k_sys_work_q)->thread)->base)->prio: -1
	// (&(&(&k_sys_work_q)->thread)->base)->sched_locked: 0
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->wait_q: NULL
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->thread: NULL
	// (&(&(&(&k_sys_work_q)->thread)->base)->timeout)->func: NULL

	/* static threads overwrite it afterwards with real value */
	// thread->init_data: (&_main_thread_s)->init_data
	// thread->init_data: (&_idle_thread_s)->init_data
	// thread->init_data: (&(&k_sys_work_q)->thread)->init_data
	thread->init_data = NULL;
	// thread->init_data: (&_main_thread_s)->init_data: NULL
	// thread->init_data: (&_idle_thread_s)->init_data: NULL
	// thread->init_data: (&(&k_sys_work_q)->thread)->init_data: NULL

	// thread->fn_abort: (&_main_thread_s)->fn_abort
	// thread->fn_abort: (&_idle_thread_s)->fn_abort
	// thread->fn_abort: (&(&k_sys_work_q)->thread)->fn_abort
	thread->fn_abort = NULL;
	// thread->fn_abort: (&_main_thread_s)->fn_abort: NULL
	// thread->fn_abort: (&_idle_thread_s)->fn_abort: NULL
	// thread->fn_abort: (&(&k_sys_work_q)->thread)->fn_abort: NULL

#ifdef CONFIG_THREAD_CUSTOM_DATA // CONFIG_THREAD_CUSTOM_DATA=n
	/* Initialize custom data field (value is opaque to kernel) */
	thread->custom_data = NULL;
#endif

#if defined(CONFIG_USERSPACE)
	thread->mem_domain_info.mem_domain = NULL;
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_THREAD_STACK_INFO) // CONFIG_THREAD_STACK_INFO=n
	thread->stack_info.start = (u32_t)pStack;
	thread->stack_info.size = (u32_t)stackSize;
#endif /* CONFIG_THREAD_STACK_INFO */
}

#if defined(CONFIG_THREAD_MONITOR) // CONFIG_THREAD_MONITOR=n
/*
 * Add a thread to the kernel's list of active threads.
 */
static ALWAYS_INLINE void thread_monitor_init(struct k_thread *thread)
{
	unsigned int key;

	key = irq_lock();
	thread->next_thread = _kernel.threads;
	_kernel.threads = thread;
	irq_unlock(key);
}
#else
// KID 20170523
// KID 20170525
#define thread_monitor_init(thread)		\
	do {/* do nothing */			\
	} while ((0))
#endif /* CONFIG_THREAD_MONITOR */

#endif /* _ASMLANGUAGE */

#endif /* _kernel_structs__h_ */
