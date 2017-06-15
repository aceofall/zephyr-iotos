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
// ARM10C 20170613
// _SYS_INIT_LEVEL_PRE_KERNEL_2: 1
void _sys_device_do_config_level(int level)
{
	struct device *info;

	// NOTE:
	// include/arch/x86/linker.ld 파일 참고
	// section ".init_PRE_KERNEL_1[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 순으로 분석 진행

	// NOTE:
	// include/arch/x86/linker.ld 파일 참고
	// section ".init_PRE_KERNEL_2[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 순으로 분석 진행

	// level: 0, config_levels[0]: __device_PRE_KERNEL_1_start, config_levels[1]: __device_PRE_KERNEL_2_start
	// level: 1, config_levels[1]: __device_PRE_KERNEL_2_start, config_levels[2]: __device_POST_KERNEL_start
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
		struct device_config *device = info->config;
		// device: &__config_sys_init_init_static_pools0
		// device: &__config_sys_init_init_pipes_module0
		// device: &__config_sys_init_init_mem_slab_module0
		// device: &__config_sys_init_init_mbox_module0
		// device: &__config_sys_init_init_cache0
		// device: &__config_sys_init__loapic_init0
		// device: &__config_sys_init__ioapic_init0
		// device: &__config_sys_init_uart_console_init0

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
