/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Spacemit.
 */

#ifndef __SBI_EMULATE_LDST_H__
#define __SBI_EMULATE_LDST_H__

#include <sbi/sbi_types.h>

struct sbi_trap_regs;

union sbi_ldst_data {
	u8    data_bytes[8];
	u32   data_u32;
	u64   data_u64;
	ulong data_ulong;
};

int sbi_emulate_load_handler(ulong addr, ulong tval2, ulong tinst,
			     struct sbi_trap_regs *regs);

int sbi_emulate_store_handler(ulong addr, ulong tval2, ulong tinst,
			      struct sbi_trap_regs *regs);

#endif
