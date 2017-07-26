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
// KID 20170726
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

	// k_work_q_start 에서 한일:
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

	return 0;
	// return 0
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
