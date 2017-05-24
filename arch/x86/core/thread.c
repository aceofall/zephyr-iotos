/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread support primitives
 *
 * This module provides core thread related primitives for the IA-32
 * processor architecture.
 */

#ifdef CONFIG_INIT_STACKS // CONFIG_INIT_STACKS=n
#include <string.h>
#endif /* CONFIG_INIT_STACKS */

#include <toolchain.h>
#include <sections.h>
#include <kernel_structs.h>
#include <wait_q.h>

/* forward declaration */

#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU) // CONFIG_GDB_INFO=n, CONFIG_DEBUG_INFO=n, CONFIG_X86_IAMCU=y
void _thread_entry_wrapper(_thread_entry_t, void *,
			   void *, void *);
#endif

/**
 *
 * @brief Initialize a new execution thread
 *
 * This function is utilized to initialize all execution threads (both fiber
 * and task).  The 'priority' parameter will be set to -1 for the creation of
 * task.
 *
 * This function is called by _new_thread() to initialize tasks.
 *
 * @param thread pointer to thread struct memory
 * @param pStackMem pointer to thread stack memory
 * @param stackSize size of a stack in bytes
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL, K_FP_REGS, K_SSE_REGS
 *
 * @return N/A
 */
// KID 20170522
// pStackMem: _main_stack, stackSize: 1024, priority: 0, options: 0x1, thread: &_main_thread_s
static void _new_thread_internal(char *pStackMem, unsigned int stackSize,
				 int priority,
				 unsigned int options,
				 struct k_thread *thread)
{
	unsigned long *pInitialCtx;

#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO)) // CONFIG_FP_SHARING=n, CONFIG_GDB_INFO=n
	thread->arch.excNestCount = 0;
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

	/*
	 * The creation of the initial stack for the task has already been done.
	 * Now all that is needed is to set the ESP. However, we have been passed
	 * the base address of the stack which is past the initial stack frame.
	 * Therefore some of the calculations done in the other routines that
	 * initialize the stack frame need to be repeated.
	 */

	// pStackMem: _main_stack, stackSize: 1024
	// STACK_ROUND_DOWN(_main_stack + 1024): _main_stack + 1024
	pInitialCtx = (unsigned long *)STACK_ROUND_DOWN(pStackMem + stackSize);
	// pInitialCtx: _main_stack + 1024

#ifdef CONFIG_THREAD_MONITOR // CONFIG_THREAD_MONITOR=n
	/*
	 * In debug mode thread->entry give direct access to the thread entry
	 * and the corresponding parameters.
	 */
	thread->entry = (struct __thread_entry *)(pInitialCtx -
		sizeof(struct __thread_entry));
#endif

	/* The stack needs to be set up so that when we do an initial switch
	 * to it in the middle of _Swap(), it needs to be set up as follows:
	 *  - 4 thread entry routine parameters
	 *  - eflags
	 *  - eip (so that _Swap() "returns" to the entry point)
	 *  - edi, esi, ebx, ebp,  eax
	 */
	// pInitialCtx: _main_stack + 1024
	pInitialCtx -= 11;
	// pInitialCtx: _main_stack + 980

	// thread->callee_saved.esp: (&_main_thread_s)->callee_saved.esp, pInitialCtx: _main_stack + 980
	thread->callee_saved.esp = (unsigned long)pInitialCtx;
	// thread->callee_saved.esp: (&_main_thread_s)->callee_saved.esp: _main_stack + 980

	// thread->coopReg.esp: (&_main_thread_s)->coopReg.esp: ????
	PRINTK("\nInitial context ESP = 0x%x\n", thread->coopReg.esp); // null function

	// thread: &_main_thread_s
	PRINTK("\nstruct thread * = 0x%x", thread); // null function

	// thread: &_main_thread_s
	thread_monitor_init(thread); // null function
}

