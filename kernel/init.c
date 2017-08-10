/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel initialization module
 *
 * This module contains routines that are used to initialize the kernel.
 */

#include <zephyr.h>
#include <offsets_short.h>
#include <kernel.h>
#include <misc/printk.h>
#include <misc/stack.h>
#include <drivers/rand32.h>
#include <linker/sections.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <device.h>
#include <init.h>
#include <linker/linker-defs.h>
#include <ksched.h>
#include <version.h>
#include <string.h>

/* kernel build timestamp items */

// KID 20170613
// BUILD_TIMESTAMP: "BUILD: " __DATE__ " " __TIME__
#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP // CONFIG_BUILD_TIMESTAMP=y
// BUILD_TIMESTAMP: "BUILD: " __DATE__ " " __TIME__
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

static const unsigned int boot_delay;
#if defined(CONFIG_BOOT_DELAY) && CONFIG_BOOT_DELAY > 0
#define BOOT_DELAY_BANNER " (delayed boot "	\
	STRINGIFY(CONFIG_BOOT_DELAY) "ms)"
static const unsigned int boot_delay = CONFIG_BOOT_DELAY;
#else
#define BOOT_DELAY_BANNER ""
static const unsigned int boot_delay;
#endif
// KID 20170613
// KERNEL_VERSION_STRING: "1.8.99"
// BOOT_BANNER: "BOOTING ZEPHYR OS v" "1.8.99"
#define BOOT_BANNER "BOOTING ZEPHYR OS v"	\
	KERNEL_VERSION_STRING BOOT_DELAY_BANNER

#if !defined(CONFIG_BOOT_BANNER) // CONFIG_BOOT_BANNER=y
#define PRINT_BOOT_BANNER() do { } while (0)
#elif !defined(CONFIG_BUILD_TIMESTAMP) // CONFIG_BUILD_TIMESTAMP=y
#define PRINT_BOOT_BANNER() printk("***** " BOOT_BANNER " *****\n")
#else
// KID 20170613
// BOOT_BANNER: "BOOTING ZEPHYR OS v" "1.8.99"
// build_timestamp: "BUILD: " __DATE__ " " __TIME__
#define PRINT_BOOT_BANNER() \
	printk("***** " BOOT_BANNER " - %s *****\n", build_timestamp)
#endif

/* boot time measurement items */

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
u64_t __noinit __start_time_stamp; /* timestamp when kernel starts */
u64_t __noinit __main_time_stamp;  /* timestamp when main task starts */
u64_t __noinit __idle_time_stamp;  /* timestamp when CPU goes idle */
#endif

#ifdef CONFIG_EXECUTION_BENCHMARKING
u64_t __noinit __start_swap_tsc;
u64_t __noinit __end_swap_tsc;
u64_t __noinit __start_intr_tsc;
u64_t __noinit __end_intr_tsc;
u64_t __noinit __start_tick_tsc;
u64_t __noinit __end_tick_tsc;
#endif
/* init/main and idle threads */

// KID 20170525
// KID 20170612
// CONFIG_IDLE_STACK_SIZE: 256
// IDLE_STACK_SIZE: 256
#define IDLE_STACK_SIZE CONFIG_IDLE_STACK_SIZE

#if CONFIG_MAIN_STACK_SIZE & (STACK_ALIGN - 1)
    #error "MAIN_STACK_SIZE must be a multiple of the stack alignment"
#endif

#if IDLE_STACK_SIZE & (STACK_ALIGN - 1)
    #error "IDLE_STACK_SIZE must be a multiple of the stack alignment"
#endif

// KID 20170519
// KID 20170612
// CONFIG_MAIN_STACK_SIZE: 1024
// MAIN_STACK_SIZE: 1024
#define MAIN_STACK_SIZE CONFIG_MAIN_STACK_SIZE

// KID 20170519
// KID 20170522
// KID 20170612
// MAIN_STACK_SIZE: 1024
//
// K_THREAD_STACK_DEFINE(_main_stack, 1024):
// char __attribute__((section("." "noinit" "." "_FILE_PATH_HASH" "." "__COUNTER__"))) __aligned(4) _main_stack[1024]
K_THREAD_STACK_DEFINE(_main_stack, MAIN_STACK_SIZE);

