
/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INIT_H_
#define _INIT_H_

#include <device.h>
#include <toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * System initialization levels. The PRE_KERNEL_1 and PRE_KERNEL_2 levels are
 * executed in the kernel's initialization context, which uses the interrupt
 * stack. The remaining levels are executed in the kernel's main task.
 */

// KID 20170527
// KID 20170529
// _SYS_INIT_LEVEL_PRE_KERNEL_1: 0
#define _SYS_INIT_LEVEL_PRE_KERNEL_1	0
// KID 20170613
// _SYS_INIT_LEVEL_PRE_KERNEL_2: 1
#define _SYS_INIT_LEVEL_PRE_KERNEL_2	1
#define _SYS_INIT_LEVEL_POST_KERNEL	2
#define _SYS_INIT_LEVEL_APPLICATION	3


/* Counter use to avoid issues if two or more system devices are declared
 * in the same C file with the same init function
 */
// KID 20170530
// _CONCAT(sys_init_, init_static_pools): sys_init_init_static_pools
// _CONCAT(sys_init_init_static_pools, __COUNTER__): sys_init_init_static_pools0
//
// _SYS_NAME(init_static_pools): sys_init_init_static_pools0
#define _SYS_NAME(init_fn) _CONCAT(_CONCAT(sys_init_, init_fn), __COUNTER__)

/**
 * @def SYS_INIT
 *
 * @brief Run an initialization function at boot at specified priority
 *
 * @details This macro lets you run a function at system boot.
 *
 * @param init_fn Pointer to the boot function to run
 *
 * @param level The initialization level, See DEVICE_INIT for details.
 *
 * @param prio Priority within the selected initialization level. See
 * DEVICE_INIT for details.
 */
// KID 20170530
// CONFIG_KERNEL_INIT_PRIORITY_OBJECTS: 30
// _SYS_NAME(init_static_pools): sys_init_init_static_pools0
// DEVICE_INIT(sys_init_init_static_pools0, "", init_static_pools, NULL, NULL, PRE_KERNEL_1, 30):
// static struct device_config __config_sys_init_init_static_pools0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (init_static_pools),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_init_static_pools0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "30"))) = {
// 	 .config = &__config_sys_init_init_static_pools0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
//
// SYS_INIT(init_static_pools, PRE_KERNEL_1, 30):
// static struct device_config __config_sys_init_init_static_pools0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (init_static_pools),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_init_static_pools0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "30"))) = {
// 	 .config = &__config_sys_init_init_static_pools0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
// KID 20170530
// CONFIG_KERNEL_INIT_PRIORITY_OBJECTS: 30
//
// SYS_INIT(init_pipes_module, PRE_KERNEL_1, 30):
// static struct device_config __config_sys_init_init_pipes_module0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (init_pipes_module),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_init_pipes_module0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "30"))) = {
// 	 .config = &__config_sys_init_init_pipes_module0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
// KID 20170530
// CONFIG_KERNEL_INIT_PRIORITY_OBJECTS: 30
//
// SYS_INIT(init_mem_slab_module, PRE_KERNEL_1, 30):
// static struct device_config __config_sys_init_init_mem_slab_module0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (init_mem_slab_module),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_init_mem_slab_module0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "30"))) = {
// 	 .config = &__config_sys_init_init_mem_slab_module0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
// KID 20170530
// CONFIG_KERNEL_INIT_PRIORITY_OBJECTS: 30
//
// SYS_INIT(init_mbox_module, PRE_KERNEL_1, 30):
// static struct device_config __config_sys_init_init_mbox_module0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (init_mbox_module),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_init_mbox_module0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "30"))) = {
// 	 .config = &__config_sys_init_init_mbox_module0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
// KID 20170530
// CONFIG_KERNEL_INIT_PRIORITY_DEFAULT: 40
//
// SYS_INIT(init_cache, PRE_KERNEL_1, 40);
// static struct device_config __config_sys_init_init_cache0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (init_cache),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_init_cache0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "40"))) = {
// 	 .config = &__config_sys_init_init_cache0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
// KID 20170530
// CONFIG_KERNEL_INIT_PRIORITY_DEFAULT: 40
//
// SYS_INIT(_loapic_init, PRE_KERNEL_1, 40):
// static struct device_config __config_sys_init__loapic_init0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (_loapic_init),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init__loapic_init0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "40"))) = {
// 	 .config = &__config_sys_init__loapic_init0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
// KID 20170530
// CONFIG_KERNEL_INIT_PRIORITY_DEFAULT: 40
//
// SYS_INIT(_ioapic_init, PRE_KERNEL_1, 40):
// static struct device_config __config_sys_init__ioapic_init0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (_ioapic_init),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init__ioapic_init0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "40"))) = {
// 	 .config = &__config_sys_init__ioapic_init0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
// KID 20170614
// CONFIG_UART_CONSOLE_INIT_PRIORITY: 60
//
// SYS_INIT(uart_console_init, PRE_KERNEL_1, 60):
// static struct device_config __config_sys_init_uart_console_init0 __used
// __attribute__((__section__(".devconfig.init"))) = {
// 	.name = "", .init = (uart_console_init),
// 	.config_info = (NULL)
// };
// static struct device __device_sys_init_uart_console_init0 __used
// __attribute__((__section__(".init_" "PRE_KERNEL_1" "60"))) = {
// 	 .config = &__config_sys_init_uart_console_init0,
// 	 .driver_api = NULL,
// 	 .driver_data = NULL
// }
#define SYS_INIT(init_fn, level, prio) \
	DEVICE_INIT(_SYS_NAME(init_fn), "", init_fn, NULL, NULL, level, prio)

/**
 * @def SYS_DEVICE_DEFINE
 *
 * @brief Run an initialization function at boot at specified priority,
 * and define device PM control function.
 *
 * @copydetails SYS_INIT
 * @param pm_control_fn Pointer to device_pm_control function.
 * Can be empty function (device_pm_control_nop) if not implemented.
 * @param drv_name Name of this system device
 */
#define SYS_DEVICE_DEFINE(drv_name, init_fn, pm_control_fn, level, prio) \
	DEVICE_DEFINE(_SYS_NAME(init_fn), drv_name, init_fn, pm_control_fn, \
		      NULL, NULL, level, prio, NULL)

#ifdef __cplusplus
}
#endif

#endif /* _INIT_H_ */