#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU)
/**
 *
 * @brief Adjust stack/parameters before invoking _thread_entry
 *
 * This function adjusts the initial stack frame created by _new_thread() such
 * that the GDB stack frame unwinders recognize it as the outermost frame in
 * the thread's stack.  For targets that use the IAMCU calling convention, the
 * first three arguments are popped into eax, edx, and ecx. The function then
 * jumps to _thread_entry().
 *
 * GDB normally stops unwinding a stack when it detects that it has
 * reached a function called main().  Kernel tasks, however, do not have
 * a main() function, and there does not appear to be a simple way of stopping
 * the unwinding of the stack.
 *
 * SYS V Systems:
 *
 * Given the initial thread created by _new_thread(), GDB expects to find a
 * return address on the stack immediately above the thread entry routine
 * _thread_entry, in the location occupied by the initial EFLAGS.
 * GDB attempts to examine the memory at this return address, which typically
 * results in an invalid access to page 0 of memory.
 *
 * This function overwrites the initial EFLAGS with zero.  When GDB subsequently
 * attempts to examine memory at address zero, the PeekPoke driver detects
 * an invalid access to address zero and returns an error, which causes the
 * GDB stack unwinder to stop somewhat gracefully.
 *
 * The initial EFLAGS cannot be overwritten until after _Swap() has swapped in
 * the new thread for the first time.  This routine is called by _Swap() the
 * first time that the new thread is swapped in, and it jumps to
 * _thread_entry after it has done its work.
 *
 * IAMCU Systems:
 *
 * There is no EFLAGS on the stack when we get here. _thread_entry() takes
 * four arguments, and we need to pop off the first three into the
 * appropriate registers. Instead of using the 'call' instruction, we push
 * a NULL return address onto the stack and jump into _thread_entry,
 * ensuring the stack won't be unwound further. Placing some kind of return
 * address on the stack is mandatory so this isn't conditionally compiled.
 *
 *       __________________
 *      |      param3      |   <------ Top of the stack
 *      |__________________|
 *      |      param2      |           Stack Grows Down
 *      |__________________|                  |
 *      |      param1      |                  V
 *      |__________________|
 *      |      pEntry      |  <----   ESP when invoked by _Swap() on IAMCU
 *      |__________________|
 *      | initial EFLAGS   |  <----   ESP when invoked by _Swap() on Sys V
 *      |__________________|             (Zeroed by this routine on Sys V)
 *
 *
 *
 * @return this routine does NOT return.
 */
// KID 20170522
__asm__("\t.globl _thread_entry\n"
	"\t.section .text\n"
	"_thread_entry_wrapper:\n" /* should place this func .S file and use
				    * SECTION_FUNC
				    */

#ifdef CONFIG_X86_IAMCU // CONFIG_X86_IAMCU=y
	/* IAMCU calling convention has first 3 arguments supplied in
	 * registers not the stack
	 */
	"\tpopl %eax\n"
	"\tpopl %edx\n"
	"\tpopl %ecx\n"
	"\tpushl $0\n" /* Null return address */
#elif defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) // CONFIG_GDB_INFO=n, CONFIG_DEBUG_INFO=n
	"\tmovl $0, (%esp)\n" /* zero initialEFLAGS location */
#endif
	"\tjmp _thread_entry\n");
#endif /* CONFIG_GDB_INFO || CONFIG_DEBUG_INFO) || CONFIG_X86_IAMCU */

/**
 *
 * @brief Create a new kernel execution thread
 *
 * This function is utilized to create execution threads for both fiber
 * threads and kernel tasks.
 *
 * @param thread pointer to thread struct memory, including any space needed
 *		for extra coprocessor context
 * @param pStackmem the pointer to aligned stack memory
 * @param stackSize the stack size in bytes
 * @param pEntry thread entry point routine
 * @param parameter1 first param to entry point
 * @param parameter2 second param to entry point
 * @param parameter3 third param to entry point
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL, K_FP_REGS, K_SSE_REGS
 *
 *
 * @return opaque pointer to initialized k_thread structure
 */
