/* pinmux_quark_mcu.h - pinmux operation for generic Quark MCU boards */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// KID 20170727
// PINMUX_PULLUP_OFFSET: 0x00
#define PINMUX_PULLUP_OFFSET   0x00
#define PINMUX_SLEW_OFFSET     0x10
#define PINMUX_INPUT_OFFSET    0x20
// KID 20170727
// PINMUX_SELECT_OFFSET: 0x30
#define PINMUX_SELECT_OFFSET   0x30

// KID 20170727
// base: 0xb0800900, 0
// PINMUX_SELECT_OFFSET: 0x30
//
// #define PINMUX_SELECT_REGISTER(0xb0800900, 0):
// (0xb0800900 + 0x30 + (0 << 2))
#define PINMUX_SELECT_REGISTER(base, reg_offset) \
	(base + PINMUX_SELECT_OFFSET + (reg_offset << 2))

/*
 * A little decyphering of what is going on here:
 *
 * Each pinmux register rperesents a bank of 16 pins, 2 bits per pin for a total
 * of four possible settings per pin.
 *
 * The first argument to the macro is name of the u32_t's that is being used
 * to contain the bit patterns for all the configuration registers.  The pin
 * number divided by 16 selects the correct register bank based on the pin
 * number.
 *
 * The pin number % 16 * 2 selects the position within the register bank for the
 * bits controlling the pin.
 *
 * All but the lower two bits of the config values are masked off to ensure
 * that we don't inadvertently affect other pins in the register bank.
 */
// KID 20170727
// PINMUX_FUNC_B: 1
//
// PIN_CONFIG(mux_config, 0, 1):
// (mux_config[((0) / 16)] |= ((0x3 & (1)) << (((0) % 16) * 2)))
#define PIN_CONFIG(A, _pin, _func) \
	(A[((_pin) / 16)] |= ((0x3 & (_func)) << (((_pin) % 16) * 2)))

// KID 20170727
// base_address: 0xb0800900, 104, PINMUX_PULLUP_ENABLE: 0x1
static inline int _quark_mcu_set_mux(u32_t base, u32_t pin, u8_t func)
{
	/*
	 * the registers are 32-bit wide, but each pin requires 1 bit
	 * to set the input enable bit.
	 */
	// pin:104
	u32_t register_offset = (pin / 32) * 4;
	// register_offset: 12

	/*
	 * Now figure out what is the full address for the register
	 * we are looking for.  Add the base register to the register_mask
	 */
	// base: 0xb0800900, register_offset: 12
	volatile u32_t *mux_register = (u32_t *)(base + register_offset);
	// mux_register: 0xb0800948

	/*
	 * Finally grab the pin offset within the register
	 */
	// pin:104
	u32_t pin_offset = pin % 32;
	// pin_offset: 8

	/*
	 * MAGIC NUMBER: 0x1 is used as the pullup is a single bit in a
	 * 32-bit register.
	 */
	// mux_register: 0xb0800948 pin_offset: 8, func: 0x1
	(*(mux_register)) = ((*(mux_register)) & ~(0x1 << pin_offset)) |
		((func & 0x01) << pin_offset);
	// *(0xb0800948):  0x100

	return 0;
	// return 0
}