// KID 20170525
// IDLE_STACK_SIZE: 256
//
// K_THREAD_STACK_DEFINE(_idle_stack, 256);
// char __attribute__((section("." "noinit" "." "_FILE_PATH_HASH" "." "__COUNTER__"))) __aligned(4) _idle_stack[256]
K_THREAD_STACK_DEFINE(_idle_stack, IDLE_STACK_SIZE);

// KID 20170519
// KID 20170712
// KID 20170720
static struct k_thread _main_thread_s;
// KID 20170525
static struct k_thread _idle_thread_s;

// KID 20170519
k_tid_t const _main_thread = (k_tid_t)&_main_thread_s;
// KID 20170524
// KID 20170525
k_tid_t const _idle_thread = (k_tid_t)&_idle_thread_s;

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during kernel initialization,
 * since the kernel hasn't yet set up its own stack areas. The dual purposing
 * of this area is safe since interrupts are disabled until the kernel context
 * switches to the init thread.
 */
#if CONFIG_ISR_STACK_SIZE & (STACK_ALIGN - 1)
    #error "ISR_STACK_SIZE must be a multiple of the stack alignment"
#endif
// KID 20170516
// KID 20170517
// KID 20170527
// CONFIG_ISR_STACK_SIZE: 2048
K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

#ifdef CONFIG_SYS_CLOCK_EXISTS // CONFIG_SYS_CLOCK_EXISTS=y
	#include <misc/dlist.h>
// KID 20170527
// _timeout_q: _kernel.timeout_q
	#define initialize_timeouts() do { \
		sys_dlist_init(&_timeout_q); \
	} while ((0))
#else
	#define initialize_timeouts() do { } while ((0))
#endif

extern void idle(void *unused1, void *unused2, void *unused3);

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
extern K_THREAD_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);


void k_call_stacks_analyze(void)
{
	printk("Kernel stacks:\n");
	STACK_ANALYZE("main     ", _main_stack);
	STACK_ANALYZE("idle     ", _idle_stack);
	STACK_ANALYZE("interrupt", _interrupt_stack);
	STACK_ANALYZE("workqueue", sys_work_q_stack);
}
#else
void k_call_stacks_analyze(void) { }
#endif

/**
 *
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 *
 * @return N/A
 */
void _bss_zero(void)
{
	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));
#ifdef CONFIG_APPLICATION_MEMORY
	memset(&__app_bss_start, 0,
		 ((u32_t) &__app_bss_end - (u32_t) &__app_bss_start));
#endif
}


#ifdef CONFIG_XIP
/**
 *
 * @brief Copy the data section from ROM to RAM
 *
 * This routine copies the data section from ROM to RAM.
 *
 * @return N/A
 */
void _data_copy(void)
{
	memcpy(&__data_ram_start, &__data_rom_start,
		 ((u32_t) &__data_ram_end - (u32_t) &__data_ram_start));
#ifdef CONFIG_APPLICATION_MEMORY
	memcpy(&__app_data_ram_start, &__app_data_rom_start,
		 ((u32_t) &__app_data_ram_end - (u32_t) &__app_data_ram_start));
#endif
}
#endif

/**
 *
 * @brief Mainline for kernel's background task
 *
 * This routine completes kernel initialization by invoking the remaining
 * init functions, then invokes application's main() routine.
 *
 * @return N/A
 */
