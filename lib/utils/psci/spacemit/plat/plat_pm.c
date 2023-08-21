#include <sbi/sbi_types.h>
#include <sbi/riscv_asm.h>
#include <sbi_utils/cci/cci.h>
#include <sbi_utils/psci/psci.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/psci/plat/arm/common/arm_def.h>
#include "underly_implement.h"

#define CORE_PWR_STATE(state) \
        ((state)->pwr_domain_state[MPIDR_AFFLVL0])
#define CLUSTER_PWR_STATE(state) \
        ((state)->pwr_domain_state[MPIDR_AFFLVL1])
#define SYSTEM_PWR_STATE(state) \
        ((state)->pwr_domain_state[PLAT_MAX_PWR_LVL])

static int spacemit_pwr_domain_on(u_register_t mpidr)
{
	int cluster;
	int curr_cluster;
	int cur_cpu = current_hartid();

	cluster = MPIDR_AFFLVL1_VAL(mpidr);

	curr_cluster = MPIDR_AFFLVL1_VAL(cur_cpu);
	if (cluster != curr_cluster)
		spacemit_cluster_on(mpidr);

	/* de-assert the cpu */
	spacemit_de_assert_cpu(mpidr);

	return 0;
}

static void spacemit_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
        unsigned int hartid = current_hartid();

        /*
         * Enable CCI coherency for this cluster.
         * No need for locks as no other cpu is active at the moment.
         */
        if (CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE)
                cci_enable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(hartid));
}

static void spacemit_pwr_domain_off(const psci_power_state_t *target_state)
{
        unsigned int hartid = current_hartid();

        if (CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE) {
                cci_disable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(hartid));
                spacemit_cluster_off(hartid);
        }

	spacemit_assert_cpu(hartid);
}

static void spacemit_pwr_domain_pwr_down_wfi(const psci_power_state_t *target_state)
{
	while (1) {
		asm volatile ("wfi");
	}
}

static void spacemit_pwr_domain_on_finish_late(const psci_power_state_t *target_state)
{
	spacemit_clr_cpu_idle();
}

static const plat_psci_ops_t spacemit_psci_ops = {
	.cpu_standby = NULL,
	.pwr_domain_on = spacemit_pwr_domain_on,
	.pwr_domain_on_finish = spacemit_pwr_domain_on_finish,
	.pwr_domain_off = spacemit_pwr_domain_off,
	.pwr_domain_pwr_down_wfi = spacemit_pwr_domain_pwr_down_wfi,
	.pwr_domain_on_finish_late = spacemit_pwr_domain_on_finish_late,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint, const plat_psci_ops_t **psci_ops)
{
	*psci_ops = &spacemit_psci_ops;

        return 0;
}
