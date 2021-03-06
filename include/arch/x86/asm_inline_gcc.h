/* Intel x86 GCC specific public inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef _ASM_INLINE_GCC_PUBLIC_GCC_H
#define _ASM_INLINE_GCC_PUBLIC_GCC_H

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#include <sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <stddef.h>

/**
 *
 * @internal
 *
 * @brief Disable all interrupts on the CPU
 *
 * GCC assembly internals of irq_lock(). See irq_lock() for a complete
 * description.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 */

// KID 20170601
// https://stackoverflow.com/questions/41044421/
// is-there-a-way-to-tell-gcc-not-to-reorder-any-instructions-not-just-load-stores
static ALWAYS_INLINE unsigned int _do_irq_lock(void)
{
	unsigned int key;

	__asm__ volatile (
		"pushfl;\n\t"
		"cli;\n\t"
		"popl %0;\n\t"
		: "=g" (key)
		:
		: "memory"
		);
	// "pushfl;" : push %eflags onto stack
	// "cli;" : clear interrupt flag 
	// "popl key;"

	// key: eflags 값
	return key;
	// return eflags 값
}


/**
 *
 * @internal
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * GCC assembly internals of irq_lock_unlock(). See irq_lock_unlock() for a
 * complete description.
 *
 * @return N/A
 */

// KID 20170602
// ALWAYS_INLINE: inline __attribute__((always_inline))
static ALWAYS_INLINE void _do_irq_unlock(void)
{
	__asm__ volatile (
		"sti;\n\t"
		: : : "memory"
		);
	// Set Interrupt Flag
}


/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 *
 * @internal
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed and leverage
 * the fact that the 'bsfl' doesn't modify the destination operand when the
 * source operand is zero.  The "bitpos" variable can be preloaded into the
 * destination register, and given the unconditional ++bitpos that is performed
 * after the 'cmovzl', the correct results are yielded.
 */

// KID 20170718
// ready_range: 0x80018000
static ALWAYS_INLINE unsigned int find_lsb_set(u32_t op)
{
	unsigned int bitpos;

	// op: 0x80018000
	__asm__ volatile (

#if defined(CONFIG_CMOV) // CONFIG_CMOV=n

		"bsfl %1, %0;\n\t"
		"cmovzl %2, %0;\n\t"
		: "=r" (bitpos)
		: "rm" (op), "r" (-1)
		: "cc"

#else
                                      // op: 0x80018000
		  "bsfl %1, %0;\n\t"  // "bsfl op, bitpos;"
		  "jnz 1f;\n\t"       // "jnz 1f;"
		  "movl $-1, %0;\n\t" // "movl $-1, bitpos;"
		  "1:\n\t"            // "1:"
		: "=r" (bitpos)
		: "rm" (op)
		: "cc"

#endif /* CONFIG_CMOV */
		);
	// bitpos: 15

	// bitpos: 15
	return (bitpos + 1);
	// return 16
}


/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 *
 * @internal
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed and leverage
 * the fact that the 'bsfl' doesn't modify the destination operand when the
 * source operand is zero.  The "bitpos" variable can be preloaded into the
 * destination register, and given the unconditional ++bitpos that is performed
 * after the 'cmovzl', the correct results are yielded.
 */

// KID 20170609
static ALWAYS_INLINE unsigned int find_msb_set(u32_t op)
{
	unsigned int bitpos;

	__asm__ volatile (

#if defined(CONFIG_CMOV) // CONFIG_CMOV=n

		"bsrl %1, %0;\n\t"
		"cmovzl %2, %0;\n\t"
		: "=r" (bitpos)
		: "rm" (op), "r" (-1)

#else

		  "bsrl %1, %0;\n\t"
		  "jnz 1f;\n\t"
		  "movl $-1, %0;\n\t"
		  "1:\n\t"
		: "=r" (bitpos)
		: "rm" (op)
		: "cc"

#endif /* CONFIG_CMOV */
		);

	return (bitpos + 1);
}


/**
 *  @brief read timestamp register ensuring serialization
 */

static inline u64_t _tsc_read(void)
{
	union {
		struct  {
			u32_t lo;
			u32_t hi;
		};
		u64_t  value;
	}  rv;

	/* rdtsc & cpuid clobbers eax, ebx, ecx and edx registers */
	__asm__ volatile (/* serialize */
		"xorl %%eax,%%eax;\n\t"
		"cpuid;\n\t"
		:
		:
		: "%eax", "%ebx", "%ecx", "%edx"
		);
	/*
	 * We cannot use "=A", since this would use %rax on x86_64 and
	 * return only the lower 32bits of the TSC
	 */
	__asm__ volatile ("rdtsc" : "=a" (rv.lo), "=d" (rv.hi));


	return rv.value;
}


/**
 *
 * @brief Get a 32 bit CPU timestamp counter
 *
 * @return a 32-bit number
 */

