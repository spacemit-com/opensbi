/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Space-T.
 */

#ifndef _C910_PLATFORM_H_
#define _C910_PLATFORM_H_

#define CSR_MCOR         0x7c2
#define CSR_MHCR         0x7c1
#define CSR_MCCR2        0x7c3
#define CSR_MHINT        0x7c5
#define CSR_MXSTATUS     0x7c0
#define CSR_PLIC_BASE    0xfc1
#define CSR_MRMR         0x7c6
#define CSR_MRVBR        0x7c7

#define PLATFORM_CCI_ADDR           (0x0FE00000)

#define C910_PLIC_CLINT_OFFSET      0x04000000  /* 64M */
#define C910_PLIC_DELEG_OFFSET      0x001ffffc
#define C910_PLIC_DELEG_ENABLE      0x1

#define CLUSTER_ID_BITSHIFT         (2)
#define CLUSTER_ID_MASK             (0x0f << CLUSTER_ID_BITSHIFT)
#define CORE_ID_BITSHIFT            (0)
#define CORE_ID_MASK                (((1 << CLUSTER_ID_BITSHIFT) - 1) << CORE_ID_BITSHIFT)

#define PLAT_CCI_CLUSTER0_IFACE_IX  0
#define PLAT_CCI_CLUSTER1_IFACE_IX  1
#define PLAT_CCI_CLUSTER2_IFACE_IX  2
#define PLAT_CCI_CLUSTER3_IFACE_IX  3


struct c910_regs_struct {
    u64 pmpaddr0;
    u64 pmpaddr1;
    u64 pmpaddr2;
    u64 pmpaddr3;
    u64 pmpaddr4;
    u64 pmpaddr5;
    u64 pmpaddr6;
    u64 pmpaddr7;
    u64 pmpcfg0;
    u64 mcor;
    u64 mhcr;
    u64 mccr2;
    u64 mhint;
    u64 mxstatus;
    u64 plic_base_addr;
    u64 clint_base_addr;
};

#endif /* _C910_PLATFORM_H_ */
