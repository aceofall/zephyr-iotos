/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "util/util.h"

#include "pdu.h"
#include "ctrl.h"
#include "ll.h"

static struct {
	u16_t interval;
	u16_t window;
	u8_t  type:1;
	u8_t  tx_addr:1;
	u8_t  filter_policy:1;
} ll_scan;

u32_t ll_scan_params_set(u8_t type, u16_t interval, u16_t window,
			 u8_t own_addr_type, u8_t filter_policy)
{
	if (radio_scan_is_enabled()) {
		return 0x0C; /* Command Disallowed */
	}

	ll_scan.type = type;
	ll_scan.interval = interval;
	ll_scan.window = window;
	ll_scan.tx_addr = own_addr_type;
	ll_scan.filter_policy = filter_policy;

	return 0;
}

u32_t ll_scan_enable(u8_t enable)
{
	u32_t status;

	if (!enable) {
		status = radio_scan_disable();

		return status;
	}

	status = radio_scan_enable(ll_scan.type, ll_scan.tx_addr,
				   ll_addr_get(ll_scan.tx_addr, NULL),
				   ll_scan.interval, ll_scan.window,
				   ll_scan.filter_policy);

	return status;
}