static ALWAYS_INLINE
	u32_t _do_read_cpu_timestamp32(void)
{
	u32_t rv;

	__asm__ volatile("rdtsc" : "=a"(rv) :  : "%edx");

	return rv;
}


/* Implementation of sys_io.h's documented functions */

static ALWAYS_INLINE
void sys_out8(u8_t data, io_port_t port)
{
	__asm__ volatile("outb	%b0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static ALWAYS_INLINE
	u8_t sys_in8(io_port_t port)
{
	u8_t ret;

	__asm__ volatile("inb	%w1, %b0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static ALWAYS_INLINE
	void sys_out16(u16_t data, io_port_t port)
{
	__asm__ volatile("outw	%w0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static ALWAYS_INLINE
	u16_t sys_in16(io_port_t port)
{
	u16_t ret;

	__asm__ volatile("inw	%w1, %w0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static ALWAYS_INLINE
	void sys_out32(u32_t data, io_port_t port)
{
	__asm__ volatile("outl	%0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static ALWAYS_INLINE
	u32_t sys_in32(io_port_t port)
{
	u32_t ret;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static ALWAYS_INLINE
	void sys_io_set_bit(io_port_t port, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 "btsl	%2, %0;\n\t"
			 "outl	%0, %w1;\n\t"
			 :
			 : "a" (reg), "Nd" (port), "Ir" (bit));
}

static ALWAYS_INLINE
	void sys_io_clear_bit(io_port_t port, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 "btrl	%2, %0;\n\t"
			 "outl	%0, %w1;\n\t"
			 :
			 : "a" (reg), "Nd" (port), "Ir" (bit));
}

static ALWAYS_INLINE
	int sys_io_test_bit(io_port_t port, unsigned int bit)
{
	u32_t ret;

	__asm__ volatile("inl	%w1, %0\n\t"
			 "btl	%2, %0\n\t"
			 : "=a" (ret)
			 : "Nd" (port), "Ir" (bit));

	return (ret & 1);
}

static ALWAYS_INLINE
	int sys_io_test_and_set_bit(io_port_t port, unsigned int bit)
{
	int ret;

	ret = sys_io_test_bit(port, bit);
	sys_io_set_bit(port, bit);

	return ret;
}

static ALWAYS_INLINE
	int sys_io_test_and_clear_bit(io_port_t port, unsigned int bit)
{
	int ret;

	ret = sys_io_test_bit(port, bit);
	sys_io_clear_bit(port, bit);

	return ret;
}

static ALWAYS_INLINE
	void sys_write8(u8_t data, mm_reg_t addr)
{
	__asm__ volatile("movb	%0, %1;\n\t"
			 :
			 : "q"(data), "m" (*(volatile u8_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u8_t sys_read8(mm_reg_t addr)
{
	u8_t ret;

	__asm__ volatile("movb	%1, %0;\n\t"
			 : "=q"(ret)
			 : "m" (*(volatile u8_t *) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE
	void sys_write16(u16_t data, mm_reg_t addr)
{
	__asm__ volatile("movw	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile u16_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u16_t sys_read16(mm_reg_t addr)
{
	u16_t ret;

	__asm__ volatile("movw	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile u16_t *) addr)
			 : "memory");

	return ret;
}

// KID 20170607
// KID 20170727
// mux_config[0]: 0x10064555, PINMUX_SELECT_REGISTER(0xb0800900, 0): (0xb0800900 + 0x30 + (0 << 2))
static ALWAYS_INLINE
	void sys_write32(u32_t data, mm_reg_t addr)
{
	__asm__ volatile("movl	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile u32_t *) addr)
			 : "memory");
	// movl	0x10064555, (*(volatile u32_t *) 0xb0800930);
}

// KID 20170607
// 0xFEE000F0
static ALWAYS_INLINE
	u32_t sys_read32(mm_reg_t addr)
{
	u32_t ret;

	// addr: 0xFEE000F0
	__asm__ volatile("movl	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile u32_t *) addr)
			 : "memory");

	// ret: 0xFF

	// ret: 0xFF
	return ret;
	// return 0xFF
}


static ALWAYS_INLINE
	void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	__asm__ volatile("btsl	%1, %0;\n\t"
			 : "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit)
			 : "memory");
}

static ALWAYS_INLINE
	void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	__asm__ volatile("btrl	%1, %0;\n\t"
			 : "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));
}

static ALWAYS_INLINE
	int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	__asm__ volatile("btl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static ALWAYS_INLINE
	int sys_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	__asm__ volatile("btsl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static ALWAYS_INLINE
	int sys_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	__asm__ volatile("btrl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

#define sys_bitfield_set_bit sys_set_bit
#define sys_bitfield_clear_bit sys_clear_bit
#define sys_bitfield_test_bit sys_test_bit
#define sys_bitfield_test_and_set_bit sys_test_and_set_bit
#define sys_bitfield_test_and_clear_bit sys_test_and_clear_bit

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */
