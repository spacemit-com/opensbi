/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Spacemit.
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_emulate_ldst.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>

static int decode_insn(ulong tinst, struct sbi_trap_regs *regs,
		       ulong *out_insn, ulong *out_insn_len)
{
	struct sbi_trap_info uptrap;

	if (tinst & 0x1) {
		*out_insn = tinst | INSN_16BIT_MASK;
		*out_insn_len = (tinst & 0x2) ? INSN_LEN(*out_insn) : 2;
	} else {
		*out_insn = sbi_get_insn(regs->mepc, &uptrap);
		if (uptrap.cause) {
			uptrap.epc = regs->mepc;
			return sbi_trap_redirect(regs, &uptrap);
		}
		*out_insn_len = INSN_LEN(*out_insn);
	}
	return 0;
}

int sbi_emulate_load_handler(ulong addr, ulong tval2, ulong tinst,
			     struct sbi_trap_regs *regs)
{
	ulong insn, insn_len;
	union sbi_ldst_data val = { 0 };
	int shift = 0, len = 0, rc;

	rc = decode_insn(tinst, regs, &insn, &insn_len);
	if (rc)
		return rc;

	if ((insn & INSN_MASK_LW) == INSN_MATCH_LW) {
		len = 4; shift = 8 * (sizeof(ulong) - len);
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_LD) == INSN_MATCH_LD) {
		len = 8; shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LWU) == INSN_MATCH_LWU) {
		len = 4;
#endif
	} else if ((insn & INSN_MASK_LH) == INSN_MATCH_LH) {
		len = 2; shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LHU) == INSN_MATCH_LHU) {
		len = 2;
	} else if ((insn & INSN_MASK_LB) == INSN_MATCH_LB) {
		len = 1; shift = 8 * (sizeof(ulong) - len);
	} else if ((insn & INSN_MASK_LBU) == INSN_MATCH_LBU) {
		len = 1;
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_C_LD) == INSN_MATCH_C_LD) {
		len = 8; shift = 8 * (sizeof(ulong) - len);
		insn = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_LDSP) == INSN_MATCH_C_LDSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 8; shift = 8 * (sizeof(ulong) - len);
#endif
	} else if ((insn & INSN_MASK_C_LW) == INSN_MATCH_C_LW) {
		len = 4; shift = 8 * (sizeof(ulong) - len);
		insn = RVC_RS2S(insn) << SH_RD;
	} else if ((insn & INSN_MASK_C_LWSP) == INSN_MATCH_C_LWSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 4; shift = 8 * (sizeof(ulong) - len);
	} else {
		return SBI_ENODEV;
	}

	rc = sbi_platform_emulate_load(sbi_platform_thishart_ptr(),
				       len, addr, &val);
	if (rc)
		return rc;

	SET_RD(insn, regs, ((long)(val.data_ulong << shift)) >> shift);
	regs->mepc += insn_len;
	return 0;
}

int sbi_emulate_store_handler(ulong addr, ulong tval2, ulong tinst,
			      struct sbi_trap_regs *regs)
{
	ulong insn, insn_len;
	union sbi_ldst_data val;
	int len = 0, rc;

	rc = decode_insn(tinst, regs, &insn, &insn_len);
	if (rc)
		return rc;

	val.data_ulong = GET_RS2(insn, regs);

	if ((insn & INSN_MASK_SW) == INSN_MATCH_SW) {
		len = 4;
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_SD) == INSN_MATCH_SD) {
		len = 8;
#endif
	} else if ((insn & INSN_MASK_SH) == INSN_MATCH_SH) {
		len = 2;
	} else if ((insn & INSN_MASK_SB) == INSN_MATCH_SB) {
		len = 1;
#if __riscv_xlen == 64
	} else if ((insn & INSN_MASK_C_SD) == INSN_MATCH_C_SD) {
		len = 8; val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SDSP) == INSN_MATCH_C_SDSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 8; val.data_ulong = GET_RS2C(insn, regs);
#endif
	} else if ((insn & INSN_MASK_C_SW) == INSN_MATCH_C_SW) {
		len = 4; val.data_ulong = GET_RS2S(insn, regs);
	} else if ((insn & INSN_MASK_C_SWSP) == INSN_MATCH_C_SWSP &&
		   ((insn >> SH_RD) & 0x1f)) {
		len = 4; val.data_ulong = GET_RS2C(insn, regs);
	} else {
		return SBI_ENODEV;
	}

	rc = sbi_platform_emulate_store(sbi_platform_thishart_ptr(),
					len, addr, val);
	if (rc)
		return rc;

	regs->mepc += insn_len;
	return 0;
}
