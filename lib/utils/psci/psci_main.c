#include <sbi_utils/psci/psci.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_scratch.h>
#include "psci_private.h"

/*******************************************************************************
 * PSCI frontend api for servicing SMCs. Described in the PSCI spec.
 ******************************************************************************/
int psci_cpu_on(u_register_t target_cpu,
                uintptr_t entrypoint)

{
        int rc;

        /* Determine if the cpu exists of not */
        rc = psci_validate_mpidr(target_cpu);
        if (rc != PSCI_E_SUCCESS)
                return PSCI_E_INVALID_PARAMS;

        /*
         * To turn this cpu on, specify which power
         * levels need to be turned on
         */
        return psci_cpu_on_start(target_cpu, entrypoint);
}

int psci_affinity_info(u_register_t target_affinity,
                       unsigned int lowest_affinity_level)
{
        int ret;
        unsigned int target_idx;
	psci_cpu_data_t *svc_cpu_data;
	struct sbi_scratch *scratch = sbi_hartid_to_scratch(target_affinity);
	svc_cpu_data = sbi_scratch_offset_ptr(scratch, psci_delta_off);

        /* We dont support level higher than PSCI_CPU_PWR_LVL */
        if (lowest_affinity_level > PSCI_CPU_PWR_LVL)
                return PSCI_E_INVALID_PARAMS;

        /* Calculate the cpu index of the target */
        ret = plat_core_pos_by_mpidr(target_affinity);
        if (ret == -1) {
                return PSCI_E_INVALID_PARAMS;
        }
        target_idx = (unsigned int)ret;

        /*
         * Generic management:
         * Perform cache maintanence ahead of reading the target CPU state to
         * ensure that the data is not stale.
         * There is a theoretical edge case where the cache may contain stale
         * data for the target CPU data - this can occur under the following
         * conditions:
         * - the target CPU is in another cluster from the current
         * - the target CPU was the last CPU to shutdown on its cluster
         * - the cluster was removed from coherency as part of the CPU shutdown
         *
         * In this case the cache maintenace that was performed as part of the
         * target CPUs shutdown was not seen by the current CPU's cluster. And
         * so the cache may contain stale data for the target CPU.
         */
	csi_dcache_clean_invalid_range((uintptr_t)svc_cpu_data->aff_info_state, sizeof(aff_info_state_t));

        return psci_get_aff_info_state_by_idx(target_idx);
}

int psci_cpu_off(void)
{
        int rc;
        unsigned int target_pwrlvl = PLAT_MAX_PWR_LVL;

        /*
         * Do what is needed to power off this CPU and possible higher power
         * levels if it able to do so. Upon success, enter the final wfi
         * which will power down this CPU.
         */
        rc = psci_do_cpu_off(target_pwrlvl);

        /*
         * The only error cpu_off can return is E_DENIED. So check if that's
         * indeed the case.
         */
        if (rc != PSCI_E_DENIED) {
		sbi_printf("%s:%d, err\n", __func__, __LINE__);
		sbi_hart_hang();
	}

        return rc;
}
