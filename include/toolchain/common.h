/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common toolchain abstraction
 *
 * Macros to abstract compiler capabilities (common to all toolchains).
 */

/* Abstract use of extern keyword for compatibility between C and C++ */
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

/* Use TASK_ENTRY_CPP to tag task entry points defined in C++ files. */

#ifdef __cplusplus
#define TASK_ENTRY_CPP  extern "C"
#endif

/*
 * Generate a reference to an external symbol.
 * The reference indicates to the linker that the symbol is required
 * by the module containing the reference and should be included
 * in the image if the module is in the image.
 *
 * The assembler directive ".set" is used to define a local symbol.
 * No memory is allocated, and the local symbol does not appear in
 * the symbol table.
 */

#ifdef _ASMLANGUAGE
  #define REQUIRES(sym) .set sym ## _Requires, sym
#else
  #define REQUIRES(sym) __asm__ (".set " # sym "_Requires, " # sym "\n\t");
#endif

#ifdef _ASMLANGUAGE
  #define SECTION .section
#endif

/*
 * If the project is being built for speed (i.e. not for minimum size) then
 * align functions and branches in executable sections to improve performance.
 */

#ifdef _ASMLANGUAGE

  #ifdef CONFIG_X86 // CONFIG_X86=y

    #ifdef PERF_OPT
      #define PERFOPT_ALIGN .balign 16
    #else
// KID 20170516
// KID 20170620
      #define PERFOPT_ALIGN .balign  1
    #endif

  #elif defined(CONFIG_ARM)

    #ifdef CONFIG_ISA_THUMB
      #define PERFOPT_ALIGN .balign  2
    #else
      #define PERFOPT_ALIGN .balign  4
    #endif

  #elif defined(CONFIG_ARC)

    #define PERFOPT_ALIGN .balign  4

  #elif defined(CONFIG_NIOS2) || defined(CONFIG_RISCV32) || \
	  defined(CONFIG_XTENSA)
    #define PERFOPT_ALIGN .balign 4

  #elif defined(CONFIG_ARCH_POSIX)

  #else

    #error Architecture unsupported

  #endif

  #define GC_SECTION(sym) SECTION .text.##sym, "ax"

#endif /* _ASMLANGUAGE */

/* force inlining a function */

#if !defined(_ASMLANGUAGE) // undefined 
// KID 20170601
// KID 20170602
// ALWAYS_INLINE: inline __attribute__((always_inline))
  #define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

// KID 20170519
// KID 20170530
// _STRINGIFY(30): "30"
#define _STRINGIFY(x) #x
// KID 20170530
// _STRINGIFY(30): "30"
//
// STRINGIFY(30): "30"
#define STRINGIFY(s) _STRINGIFY(s)

/* Indicate that an array will be used for stack space. */

#if !defined(_ASMLANGUAGE)
  #define __stack __aligned(STACK_ALIGN)
// KID 20170517
// KID 20170519
// STACK_ALIGN: 4
// __stack: __aligned(4)
  /* don't use this anymore, use K_DECLARE_STACK instead. Remove for 1.11 */
  #define __stack __aligned(STACK_ALIGN) __DEPRECATED_MACRO
#endif

/* concatenate the values of the arguments into one */
// KID 20170530
// _DO_CONCAT(sys_init_, init_static_pools): sys_init_init_static_pools
// KID 20170530
// _DO_CONCAT(sys_init_init_static_pools, __COUNTER__): sys_init_init_static_pools0
#define _DO_CONCAT(x, y) x ## y
// KID 20170530
// _DO_CONCAT(sys_init_, init_static_pools): sys_init_init_static_pools
//
// _CONCAT(sys_init_, init_static_pools): sys_init_init_static_pools
// KID 20170530
// _DO_CONCAT(sys_init_init_static_pools, __COUNTER__): sys_init_init_static_pools0
//
// _CONCAT(sys_init_init_static_pools, __COUNTER__): sys_init_init_static_pools0
#define _CONCAT(x, y) _DO_CONCAT(x, y)

/* Additionally used as a sentinel by gen_syscalls.py to identify what
 * functions are system calls
 *
 * Note POSIX unit tests don't still generate the system call stubs, so
 * until https://github.com/zephyrproject-rtos/zephyr/issues/5006 is
 * fixed via possibly #4174, we introduce this hack -- which will
 * disallow us to test system calls in POSIX unit testing (currently
 * not used).
 */
#ifndef ZTEST_UNITTEST
#define __syscall static inline
#else
#define __syscall
#endif /* #ifndef ZTEST_UNITTEST */

#ifndef BUILD_ASSERT
/* compile-time assertion that makes the build fail */
#define BUILD_ASSERT(EXPR) typedef char __build_assert_failure[(EXPR) ? 1 : -1]
#endif
#ifndef BUILD_ASSERT_MSG
/* build assertion with message -- common implementation swallows message. */
#define BUILD_ASSERT_MSG(EXPR, MSG) BUILD_ASSERT(EXPR)
#endif
