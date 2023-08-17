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
#include <sbi/sbi_hsm.h>
#include <sbi_utils/psci/psci.h>
#include <sbi_utils/cache/cacheflush.h>
#include <../../../lib/utils/psci/psci_private.h>
#include <sbi_utils/psci/plat/arm/common/plat_arm.h>
#include <spacemit/spacemit_config.h>

extern struct sbi_platform platform;

PLAT_CCI_MAP

void raise_soc_performance(void)
{
    csr_write(CSR_MHCR, 0x10011ff);
    csr_write(CSR_MHINT, 0x6e30c);
}

static void wakeup_other_core(void)
{
    int i;
    u32 hartid, clusterid, cluster_enabled = 0;

    // hart0 is already boot up
    for (i = 1; i < platform.hart_count; i++) {
        hartid = platform.hart_index2id[i];

        // cluster0 had release reset
        clusterid = MPIDR_AFFLVL1_VAL(hartid);;

#ifndef CONFIG_ARM_PSCI_SUPPORT
    	u32 *cpu_reset_reg;
	u32 coreid;
        cpu_reset_reg = (u32 *)CPU_RESET_BASE_ADDR + clusterid;
        coreid = MPIDR_AFFLVL0_VAL(hartid);

	if (0 == (cluster_enabled & (1 << clusterid))) {
            cluster_enabled |= 1 << clusterid;

            if (0 != clusterid)
                // de-assert cluster 1 APB, L2C, PIC, ACEM/LLP
                writel(readl(cpu_reset_reg) | 0x0F, cpu_reset_reg);
            /* enable cci for current cluster */
            cci_enable_snoop_dvm_reqs(clusterid);
        }

        writel(readl(cpu_reset_reg) | 1 << (coreid + 4), cpu_reset_reg);
#else
	/* we only enable snoop of cluster0 */
        if (0 == (cluster_enabled & (1 << clusterid))) {
            cluster_enabled |= 1 << clusterid;
            if (0 == clusterid) {
		cci_enable_snoop_dvm_reqs(clusterid);
	    }
	}
#endif
    }
}

/*
 * Platform early initialization.
 */
static int spacemit_pro_early_init(bool cold_boot, const struct fdt_match *match)
{
    if (cold_boot) {
        /* initiate cci */
        cci_init(PLATFORM_CCI_ADDR, cci_map, array_size(cci_map));
	/* enable dcache */
        csi_enable_dcache();
	/* wakeup other core ? */
	wakeup_other_core();
	/* initialize */
#ifdef CONFIG_ARM_SCMI_PROTOCOL_SUPPORT
	plat_arm_pwrc_setup();
#endif
    } else {
#ifdef CONFIG_ARM_PSCI_SUPPORT
	psci_warmboot_entrypoint();
#else
	csi_enable_dcache();
#endif
    }

    return 0;
}

#ifdef CONFIG_ARM_PSCI_SUPPORT
/** Start (or power-up) the given hart */
static int spacemit_hart_start(unsigned int hartid, unsigned long saddr)
{
	return psci_cpu_on_start(hartid, saddr);
}

/**
 * Stop (or power-down) the current hart from running. This call
 * doesn't expect to return if success.
 */
static int spacemit_hart_stop(void)
{
	psci_cpu_off();

	return 0;
}

static int spacemit_hart_suspend(unsigned int suspend_type)
{
	return 0;
}

static void spacemit_hart_resume(void)
{

}

static const struct sbi_hsm_device spacemit_hsm_ops = {
	.name		= "spacemit-hsm",
	.hart_start	= spacemit_hart_start,
	.hart_stop	= spacemit_hart_stop,
	.hart_suspend	= spacemit_hart_suspend,
	.hart_resume	= spacemit_hart_resume,
};
#endif

/*
 * Platform final initialization.
 */
static int spacemit_pro_final_init(bool cold_boot, const struct fdt_match *match)
{
#ifdef CONFIG_ARM_PSCI_SUPPORT
    /* for clod boot, we build the cpu topology structure */
    if (cold_boot) {
	    sbi_hsm_set_device(&spacemit_hsm_ops);
	    return psci_setup();
    }
#endif

    return 0;
}

static const struct fdt_match spacemit_pro_match[] = {
	{ .compatible = "spacemit,k1-pro" },
	{ .compatible = "spacemit,k1x" },
	{ },
};

const struct platform_override spacemit_pro = {
	.match_table = spacemit_pro_match,
	.early_init = spacemit_pro_early_init,
	.final_init = spacemit_pro_final_init,
};
