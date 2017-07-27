/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <device.h>
#include <misc/util.h>
#include <atomic.h>

// KID 20170614
extern struct device __device_init_start[];
// KID 20170527
extern struct device __device_PRE_KERNEL_1_start[];
// KID 20170527
extern struct device __device_PRE_KERNEL_2_start[];
extern struct device __device_POST_KERNEL_start[];
extern struct device __device_APPLICATION_start[];
// KID 20170614
extern struct device __device_init_end[];

// KID 20170527
// KID 20170529
// KID 20170613
static struct device *config_levels[] = {
	__device_PRE_KERNEL_1_start,
	__device_PRE_KERNEL_2_start,
	__device_POST_KERNEL_start,
	__device_APPLICATION_start,
	/* End marker */
	__device_init_end,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
extern u32_t __device_busy_start[];
extern u32_t __device_busy_end[];
#define DEVICE_BUSY_SIZE (__device_busy_end - __device_busy_start)
#endif

/**
 * @brief Execute all the device initialization functions at a given level
 *
 * @details Invokes the initialization routine for each device object
 * created by the DEVICE_INIT() macro using the specified level.
 * The linker script places the device objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
// KID 20170527
// _SYS_INIT_LEVEL_PRE_KERNEL_1: 0
// KID 20170613
// _SYS_INIT_LEVEL_PRE_KERNEL_2: 1
// KID 20170713
// _SYS_INIT_LEVEL_POST_KERNEL: 2
// KID 20170726
// _SYS_INIT_LEVEL_POST_KERNEL: 2
void _sys_device_do_config_level(int level)
{
	struct device *info;

	// NOTE:
	// include/arch/x86/linker.ld 파일 참고
	// section ".init_PRE_KERNEL_1[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 순으로 분석 진행

	// NOTE:
	// include/arch/x86/linker.ld 파일 참고
	// section ".init_PRE_KERNEL_2[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 순으로 분석 진행

	// NOTE:
	// include/arch/x86/linker.ld 파일 참고
	// section ".init_POST_KERNEL[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 순으로 분석 진행

	// level: 0, config_levels[0]: __device_PRE_KERNEL_1_start, config_levels[1]: __device_PRE_KERNEL_2_start
	// level: 1, config_levels[1]: __device_PRE_KERNEL_2_start, config_levels[2]: __device_POST_KERNEL_start
	// level: 2, config_levels[2]: __device_POST_KERNEL_start, config_levels[2]: __device_APPLICATION_start
	for (info = config_levels[level]; info < config_levels[level+1];
								info++) {
		// info: __device_sys_init_init_static_pools0
		// info->config: (__device_sys_init_init_static_pools0))->config: &__config_sys_init_init_static_pools0
		// info: __device_sys_init_init_pipes_module0
		// info->config: (__device_sys_init_init_pipes_module0))->config: &__config_sys_init_init_pipes_module0
		// info: __device_sys_init_init_mem_slab_module0
		// info->config: (__device_sys_init_init_mem_slab_module0))->config: &__config_sys_init_init_mem_slab_module0
		// info: __device_sys_init_init_mbox_module0
		// info->config: (__device_sys_init_init_mbox_module0))->config: &__config_sys_init_init_mbox_module0
		// info: __device_sys_init_init_cache0
		// info->config: (__device_sys_init_init_cache0))->config: &__config_sys_init_init_cache0
		// info: __device_sys_init__loapic_init0
		// info->config: (__device_sys_init__loapic_init0))->config: &__config_sys_init__loapic_init0
		// info: __device_sys_init__ioapic_init0
		// info->config: (__device_sys_init__ioapic_init0))->config: &__config_sys_init__ioapic_init0
		// info: __device_sys_init_uart_console_init0
		// info->config: (__device_sys_init_uart_console_init0))->config: &__config_sys_init_uart_console_init0
		// info: __device_sys_init_k_sys_work_q_init0
		// info->config: (__device_sys_init_k_sys_work_q_init0))->config: &__config_sys_init_k_sys_work_q_init0
		//
		// info: __device_sys_init_pinmux_initialize0
		// info->config: (__device_sys_init_pinmux_initialize0))->config: &__config_sys_init_pinmux_initialize0
		struct device_config *device = info->config;
		// device: &__config_sys_init_init_static_pools0
		// device: &__config_sys_init_init_pipes_module0
		// device: &__config_sys_init_init_mem_slab_module0
		// device: &__config_sys_init_init_mbox_module0
		// device: &__config_sys_init_init_cache0
		// device: &__config_sys_init__loapic_init0
		// device: &__config_sys_init__ioapic_init0
		// device: &__config_sys_init_uart_console_init0
		// device: &__config_sys_init_k_sys_work_q_init0
		//
		// device: &__config_sys_init_pinmux_initialize0

		// device->init: (&__config_sys_init_init_static_pools0)->init: init_static_pools,
		// info: __device_sys_init_init_static_pools0
		// init_static_pools(__device_sys_init_init_static_pools0): 0
		// device->init: (&__config_sys_init_init_pipes_module0)->init: init_pipes_module,
		// info: __device_sys_init_init_pipes_module0
		// init_pipes_module(__device_sys_init_init_pipes_module0): 0
		// device->init: (&__config_sys_init_init_mem_slab_module0)->init: init_mem_slab_module,
		// info: __device_sys_init_init_mem_slab_module0
		// init_mem_slab_module(__device_sys_init_init_mem_slab_module0): 0
		// device->init: (&__config_sys_init_init_mbox_module0)->init: init_mbox_module,
		// info: __device_sys_init_init_mbox_module0
		// init_mbox_module(__device_sys_init_init_mbox_module0): 0
		// device->init: (&__config_sys_init_init_cache0)->init: init_cache,
		// info: __device_sys_init_init_cache0
		// init_cache(__device_sys_init_init_cache0): 0
		// device->init: (&__config_sys_init__loapic_init0)->init: _loapic_init,
		// info: __device_sys_init__loapic_init0
		// _loapic_init(__device_sys_init__loapic_init0): 0
		// device->init: (&__config_sys_init__ioapic_init0)->init: _ioapic_init,
		// info: __device_sys_init__ioapic_init0
		// _ioapic_init(__device_sys_init__ioapic_init0): 0
		// device->init: (&__config_sys_init_uart_console_init0)->init: uart_console_init,
		// info: __device_sys_init_uart_console_init0
		// uart_console_init(__device_sys_init_uart_console_init0): 0
		// device->init: (&__config_sys_init_k_sys_work_q_init0)->init: k_sys_work_q_init,
		// info: __device_sys_init_k_sys_work_q_init0
		// k_sys_work_q_init(__device_sys_init_k_sys_work_q_init0): 0
		//
		// device->init: (&__config_sys_init_pinmux_initialize0)->init: pinmux_initialize,
		// info: __device_sys_init_pinmux_initialize0
		// pinmux_initialize(__device_sys_init_pinmux_initialize0): 0
		device->init(info);

		// init_static_pools 에서 한일:
		// 없음

		// init_pipes_module 에서 한일;
		// async_msg[0...9].thread.thread_state: 0x1
		// async_msg[0...9].thread.swap_data: &async_msg[0...9].desc
		//
		// _k_stack_buf_pipe_async_msgs[0...9]: (u32_t)&async_msg[0...9]
		// (&pipe_async_msgs)->next: &_k_stack_buf_pipe_async_msgs[9]

		// init_mem_slab_module 에서 한일:
		// 없음

		// init_mbox_module 에서 한일:
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

		// init_cache 에서 한일:
		// 없음

		// _loapic_init 에서 한일:
		// 정리 필요

		//_ioapic_init 에서 한일:
		// 정리 필요

		// uart_console_init 에서 한일
		// _char_out: console_out

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
	}
}

// KID 20170614
// CONFIG_UART_CONSOLE_ON_DEV_NAME: "UART_1"
struct device *device_get_binding(const char *name)
{
	struct device *info;

	// NOTE:
	// include/linker-defs.h 파일 참고
	// 코드 영역 __device_init_start 에서 __device_init_end 사이에 있는 코드들을 순차적으로 찾음
	// SYS_INIT 메크로에 정의된 struct device 의 구조체 값들 중에는 "UART_1" 이름의 장치가 없는 것으로 보임

	for (info = __device_init_start; info != __device_init_end; info++) {
		// name: "UART_1"
		if (info->driver_api && !strcmp(name, info->config->name)) {
			return info;
		}
	}

	return NULL;
	// return NULL
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
int device_pm_control_nop(struct device *unused_device,
		       u32_t unused_ctrl_command, void *unused_context)
{
	return 0;
}

void device_list_get(struct device **device_list, int *device_count)
{

	*device_list = __device_init_start;
	*device_count = __device_init_end - __device_init_start;
}


int device_any_busy_check(void)
{
	int i = 0;

	for (i = 0; i < DEVICE_BUSY_SIZE; i++) {
		if (__device_busy_start[i] != 0) {
			return -EBUSY;
		}
	}
	return 0;
}

int device_busy_check(struct device *chk_dev)
{
	if (atomic_test_bit((const atomic_t *)__device_busy_start,
				 (chk_dev - __device_init_start))) {
		return -EBUSY;
	}
	return 0;
}

#endif

void device_busy_set(struct device *busy_dev)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	atomic_set_bit((atomic_t *) __device_busy_start,
				 (busy_dev - __device_init_start));
#else
	ARG_UNUSED(busy_dev);
#endif
}

void device_busy_clear(struct device *busy_dev)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	atomic_clear_bit((atomic_t *) __device_busy_start,
				 (busy_dev - __device_init_start));
#else
	ARG_UNUSED(busy_dev);
#endif
}
