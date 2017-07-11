/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>

#ifndef _kernel_offsets__h_
#define _kernel_offsets__h_

/*
 * The final link step uses the symbol _OffsetAbsSyms to force the linkage of
 * offsets.o into the ELF image.
 */

GEN_ABS_SYM_BEGIN(_OffsetAbsSyms)

// KID 20170711
// GEN_OFFSET_SYM(_kernel_t, current):
// __asm__(".globl\t" ___kernel_t_current_OFFSET "\n\t.equ\t" ___kernel_t_current_OFFSET
// 	   ",%c0"
// 	   "\n\t.type\t" ___kernel_t_current_OFFSET ",@object" :  : "n"(((size_t) &((_kernel_t *)0)->current)))
GEN_OFFSET_SYM(_kernel_t, current);

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(_kernel_t, threads);
#endif

GEN_OFFSET_SYM(_kernel_t, nested);
GEN_OFFSET_SYM(_kernel_t, irq_stack);
#ifdef CONFIG_SYS_POWER_MANAGEMENT
GEN_OFFSET_SYM(_kernel_t, idle);
#endif

// KID 20170711
// GEN_OFFSET_SYM(_kernel_t, ready_q):
// __asm__(".globl\t" ___kernel_t_ready_q_OFFSET "\n\t.equ\t" ___kernel_t_ready_q_OFFSET
// 	   ",%c0"
// 	   "\n\t.type\t" ___kernel_t_ready_q_OFFSET ",@object" :  : "n"(((size_t) &((_kernel_t *)0)->ready_q)))
GEN_OFFSET_SYM(_kernel_t, ready_q);
GEN_OFFSET_SYM(_kernel_t, arch);

// KID 20170711
// GEN_OFFSET_SYM(_ready_q_t, ready_q):
// __asm__(".globl\t" ___ready_q_t_cache_OFFSET "\n\t.equ\t" ___ready_q_t_cache_OFFSET
// 	   ",%c0"
// 	   "\n\t.type\t" ___ready_q_t_cache_OFFSET ",@object" :  : "n"(((size_t) &((_ready_q_t *)0)->ready_q)))
GEN_OFFSET_SYM(_ready_q_t, cache);

#ifdef CONFIG_FP_SHARING
GEN_OFFSET_SYM(_kernel_t, current_fp);
#endif

// KID 20170518
// sizeof(struct _kernel): 284 bytes
// GEN_ABSOLUTE_SYM(_STRUCT_KERNEL_SIZE, 284):
// __asm__(".globl\t" "_STRUCT_KERNEL_SIZE" "\n\t.equ\t" "_STRUCT_KERNEL_SIZE" ",%c0"
//         "\n\t.type\t" "_STRUCT_KERNEL_SIZE" ",@object" :  : "n"(284))
GEN_ABSOLUTE_SYM(_STRUCT_KERNEL_SIZE, sizeof(struct _kernel));

GEN_OFFSET_SYM(_thread_base_t, user_options);
GEN_OFFSET_SYM(_thread_base_t, thread_state);
GEN_OFFSET_SYM(_thread_base_t, prio);
GEN_OFFSET_SYM(_thread_base_t, sched_locked);
GEN_OFFSET_SYM(_thread_base_t, preempt);
GEN_OFFSET_SYM(_thread_base_t, swap_data);

GEN_OFFSET_SYM(_thread_t, base);
GEN_OFFSET_SYM(_thread_t, caller_saved);

// KID 20170711
// GEN_OFFSET_SYM(_thread_t, callee_saved):
// __asm__(".globl\t" ___thread_t_callee_saved_OFFSET "\n\t.equ\t" ___thread_t_callee_saved_OFFSET
// 	   ",%c0"
// 	   "\n\t.type\t" ___thread_t_callee_saved_OFFSET ",@object" :  : "n"(((size_t) &((_thread_t *)0)->callee_saved)))
GEN_OFFSET_SYM(_thread_t, callee_saved);
GEN_OFFSET_SYM(_thread_t, arch);

#ifdef CONFIG_THREAD_STACK_INFO
GEN_OFFSET_SYM(_thread_stack_info_t, start);
GEN_OFFSET_SYM(_thread_stack_info_t, size);

GEN_OFFSET_SYM(_thread_t, stack_info);
#endif

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(_thread_t, next_thread);
#endif

#ifdef CONFIG_THREAD_CUSTOM_DATA
GEN_OFFSET_SYM(_thread_t, custom_data);
#endif

GEN_ABSOLUTE_SYM(K_THREAD_SIZEOF, sizeof(struct k_thread));

/* size of the device structure. Used by linker scripts */
GEN_ABSOLUTE_SYM(_DEVICE_STRUCT_SIZE, sizeof(struct device));

#endif /* _kernel_offsets__h_ */
