/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Spacemit.
 */

#ifndef _K1PRO_PLATFORM_H_
#define _K1PRO_PLATFORM_H_

#define CSR_MCOR        0x7c2
#define CSR_MHCR        0x7c1
#define CSR_MCCR2       0x7c3
#define CSR_MHINT       0x7c5
#define CSR_MXSTATUS    0x7c0
#define CSR_PLIC_BASE   0xfc1
#define CSR_MRMR        0x7c6
#define CSR_MRVBR       0x7c7
#define CSR_MSETUP      0x7C0
#define CSR_MCPM        0x7C1
#define CSR_MPCTL       0x7D0
#define CSR_ML2SETUP    0x7F0

#define PLATFORM_CCI_ADDR           (0x0FE00000)
#define CPU_RESET_BASE_ADDR         (0x2F024000)

#define X60_PLIC_CLINT_OFFSET       0x04000000  /* 64M */
#define X60_PLIC_DELEG_OFFSET       0x001ffffc
#define X60_PLIC_DELEG_ENABLE       0x1

#define CLUSTER_ID_BITSHIFT         (2)
#define CLUSTER_ID_MASK             (0x0f << CLUSTER_ID_BITSHIFT)
#define CORE_ID_BITSHIFT            (0)
#define CORE_ID_MASK                (((1 << CLUSTER_ID_BITSHIFT) - 1) << CORE_ID_BITSHIFT)

#define PLAT_CCI_CLUSTER0_IFACE_IX  0
#define PLAT_CCI_CLUSTER1_IFACE_IX  1
#define PLAT_CCI_CLUSTER2_IFACE_IX  2
#define PLAT_CCI_CLUSTER3_IFACE_IX  3


struct x60_regs_struct {
    u64 pmpaddr0;
    u64 pmpaddr1;
    u64 pmpaddr2;
    u64 pmpaddr3;
    u64 pmpaddr4;
    u64 pmpaddr5;
    u64 pmpaddr6;
    u64 pmpaddr7;
    u64 pmpcfg0;
    u64 msetup;
    u64 mcpm;
    u64 plic_base_addr;
    u64 clint_base_addr;
};

#endif /* _K1PRO_PLATFORM_H_ */
