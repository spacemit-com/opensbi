/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Spacemit.
 */

#include <libfdt.h>
#include <platform_override.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/psci/psci_lib.h>
#include <sbi_utils/cci/cci.h>

#define CSR_MHCR                    (0x7c1)
#define CSR_MCCR2                   (0x7c3)
#define CSR_MHINT                   (0x7c5)
#define CSR_MRMR                    (0x7c6)
#define CSR_MRVBR                   (0x7c7)
#define CSR_MSETUP                  (0x7C0)
#define CSR_MCPM                    (0x7C1)
#define CSR_MPCTL                   (0x7D0)
#define CSR_ML2SETUP                (0x7F0)

#define PLATFORM_CCI_ADDR           (0x0FE00000)
#define CPU_RESET_BASE_ADDR         (0x2F024000)

#define X60_PLIC_CLINT_OFFSET       (0x04000000)  /* 64M */
#define X60_PLIC_DELEG_OFFSET       (0x001ffffc)
#define X60_PLIC_DELEG_ENABLE       (0x1)

#define CLUSTER_ID_BITSHIFT         (2)
#define CLUSTER_ID_MASK             (0x0f << CLUSTER_ID_BITSHIFT)
#define CORE_ID_BITSHIFT            (0)
#define CORE_ID_MASK                (((1 << CLUSTER_ID_BITSHIFT) - 1) << CORE_ID_BITSHIFT)

#define PLAT_CCI_CLUSTER0_IFACE_IX  0
#define PLAT_CCI_CLUSTER1_IFACE_IX  1
#define PLAT_CCI_CLUSTER2_IFACE_IX  2
#define PLAT_CCI_CLUSTER3_IFACE_IX  3

extern struct sbi_platform platform;

static const int cci_map[] = {
    PLAT_CCI_CLUSTER0_IFACE_IX,
    PLAT_CCI_CLUSTER1_IFACE_IX,
    PLAT_CCI_CLUSTER2_IFACE_IX,
    PLAT_CCI_CLUSTER3_IFACE_IX,
};

void raise_soc_performance(void)
{
    csr_write(CSR_MHCR, 0x10011ff);
    csr_write(CSR_MHINT, 0x6e30c);
}

static void cache_enable(void)
{
    // enable Dache, Icache, branch predict, prefetch predict, unalign access, ECC en
    csr_set(CSR_MSETUP, 0x10073);
    // csr_set(CSR_MCPM, 0x300000031);
    // csr_set(CSR_MPCTL, 0xB10);
}

static void wakeup_other_core(void)
{
    int i;
    u32 hartid, clusterid, coreid, cluster_enabled = 0;
    u32 *cpu_reset_reg;

    // hart0 is already boot up
    for (i = 1; i < platform.hart_count; i++) {
        hartid = platform.hart_index2id[i];

        // cluster0 had release reset
        clusterid = (hartid & CLUSTER_ID_MASK) >> CLUSTER_ID_BITSHIFT;
        coreid = (hartid & CORE_ID_MASK) >> CORE_ID_BITSHIFT;

        cpu_reset_reg = (u32 *)CPU_RESET_BASE_ADDR + clusterid;
        if (0 == (cluster_enabled & (1 << clusterid))) {
            cluster_enabled |= 1 << clusterid;

            if (0 != clusterid)
                // de-assert cluster 1 APB, L2C, PIC, ACEM/LLP
                writel(readl(cpu_reset_reg) | 0x0F, cpu_reset_reg);
            /* enable cci for current cluster */
            cci_enable_snoop_dvm_reqs(clusterid);
        }

        writel(readl(cpu_reset_reg) | 1 << (coreid + 4), cpu_reset_reg);
    }
}

/*
 * Platform early initialization.
 */
static int spacemit_k1x_early_init(bool cold_boot, const struct fdt_match *match)
{
    if (cold_boot) {
        /* initiate cci */
        cci_init(PLATFORM_CCI_ADDR, cci_map, array_size(cci_map));

        cache_enable();
        wakeup_other_core();
    } else {
        cache_enable();
    }

    return 0;
}

/*
 * Platform final initialization.
 */
static int spacemit_k1x_final_init(bool cold_boot, const struct fdt_match *match)
{
    /* for clod boot, we build the cpu topology structure */
    if (cold_boot)
	    return psci_setup();

    return 0;
}

static const struct fdt_match spacemit_k1x_match[] = {
	{ .compatible = "spacemit,k1x" },
	{ },
};

const struct platform_override spacemit_k1x = {
	.match_table = spacemit_k1x_match,
	.early_init = spacemit_k1x_early_init,
	.final_init = spacemit_k1x_final_init,
};
