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
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/psci/psci_lib.h>
#include <sbi_utils/cci/cci.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi_utils/psci/psci.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/cache/cacheflush.h>
#include <../../../lib/utils/psci/psci_private.h>
#include <sbi_utils/psci/plat/arm/common/plat_arm.h>
#include <sbi_utils/psci/plat/common/platform.h>
#include <spacemit/spacemit_config.h>

extern struct sbi_platform platform;

PLAT_CCI_MAP

static void wakeup_other_core(void)
{
    int i;
    u32 hartid, clusterid, cluster_enabled = 0;
    unsigned int cur_hartid = current_hartid();
    struct sbi_scratch *scratch = sbi_hartid_to_scratch(cur_hartid);

#if defined(CONFIG_PLATFORM_SPACEMIT_K1X)
    /* set other cpu's boot-entry */
    writel(scratch->warmboot_addr & 0xffffffff, (u32 *)C0_RVBADDR_LO_ADDR);
    writel((scratch->warmboot_addr >> 32) & 0xffffffff, (u32 *)C0_RVBADDR_HI_ADDR);

    writel(scratch->warmboot_addr & 0xffffffff, (u32 *)C1_RVBADDR_LO_ADDR);
    writel((scratch->warmboot_addr >> 32) & 0xffffffff, (u32 *)C1_RVBADDR_HI_ADDR);
#elif defined(CONFIG_PLATFORM_SPACEMIT_K1PRO)
    for (i = 0; i < platform.hart_count; i++) {
        hartid = platform.hart_index2id[i];

	unsigned long core_index = MPIDR_AFFLVL1_VAL(hartid) * PLATFORM_MAX_CPUS_PER_CLUSTER
	       + MPIDR_AFFLVL0_VAL(hartid);

	writel(scratch->warmboot_addr & 0xffffffff, (u32 *)(CORE0_RVBADDR_LO_ADDR + core_index * CORE_RVBADDR_STEP));
	writel((scratch->warmboot_addr >> 32) & 0xffffffff, (u32 *)(CORE0_RVBADDR_HI_ADDR + core_index * CORE_RVBADDR_STEP));
    }
#endif

#ifdef CONFIG_ARM_PSCI_SUPPORT
    unsigned char *cpu_topology = plat_get_power_domain_tree_desc();
#endif

    // hart0 is already boot up
    for (i = 0; i < platform.hart_count; i++) {
        hartid = platform.hart_index2id[i];

        clusterid = MPIDR_AFFLVL1_VAL(hartid);

	/* we only enable snoop of cluster0 */
        if (0 == (cluster_enabled & (1 << clusterid))) {
            cluster_enabled |= 1 << clusterid;
            if (0 == clusterid) {
		cci_enable_snoop_dvm_reqs(clusterid);
	    }
#ifdef CONFIG_ARM_PSCI_SUPPORT
	    cpu_topology[CLUSTER_INDEX_IN_CPU_TOPOLOGY]++;
#endif
	}

#ifdef CONFIG_ARM_PSCI_SUPPORT
	/* we only support 2 cluster by now */
	if (clusterid == PLATFORM_CLUSTER_COUNT - 1)
		cpu_topology[CLUSTER1_INDEX_IN_CPU_TOPOLOGY]++;
	else
		cpu_topology[CLUSTER0_INDEX_IN_CPU_TOPOLOGY]++;
#endif
    }
}

/*
 * Platform early initialization.
 */
static int spacemit_k1_early_init(bool cold_boot, const struct fdt_match *match)
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
	psci_cpu_suspend(suspend_type, 0, 0);

	return 0;
}

static void spacemit_hart_resume(void)
{
	psci_warmboot_entrypoint();
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
static int spacemit_k1_final_init(bool cold_boot, const struct fdt_match *match)
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

static bool spacemit_cold_boot_allowed(u32 hartid, const struct fdt_match *match)
{
	/* enable core snoop */
	csr_set(CSR_ML2SETUP, 1 << (hartid % PLATFORM_MAX_CPUS_PER_CLUSTER));

	/* dealing with resuming process */
	if ((__sbi_hsm_hart_get_state(hartid) == SBI_HSM_STATE_SUSPENDED) && (hartid == 0))
		return false;

	return ((hartid == 0) ? true : false);
}

static const struct fdt_match spacemit_k1_match[] = {
	{ .compatible = "spacemit,k1-pro" },
	{ .compatible = "spacemit,k1x" },
	{ },
};

const struct platform_override spacemit_k1 = {
	.match_table = spacemit_k1_match,
	.early_init = spacemit_k1_early_init,
	.final_init = spacemit_k1_final_init,
	.cold_boot_allowed = spacemit_cold_boot_allowed,
};