// KID 20170519
// KID 20170712
// KID 20170713
// NULL, NULL, NULL
// KID 20170726
// NULL, NULL, NULL
static void _main(void *unused1, void *unused2, void *unused3)
{
	// unused1: NULL
	ARG_UNUSED(unused1);

	// unused2: NULL
	ARG_UNUSED(unused2);

	// unused3: NULL
	ARG_UNUSED(unused3);

	// _SYS_INIT_LEVEL_POST_KERNEL: 2
	_sys_device_do_config_level(_SYS_INIT_LEVEL_POST_KERNEL);

	// _sys_device_do_config_level 에서 한일:
	// section ".init_POST_KERNEL[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 순으로 분석 진행
	//
	// k_sys_work_q_init 에서 한일
	// 워크큐 생성 및 초기화 수행
	// (&(&(&k_sys_work_q)->fifo)->data_q)->head: NULL
	// (&(&(&k_sys_work_q)->fifo)->data_q)->tail: NULL
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->head: &(&(&k_sys_work_q)->fifo)->wait_q
	// (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->fifo)->wait_q
	//
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
	//
	// pinmux_initialize 에서 한일:
	// Arduino 보드에 맞는 pin 의 기능을 mux 하여 해달 gpio register에 설정함
	//
	// (*(volatile u32_t *) 0xb0800930): 0x10064555
	// (*(volatile u32_t *) 0xb0800934): 0xA
	// (*(volatile u32_t *) 0xb0800938): 0x50000
	// (*(volatile u32_t *) 0xb080093C): 0x40054000
	// (*(volatile u32_t *) 0xb0800940): 0x15
	//
	// *(0xb0800948):  0x100

	/* Final init level before app starts */
	_sys_device_do_config_level(_SYS_INIT_LEVEL_APPLICATION);

	// _sys_device_do_config_level 에서 한일:
	// 없음

#ifdef CONFIG_CPLUSPLUS // CONFIG_CPLUSPLUS=n
	/* Process the .ctors and .init_array sections */
	extern void __do_global_ctors_aux(void);
	extern void __do_init_array_aux(void);
	__do_global_ctors_aux();
	__do_init_array_aux();
#endif

	_init_static_threads();

	if (boot_delay > 0) {
		printk("***** delaying boot " STRINGIFY(CONFIG_BOOT_DELAY)
		       "ms (per build configuration) *****\n");
		k_sleep(CONFIG_BOOT_DELAY);
	}
	PRINT_BOOT_BANNER();

	// PRINT_BOOT_BANNER 에서 한일:
	// "***** " "BOOTING ZEPHYR OS v" "1.8.99" " - %s *****\n", "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/* record timestamp for kernel's _main() function */
	extern u64_t __main_time_stamp;

	__main_time_stamp = (u64_t)k_cycle_get_32();
#endif

	extern void main(void);

	main();

	/* Terminate thread normally since it has no more work to do */
	_main_thread->base.user_options &= ~K_ESSENTIAL;
}

void __weak main(void)
{
	/* NOP default main() if the application does not provide one. */
}

/**
 *
 * @brief Initializes kernel data structures
 *
 * This routine initializes various kernel data structures, including
 * the init and idle threads and any architecture-specific initialization.
 *
 * Note that all fields of "_kernel" are set to zero on entry, which may
 * be all the initialization many of them require.
 *
 * @return N/A
 */
// KID 20170518
// dummy_thread: &dummy_thread_memory
static void prepare_multithreading(struct k_thread *dummy_thread)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN // CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN=n
	ARG_UNUSED(dummy_thread);
#else
	/*
	 * Initialize the current execution thread to permit a level of
	 * debugging output if an exception should happen during kernel
	 * initialization.  However, don't waste effort initializing the
	 * fields of the dummy thread beyond those needed to identify it as a
	 * dummy thread.
	 */

	// _current: _kernel.current, dummy_thread: &dummy_thread_memory
	_current = dummy_thread;
	// _kernel.current: &dummy_thread_memory

	// dummy_thread->base.user_options: (struct k_thread* &dummy_thread_memory)->base.user_options,
	// K_ESSENTIAL: 0x1
	dummy_thread->base.user_options = K_ESSENTIAL;
	// dummy_thread->base.user_options: (struct k_thread* &dummy_thread_memory)->base.user_options: 0x1

	// dummy_thread->base.thread_state: (struct k_thread* &dummy_thread_memory)->base.thread_state,
	// _THREAD_DUMMY: 0x1
	dummy_thread->base.thread_state = _THREAD_DUMMY;
	// dummy_thread->base.thread_state: (struct k_thread* &dummy_thread_memory)->base.thread_state: 0x1
