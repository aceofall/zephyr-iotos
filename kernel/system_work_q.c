/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * System workqueue.
 */

#include <kernel.h>
#include <init.h>

// KID 20170715
// KID 20170717
// CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE: 1024
//
// K_THREAD_STACK_DEFINE(sys_work_q_stack, 1024):
// char __attribute__((section("." "noinit" "." "_FILE_PATH_HASH" "." "__COUNTER__"))) __aligned(4) sys_work_q_stack[1024]
K_THREAD_STACK_DEFINE(sys_work_q_stack, CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);

// KID 20170715
// KID 20170717
// KID 20170719
struct k_work_q k_sys_work_q;

// KID 20170715
// __device_sys_init_k_sys_work_q_init0
static int k_sys_work_q_init(struct device *dev)
{
	// dev: __device_sys_init_k_sys_work_q_init0
	ARG_UNUSED(dev);

	// K_THREAD_STACK_SIZEOF(sys_work_q_stack): 1024, CONFIG_SYSTEM_WORKQUEUE_PRIORITY: -1
	k_work_q_start(&k_sys_work_q,
		       sys_work_q_stack,
		       K_THREAD_STACK_SIZEOF(sys_work_q_stack),
		       CONFIG_SYSTEM_WORKQUEUE_PRIORITY);

	return 0;
}

// KID 20170714
// CONFIG_KERNEL_INIT_PRIORITY_DEFAULT: 40
//
// SYS_INIT(k_sys_work_q_init, POST_KERNEL, 40):
// static struct device_config __config_sys_init_k_sys_work_q_init0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "",
// 	.init = (k_sys_work_q_init),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_k_sys_work_q_init0 __used
// __attribute__((__section__(".init_" "POST_KERNEL" "40"))) = {
// 	 .config = &__config_sys_init_k_sys_work_q_init0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
SYS_INIT(k_sys_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
