/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef _kernel_arch_func__h_
#define _kernel_arch_func__h_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* stack alignment related macros: STACK_ALIGN_SIZE is defined above */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
// KID 20170522
// STACK_ALIGN_SIZE: 4
// _main_stack + 1024
// ARM10C 20170525
// _idle_stack + 1024
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the kernel.
 * Trivial stuff is done inline; more complex initialization is done via
 * function calls.
 *
 * @return N/A
 */
// KID 20170527
static inline void kernel_arch_init(void)
{
	// CONFIG_ISR_STACK_SIZE: 2048
	extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];

	_kernel.nested = 0;
	// _kernel.nested: 0

	// CONFIG_ISR_STACK_SIZE: 2048
	_kernel.irq_stack = _interrupt_stack + CONFIG_ISR_STACK_SIZE;
	// _kernel.irq_stack: _interrupt_stack + 2048
}

/**
 *
 * @brief Set the return value for the specified fiber (inline)
 *
 * @param fiber pointer to fiber
 * @param value value to set as return value
 *
 * The register used to store the return value from a function call invocation
 * is set to <value>.  It is assumed that the specified <fiber> is pending, and
 * thus the fibers context is stored in its TCS.
 *
 * @return N/A
 */
// KID 20170625
// KID 20170712
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	/* write into 'eax' slot created in _Swap() entry */

	*(unsigned int *)(thread->callee_saved.esp) = value;
}

extern void k_cpu_atomic_idle(unsigned int imask);

extern void _MsrWrite(unsigned int msr, u64_t msrData);
extern u64_t _MsrRead(unsigned int msr);

/*
 * _IntLibInit() is called from the non-arch specific function,
 * prepare_multithreading(). The IA-32 kernel does not require any special
 * initialization of the interrupt subsystem. However, we still need to
 * provide an _IntLibInit() of some sort to prevent build errors.
 */
// KID 20170519
static inline void _IntLibInit(void)
{
}

/* the _idt_base_address symbol is generated via a linker script */
// KID 20170517
extern unsigned char _idt_base_address[];

#include <stddef.h> /* For size_t */

#ifdef __cplusplus
}
#endif

#define _is_in_isr() (_kernel.nested != 0)

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_func__h_ */