#endif

	/* _kernel.ready_q is all zeroes */


	/*
	 * The interrupt library needs to be initialized early since a series
	 * of handlers are installed into the interrupt table to catch
	 * spurious interrupts. This must be performed before other kernel
	 * subsystems install bonafide handlers, or before hardware device
	 * drivers are initialized.
	 */

	_IntLibInit(); // null function

	/* ready the init/main and idle threads */

	// K_NUM_PRIORITIES: 32
	for (int ii = 0; ii < K_NUM_PRIORITIES; ii++) {
		// ii: 0, _ready_q.q[0]: _kernel.ready_q.q[0]
		sys_dlist_init(&_ready_q.q[ii]);

		// sys_dlist_init 에서 한일:
		// (&_kernel.ready_q.q[0])->head: &_kernel.ready_q.q[0]
		// (&_kernel.ready_q.q[0])->tail: &_kernel.ready_q.q[0]

		// ii: 1...31 까지 루프 수행
	}

	// 위 루프 수행 결과:
	// ready_q 의 list를 초기화함
	//
	// (&_kernel.ready_q.q[0...31])->head: &_kernel.ready_q.q[0...31]
	// (&_kernel.ready_q.q[0...31])->tail: &_kernel.ready_q.q[0...31]

	/*
	 * prime the cache with the main thread since:
	 *
	 * - the cache can never be NULL
	 * - the main thread will be the one to run first
	 * - no other thread is initialized yet and thus their priority fields
	 *   contain garbage, which would prevent the cache loading algorithm
	 *   to work as intended
	 */
	// _ready_q.cache: _kernel.ready_q.cache, _main_thread: &_main_thread_s
	_ready_q.cache = _main_thread;
	// _ready_q.cache: _kernel.ready_q.cache: &_main_thread_s

	// _main_thread: &_main_thread_s, MAIN_STACK_SIZE: 1024, CONFIG_MAIN_THREAD_PRIORITY: 0
	// K_ESSENTIAL: 0x1
	_new_thread(_main_thread, _main_stack,
		    MAIN_STACK_SIZE, _main, NULL, NULL, NULL,
		    CONFIG_MAIN_THREAD_PRIORITY, K_ESSENTIAL);

	// _new_thread 에서 한일:
	// (&(&_main_thread_s)->base)->user_options: 0x1
	// (&(&_main_thread_s)->base)->thread_state: 0x4
	// (&(&_main_thread_s)->base)->prio: 0
	// (&(&_main_thread_s)->base)->sched_locked: 0
	// (&(&(&_main_thread_s)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&_main_thread_s)->base)->timeout)->wait_q: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->thread: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->func: NULL
	//
	// (&_main_thread_s)->init_data: NULL
	// (&_main_thread_s)->fn_abort: NULL
	//
	// *(unsigned long *)(_main_stack + 1024): NULL
	// *(unsigned long *)(_main_stack + 1020): NULL
	// *(unsigned long *)(_main_stack + 1016): NULL
	// *(unsigned long *)(_main_stack + 1016): _main
	// *(unsigned long *)(_main_stack + 1012): eflag 레지스터 값 | 0x00000200
	// *(unsigned long *)(_main_stack + 1008): _thread_entry_wrapper
	//
	// (&_main_thread_s)->callee_saved.esp: _main_stack + 980

	// _main_thread: &_main_thread_s
	_mark_thread_as_started(_main_thread);

	// _mark_thread_as_started 에서 한일:
	// (&_main_thread_s)->base.thread_state: 0

	// _main_thread: &_main_thread_s
	_add_thread_to_ready_q(_main_thread);

	// _add_thread_to_ready_q 에서 한일:
	// _kernel.ready_q.prio_bmap[0]: 0x10000
	//
	// (&(&_main_thread_s)->base.k_q_node)->next: &_kernel.ready_q.q[16]
	// (&(&_main_thread_s)->base.k_q_node)->prev: (&_kernel.ready_q.q[16])->tail
	// (&_kernel.ready_q.q[16])->tail->next: &(&_main_thread_s)->base.k_q_node
	// (&_kernel.ready_q.q[16])->tail: &(&_main_thread_s)->base.k_q_node
	//
	// _kernel.ready_q.cache: &_main_thread_s

