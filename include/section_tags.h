/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _section_tags__h_
#define _section_tags__h_

#include <toolchain.h>

#if !defined(_ASMLANGUAGE)

// KID 20170519
// KID 20170601
// NOINIT: noinit
//
// __in_section_unique(noinit):
// __attribute__((section("." "noinit" "." "_FILE_PATH_HASH" "." "__COUNTER__")))
// __noinit: __attribute__((section("." "noinit" "." "_FILE_PATH_HASH" "." "__COUNTER__")))
#define __noinit		__in_section_unique(NOINIT)
#define __irq_vector_table	_GENERIC_SECTION(IRQ_VECTOR_TABLE)
#define __sw_isr_table		_GENERIC_SECTION(SW_ISR_TABLE)

#if defined(CONFIG_ARM)
#define __kinetis_flash_config_section __in_section_unique(KINETIS_FLASH_CONFIG)
#endif /* CONFIG_ARM */

#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
