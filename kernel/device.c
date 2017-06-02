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

extern struct device __device_init_start[];
// KID 20170527
extern struct device __device_PRE_KERNEL_1_start[];
// KID 20170527
extern struct device __device_PRE_KERNEL_2_start[];
extern struct device __device_POST_KERNEL_start[];
extern struct device __device_APPLICATION_start[];
extern struct device __device_init_end[];

// KID 20170527
// KID 20170529
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
void _sys_device_do_config_level(int level)
{
	struct device *info;

	// NOTE:
	// include/arch/x86/linker.ld 파일 참고
	// section ".init_PRE_KERNEL_1[0-9][0-9]" 순으로 소팅되어 컴파일 된 코드들 순으로 분석 진행

	// level: 0, config_levels[0]: __device_PRE_KERNEL_1_start, config_levels[1]: __device_PRE_KERNEL_2_start
	for (info = config_levels[level]; info < config_levels[level+1];
								info++) {
		// info: __device_sys_init_init_static_pools0
		// info->config: (__device_sys_init_init_static_pools0))->config: &__config_sys_init_init_static_pools0
		// info: __device_sys_init_init_pipes_module0
		// info->config: (__device_sys_init_init_pipes_module0))->config: &__config_sys_init_init_pipes_module0
		struct device_config *device = info->config;
		// device: &__config_sys_init_init_static_pools0
		// device: &__config_sys_init_init_pipes_module0

		// device->init: (&__config_sys_init_init_static_pools0)->init: init_static_pools,
		// info: __device_sys_init_init_static_pools0
		// init_static_pools(__device_sys_init_init_static_pools0): 0
		//
		// device->init: (&__config_sys_init_init_pipes_module0)->init: init_pipes_module,
		// info: __device_sys_init_init_pipes_module0
		// init_pipes_module(__device_sys_init_init_pipes_module0): 0
		device->init(info);

		// init_static_pools 에서 한일:
		// 없음

		// init_pipes_module 에서 한일;
		// async_msg[0...9].thread.thread_state: 0x1
		// async_msg[0...9].thread.swap_data: &async_msg[0...9].desc
		//
		// _k_stack_buf_pipe_async_msgs[0...9]: (u32_t)&async_msg[0...9]
		// (&pipe_async_msgs)->next: &_k_stack_buf_pipe_async_msgs[9]
	}
}

struct device *device_get_binding(const char *name)
{
	struct device *info;

	for (info = __device_init_start; info != __device_init_end; info++) {
		if (info->driver_api && !strcmp(name, info->config->name)) {
			return info;
		}
	}

	return NULL;
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