#ifdef CONFIG_MULTITHREADING // CONFIG_MULTITHREADING=y
	// _idle_thread: &_idle_thread_s, IDLE_STACK_SIZE: 256, K_LOWEST_THREAD_PRIO: 15, K_ESSENTIAL: 0x1
	_new_thread(_idle_thread, _idle_stack, IDLE_STACK_SIZE, idle, NULL, NULL, NULL,
		    K_LOWEST_THREAD_PRIO, K_ESSENTIAL);

	// _new_thread 에서 한일:
	// (&(&_idle_thread_s)->base)->user_options: 0x1
	// (&(&_idle_thread_s)->base)->thread_state: 0x4
	// (&(&_idle_thread_s)->base)->prio: 15
	// (&(&_idle_thread_s)->base)->sched_locked: 0
	// (&(&(&_idle_thread_s)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&_idle_thread_s)->base)->timeout)->wait_q: NULL
	// (&(&(&_idle_thread_s)->base)->timeout)->thread: NULL
	// (&(&(&_idle_thread_s)->base)->timeout)->func: NULL
	//
	// (&_idle_thread_s)->init_data: NULL
	// (&_idle_thread_s)->fn_abort: NULL
	//
	// *(unsigned long *)(_idle_stack + 256): NULL
	// *(unsigned long *)(_idle_stack + 252): NULL
	// *(unsigned long *)(_idle_stack + 248): NULL
	// *(unsigned long *)(_idle_stack + 244): idle
	// *(unsigned long *)(_idle_stack + 240): eflag 레지스터 값 | 0x00000200
	// *(unsigned long *)(_idle_stack + 236): _thread_entry_wrapper
	//
	// (&_idle_thread_s)->callee_saved.esp: _idle_stack + 212

	// _idle_thread: &_idle_thread_s
	_mark_thread_as_started(_idle_thread);

	// _mark_thread_as_started 에서 한일:
	// (&_idle_thread_s)->base.thread_state: 0

	// _idle_thread: &_idle_thread_s
	_add_thread_to_ready_q(_idle_thread);

	// _add_thread_to_ready_q 에서 한일:
	// _kernel.ready_q.prio_bmap[0]: 0x80010000
	//
	// (&(&_idle_thread_s)->base.k_q_node)->next: &_kernel.ready_q.q[31]
	// (&(&_idle_thread_s)->base.k_q_node)->prev: (&_kernel.ready_q.q[31])->tail
	// (&_kernel.ready_q.q[31])->tail->next: &(&_idle_thread_s)->base.k_q_node
	// (&_kernel.ready_q.q[31])->tail: &(&_idle_thread_s)->base.k_q_node
	//
	// _kernel.ready_q.cache: &_main_thread_s
#endif

	initialize_timeouts();

	// initialize_timeouts 에서 한일:
	// (&_kernel.timeout_q)->head: &_kernel.timeout_q
	// (&_kernel.timeout_q)->tail: &_kernel.timeout_q

	/* perform any architecture-specific initialization */

	kernel_arch_init();

	// kernel_arch_init 에서 한일:
	// _kernel.nested: 0
	// _kernel.irq_stack: _interrupt_stack + 2048
}

// KID 20170618
static void switch_to_main_thread(void)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN // CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN=n
	_arch_switch_to_main_thread(_main_thread, _main_stack, MAIN_STACK_SIZE,
				    _main);
#else
	/*
	 * Context switch to main task (entry function is _main()): the
	 * current fake thread is not on a wait queue or ready queue, so it
	 * will never be rescheduled in.
	 */

	// irq_lock(): eflags 값
	_Swap(irq_lock());
#endif
}

#ifdef CONFIG_STACK_CANARIES
extern void *__stack_chk_guard;
#endif

/**
 *
 * @brief Initialize kernel
 *
 * This routine is invoked when the system is ready to run C code. The
 * processor must be running in 32-bit mode, and the BSS must have been
 * cleared/zeroed.
 *
 * @return Does not return
 */
