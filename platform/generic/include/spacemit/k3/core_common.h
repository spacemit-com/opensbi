/*
 * SPDX-License-Identifier: BSD-2-Clause
 */
#ifndef __K3_CORE_COMMON_H__
#define __K3_CORE_COMMON_H__

#define CSR_MSETUP	0x7c0
#define CSR_MHCR	0x7c1
#define CSR_MHINT	0x7c5
#define CSR_ML2SETUP	0x7F0

/*When the instruction fetch unit misses in the L2 Cache, the L2 Cache loads the cache line where the missed address is located. */
#define IPRF		(_UL(1) << 16)
/*Enable the TLB prefetch function of L2Cache */
#define TPRF		(_UL(1) << 18)

#define CSR_ML2HINT 0x7F7

/* [1]Disbale read and prefetch transaction merge */
#define CIU_MER_DIS		(_UL(1) << 2)

/* [3]Disable full address dependency check */
#define CIU_DEPD_DIS		(_UL(1) << 3)

/* [4]Disable the behavior of snb discarding prefetch when reaching a certain load */
#define CIU_PRF_THROT_DIS	(_UL(1) << 4)

#define CSR_PERF_CTRL 0x7D0

/*[32] Cache level control for vector load instructions */
#define VEC_L1BYPASS		(_UL(1) << 32)

#define CSR_PREFETCH_CTRL    0x7D1

/*[11:10] Cache level control for vector load instructions */
#define L2_PERF_DIST		(_UL(3) << 10)

/*[26]  icg cell */
#define TRACE_TOP_ICGEN		(_UL(1) << 26)

#define CSR_MVMD1	0xBF9

/* [37] Disable ifu fusion type5 */
#define IFU_FUSION_TYPE5_DIS	(_UL(1) << 37)

#define CACHE_LINE_SIZE		(64)
#define CACHE_INV_ADDR_Msk	(0xffffffffffffffff << 6)

#define CSR_PMACFG0		0x7DE
#define CSR_PMACFG2		0x7DF

#define CSR_PMAADDR0		0x7E0
#define CSR_PMAADDR1		0x7E1
#define CSR_PMAADDR2		0x7E2
#define CSR_PMAADDR3		0x7E3
#define CSR_PMAADDR4		0x7E4
#define CSR_PMAADDR5		0x7E5
#define CSR_PMAADDR6		0x7E6
#define CSR_PMAADDR7		0x7E7
#define CSR_PMAADDR8		0x7E8
#define CSR_PMAADDR9		0x7E9
#define CSR_PMAADDR10		0x7EA
#define CSR_PMAADDR11		0x7EB
#define CSR_PMAADDR12		0x7EC
#define CSR_PMAADDR13		0x7ED
#define CSR_PMAADDR14		0x7EE
#define CSR_PMAADDR15		0x7EF

#endif /* __K3_CORE_COMMON_H__ */

