/*
 * Copyright (c) 2015-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_hart.h>
#include <sbi/riscv_asm.h>
#include <sbi_utils/psci/psci.h>
#include <sbi_utils/cci/cci.h>
#include <sbi_utils/psci/plat/arm/common/arm_def.h>
#include <sbi_utils/psci/plat/arm/css/common/css_pm.h>
#include <sbi_utils/psci/drivers/arm/css/css_scp.h>

/* Allow CSS platforms to override `plat_arm_psci_pm_ops` */
#pragma weak plat_arm_psci_pm_ops

/*******************************************************************************
 * Handler called when a power domain is about to be turned on. The
 * level and mpidr determine the affinity instance.
 ******************************************************************************/
int css_pwr_domain_on(u_register_t mpidr)
{
	css_scp_on(mpidr);

	return PSCI_E_SUCCESS;
}

static void css_pwr_domain_on_finisher_common(
		const psci_power_state_t *target_state)
{
	unsigned int clusterid;
	unsigned int hartid = current_hartid();

	if (CSS_CORE_PWR_STATE(target_state) != ARM_LOCAL_STATE_OFF) {
		sbi_printf("%s:%d\n", __func__, __LINE__);
		sbi_hart_hang();
	}

	/*
	 * Perform the common cluster specific operations i.e enable coherency
	 * if this cluster was off.
	 */
	if (CSS_CLUSTER_PWR_STATE(target_state) == ARM_LOCAL_STATE_OFF) {
		clusterid = MPIDR_AFFLVL1_VAL(hartid);
		cci_enable_snoop_dvm_reqs(clusterid);
	}
}

/*******************************************************************************
 * Handler called when a power level has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from. This handler would never be invoked with
 * the system power domain uninitialized as either the primary would have taken
 * care of it as part of cold boot or the first core awakened from system
 * suspend would have already initialized it.
 ******************************************************************************/
void css_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	/* Assert that the system power domain need not be initialized */
	if (css_system_pwr_state(target_state) != ARM_LOCAL_STATE_RUN) {
		sbi_printf("%s:%d\n", __func__, __LINE__);
		sbi_hart_hang();
	}

	css_pwr_domain_on_finisher_common(target_state);
}

/*******************************************************************************
 * Handler called when a power domain has just been powered on and the cpu
 * and its cluster are fully participating in coherent transaction on the
 * interconnect. Data cache must be enabled for CPU at this point.
 ******************************************************************************/
void css_pwr_domain_on_finish_late(const psci_power_state_t *target_state)
{
#if 0
	/* Program the gic per-cpu distributor or re-distributor interface */
	plat_arm_gic_pcpu_init();

	/* Enable the gic cpu interface */
	plat_arm_gic_cpuif_enable();

	/* Setup the CPU power down request interrupt for secondary core(s) */
	css_setup_cpu_pwr_down_intr();
#endif
}

/*******************************************************************************
 * Common function called while turning a cpu off or suspending it. It is called
 * from css_off() or css_suspend() when these functions in turn are called for
 * power domain at the highest power level which will be powered down. It
 * performs the actions common to the OFF and SUSPEND calls.
 ******************************************************************************/
static void css_power_down_common(const psci_power_state_t *target_state)
{
	unsigned int clusterid;
	unsigned int hartid = current_hartid();
#if 0
	/* Prevent interrupts from spuriously waking up this cpu */
	plat_arm_gic_cpuif_disable();
#endif
	/* Cluster is to be turned off, so disable coherency */
	if (CSS_CLUSTER_PWR_STATE(target_state) == ARM_LOCAL_STATE_OFF) {
		clusterid = MPIDR_AFFLVL1_VAL(hartid);
		cci_disable_snoop_dvm_reqs(clusterid);
	}
}

/*******************************************************************************
 * Handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void css_pwr_domain_off(const psci_power_state_t *target_state)
{
	if (CSS_CORE_PWR_STATE(target_state) != ARM_LOCAL_STATE_OFF) {
		sbi_printf("%s:%d\n", __func__, __LINE__);
		sbi_hart_hang();
	}

	css_power_down_common(target_state);
	css_scp_off(target_state);
}

void css_pwr_down_wfi(const psci_power_state_t *target_state)
{
	while (1) {
		asm volatile ("wfi");
	}
}

/*******************************************************************************
 * Export the platform handlers via plat_arm_psci_pm_ops. The ARM Standard
 * platform will take care of registering the handlers with PSCI.
 ******************************************************************************/
plat_psci_ops_t plat_arm_psci_pm_ops = {
	.pwr_domain_on		= css_pwr_domain_on,
	.pwr_domain_on_finish	= css_pwr_domain_on_finish,
	.pwr_domain_on_finish_late = css_pwr_domain_on_finish_late,
	.pwr_domain_off		= css_pwr_domain_off,
	.pwr_domain_pwr_down_wfi = css_pwr_down_wfi,
};