// KID 20170516
// KID 20170517
FUNC_NORETURN void _Cstart(void)
{
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN // CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN=n
	struct k_thread *dummy_thread = NULL;
#else
	// __stack: __aligned(4), _K_THREAD_NO_FLOAT_SIZEOF: 56
	struct k_thread dummy_thread_memory;
	struct k_thread *dummy_thread = &dummy_thread_memory;
	// dummy_thread: &dummy_thread_memory
#endif

	/*
	 * Initialize kernel data structures. This step includes
	 * initializing the interrupt subsystem, which must be performed
	 * before the hardware initialization phase.
	 */

	// dummy_thread: &dummy_thread_memory
	prepare_multithreading(dummy_thread);

	// prepare_multithreading 에서 한일:
	// _kernel.current: &dummy_thread_memory
	// (struct k_thread* &dummy_thread_memory)->base.user_options: 0x1
	// (struct k_thread* &dummy_thread_memory)->base.thread_state: 0x1
	//
	// sys_dlist_init 에서 한일:
	// ready_q 의 list를 초기화함
	//
	// (&_kernel.ready_q.q[0...31])->head: &_kernel.ready_q.q[0...31]
	// (&_kernel.ready_q.q[0...31])->tail: &_kernel.ready_q.q[0...31]
	//
	// _kernel.ready_q.cache: &_main_thread_s
	//
	// _new_thread 에서 한일:
	// (&(&_main_thread_s)->base)->user_options: 0x1
	// (&(&_main_thread_s)->base)->thread_state: 0x4
	// (&(&_main_thread_s)->base)->prio: 0
	// (&(&_main_thread_s)->base)->sched_locked: 0
	// (&(&(&_main_thread_s)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&_main_thread_s)->base)->timeout)->wait_q: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->thread: NULL
	// (&(&(&_main_thread_s)->base)->timeout)->func: NULL
	//
	// (&_main_thread_s)->init_data: NULL
	// (&_main_thread_s)->fn_abort: NULL
	//
	// *(unsigned long *)(_main_stack + 1024): NULL
	// *(unsigned long *)(_main_stack + 1020): NULL
	// *(unsigned long *)(_main_stack + 1016): NULL
	// *(unsigned long *)(_main_stack + 1016): _main
	// *(unsigned long *)(_main_stack + 1012): eflag 레지스터 값 | 0x00000200
	// *(unsigned long *)(_main_stack + 1008): _thread_entry_wrapper
	//
	// (&_main_thread_s)->callee_saved.esp: _main_stack + 980
	//
	// _mark_thread_as_started 에서 한일:
	// (&_main_thread_s)->base.thread_state: 0
	//
	// _add_thread_to_ready_q 에서 한일:
	// _kernel.ready_q.prio_bmap[0]: 0x10000
	//
	// (&(&_main_thread_s)->base.k_q_node)->next: &_kernel.ready_q.q[16]
	// (&(&_main_thread_s)->base.k_q_node)->prev: (&_kernel.ready_q.q[16])->tail
	// (&_kernel.ready_q.q[16])->tail->next: &(&_main_thread_s)->base.k_q_node
	// (&_kernel.ready_q.q[16])->tail: &(&_main_thread_s)->base.k_q_node
	//
	// _kernel.ready_q.cache: &_main_thread_s
	//
	// _new_thread 에서 한일:
	// (&(&_idle_thread_s)->base)->user_options: 0x1
	// (&(&_idle_thread_s)->base)->thread_state: 0x4
	// (&(&_idle_thread_s)->base)->prio: 15
	// (&(&_idle_thread_s)->base)->sched_locked: 0
	// (&(&(&_idle_thread_s)->base)->timeout)->delta_ticks_from_prev: -1
	// (&(&(&_idle_thread_s)->base)->timeout)->wait_q: NULL
	// (&(&(&_idle_thread_s)->base)->timeout)->thread: NULL
	// (&(&(&_idle_thread_s)->base)->timeout)->func: NULL
	//
	// (&_idle_thread_s)->init_data: NULL
	// (&_idle_thread_s)->fn_abort: NULL
	//
	// *(unsigned long *)(_idle_stack + 256): NULL
	// *(unsigned long *)(_idle_stack + 252): NULL
	// *(unsigned long *)(_idle_stack + 248): NULL
	// *(unsigned long *)(_idle_stack + 244): idle
	// *(unsigned long *)(_idle_stack + 240): eflag 레지스터 값 | 0x00000200
	// *(unsigned long *)(_idle_stack + 236): _thread_entry_wrapper
	//
	// (&_idle_thread_s)->callee_saved.esp: _idle_stack + 212
	//
	// _mark_thread_as_started 에서 한일:
	// (&_idle_thread_s)->base.thread_state: 0
	//
	// _add_thread_to_ready_q 에서 한일:
	// _kernel.ready_q.prio_bmap[0]: 0x80010000
	//
	// (&(&_idle_thread_s)->base.k_q_node)->next: &_kernel.ready_q.q[31]
	// (&(&_idle_thread_s)->base.k_q_node)->prev: (&_kernel.ready_q.q[31])->tail
	// (&_kernel.ready_q.q[31])->tail->next: &(&_idle_thread_s)->base.k_q_node
	// (&_kernel.ready_q.q[31])->tail: &(&_idle_thread_s)->base.k_q_node
	//
	// _kernel.ready_q.cache: &_main_thread_s
	//
	// initialize_timeouts 에서 한일:
	// (&_kernel.timeout_q)->head: &_kernel.timeout_q
	// (&_kernel.timeout_q)->tail: &_kernel.timeout_q
	//
	// kernel_arch_init 에서 한일:
	// _kernel.nested: 0
	// _kernel.irq_stack: _interrupt_stack + 2048

	/* perform basic hardware initialization */
	// _SYS_INIT_LEVEL_PRE_KERNEL_1: 0
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_1);

	// _sys_device_do_config_level 에서 한일;
	// section ".init_PRE_KERNEL_1[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 초기화 진행
	// init_static_pools
	// init_pipes_module
	// init_mem_slab_module
	// init_mbox_module
	// init_cache
	// _loapic_init
	// _ioapic_init
	// uart_console_init 순으로 초기화 진행
	//
	// async_msg[0...9].thread.thread_state: 0x1
	// async_msg[0...9].thread.swap_data: &async_msg[0...9].desc
	//
	// _k_stack_buf_pipe_async_msgs[0...9]: (u32_t)&async_msg[0...9]
	// (&pipe_async_msgs)->next: &_k_stack_buf_pipe_async_msgs[9]
	//
	// (&async_msg[0...9].thread)->user_options: 0x0
	// (&async_msg[0...9].thread)->thread_state: 0x1
	// (&async_msg[0...9].thread)->prio: 0
	// (&async_msg[0...9].thread)->sched_locked: 0
	// (&(&async_msg[0...9].thread)->timeout)->delta_ticks_from_prev: -1
	// (&(&async_msg[0...9].thread)->timeout)->wait_q: NULL
	// (&(&async_msg[0...9].thread)->timeout)->thread: NULL
	// (&(&async_msg[0...9].thread)->timeout)->func: NULL
	//
	// _k_stack_buf_async_msg_free[0...9]: (u32_t)&async_msg[0...9]
	// (&async_msg_free)->next: &_k_stack_buf_async_msg_free[9]
	//
	// _char_out: console_out

	// _SYS_INIT_LEVEL_PRE_KERNEL_2: 1
	_sys_device_do_config_level(_SYS_INIT_LEVEL_PRE_KERNEL_2);

	// _sys_device_do_config_level 에서 한일;
	// section ".init_PRE_KERNEL_2[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 초기화 진행
	// 현재 코드 기준 컴파일 된 코드가 없음

	/* initialize stack canaries */
#ifdef CONFIG_STACK_CANARIES // CONFIG_STACK_CANARIES=n
	__stack_chk_guard = (void *)sys_rand32_get();
#endif

	/* display boot banner */

	switch_to_main_thread();

	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