// KID 20170519
// _main_thread: &_main_thread_s, _main_stack, MAIN_STACK_SIZE: 1024,
// _main, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY: 0, K_ESSENTIAL: 0x1
void _new_thread(struct k_thread *thread, char *pStackMem, size_t stackSize,
		 _thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned int options)
{
	// priority: 0, pEntry: _main
	// _is_idle_thread(_main): 0, _is_prio_higher_or_equal((0), 14): 1, _is_prio_lower_or_equal((0), -16): 1
	//
	// _ASSERT_VALID_PRIO(0, _main):
	// __ASSERT(((0) == 15 && _is_idle_thread(_main)) || (_is_prio_higher_or_equal((0), 14) && _is_prio_lower_or_equal((0), -16)),
	//          "invalid priority (%d); allowed range: %d to %d", (0), 14, -16); // null function
	_ASSERT_VALID_PRIO(priority, pEntry);

	unsigned long *pInitialThread;

	// thread: &_main_thread_s, pStackMem: _main_stack, stackSize: 1024, priority: 0, options: 0x1
	_new_thread_init(thread, pStackMem, stackSize, priority, options);

	// _new_thread_init 에서 한일:
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

	/* carve the thread entry struct from the "base" of the stack */

	// pStackMem: _main_stack, stackSize: 1024, STACK_ROUND_DOWN(_main_stack + 1024): _main_stack + 1024
	pInitialThread =
		(unsigned long *)STACK_ROUND_DOWN(pStackMem + stackSize);
	// pInitialThread: (unsigned long *)(_main_stack + 1024)

	/*
	 * Create an initial context on the stack expected by the _Swap()
	 * primitive.
	 */

	/* push arguments required by _thread_entry() */

	// *pInitialThread: *(unsigned long *)(_main_stack + 1024), parameter3: NULL
	*--pInitialThread = (unsigned long)parameter3;
	// *pInitialThread: *(unsigned long *)(_main_stack + 1024): NULL

	// *pInitialThread: *(unsigned long *)(_main_stack + 1020), parameter2: NULL
	*--pInitialThread = (unsigned long)parameter2;
	// *pInitialThread: *(unsigned long *)(_main_stack + 1020): NULL

	// *pInitialThread: *(unsigned long *)(_main_stack + 1016), parameter1: NULL
	*--pInitialThread = (unsigned long)parameter1;
	// *pInitialThread: *(unsigned long *)(_main_stack + 1016): NULL

	// *pInitialThread: *(unsigned long *)(_main_stack + 1016), pEntry: _main
	*--pInitialThread = (unsigned long)pEntry;
	// *pInitialThread: *(unsigned long *)(_main_stack + 1016): _main

	/* push initial EFLAGS; only modify IF and IOPL bits */

	// *pInitialThread: *(unsigned long *)(_main_stack + 1012),
	// EflagsGet(): eflag 레지스터 값, EFLAGS_MASK: 0x00003200, EFLAGS_INITIAL: 0x00000200
	*--pInitialThread = (EflagsGet() & ~EFLAGS_MASK) | EFLAGS_INITIAL;
	// *pInitialThread: *(unsigned long *)(_main_stack + 1012): eflag 레지스터 값 | 0x00000200

#if defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) \
	|| defined(CONFIG_X86_IAMCU) // CONFIG_GDB_INFO=n, CONFIG_DEBUG_INFO=n, CONFIG_X86_IAMCU=y
	/*
	 * Arrange for the _thread_entry_wrapper() function to be called
	 * to adjust the stack before _thread_entry() is invoked.
	 */

	// *pInitialThread: *(unsigned long *)(_main_stack + 1008)
	*--pInitialThread = (unsigned long)_thread_entry_wrapper;
	// *pInitialThread: *(unsigned long *)(_main_stack + 1008): _thread_entry_wrapper

#else /* defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) */

	*--pInitialThread = (unsigned long)_thread_entry;

#endif /* defined(CONFIG_GDB_INFO) || defined(CONFIG_DEBUG_INFO) */

	/*
	 * note: stack area for edi, esi, ebx, ebp, and eax registers can be
	 * left
	 * uninitialized, since _thread_entry() doesn't care about the values
	 * of these registers when it begins execution
	 */

	/*
	 * The k_thread structure is located at the "low end" of memory set
	 * aside for the thread's stack.
	 */

	// pStackMem: _main_stack, stackSize: 1024, priority: 0, options: 0x1, thread: &_main_thread_s
	_new_thread_internal(pStackMem, stackSize, priority, options, thread);

	// _new_thread_internal 에서 한일:
	// (&_main_thread_s)->callee_saved.esp: _main_stack + 980
}
