/*
 * Copyright (c) 2013-2022, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <sbi_utils/psci/psci.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_console.h>
#include "psci_private.h"

/*
 * PSCI requested local power state map. This array is used to store the local
 * power states requested by a CPU for power levels from level 1 to
 * PLAT_MAX_PWR_LVL. It does not store the requested local power state for power
 * level 0 (PSCI_CPU_PWR_LVL) as the requested and the target power state for a
 * CPU are the same.
 *
 * During state coordination, the platform is passed an array containing the
 * local states requested for a particular non cpu power domain by each cpu
 * within the domain.
 *
 * TODO: Dense packing of the requested states will cause cache thrashing
 * when multiple power domains write to it. If we allocate the requested
 * states at each power level in a cache-line aligned per-domain memory,
 * the cache thrashing can be avoided.
 */
static plat_local_state_t
	psci_req_local_pwr_states[PLAT_MAX_PWR_LVL][PLATFORM_CORE_COUNT];

unsigned int psci_plat_core_count;

unsigned long psci_delta_off;

/*******************************************************************************
 * Arrays that hold the platform's power domain tree information for state
 * management of power domains.
 * Each node in the array 'psci_non_cpu_pd_nodes' corresponds to a power domain
 * which is an ancestor of a CPU power domain.
 * Each node in the array 'psci_cpu_pd_nodes' corresponds to a cpu power domain
 ******************************************************************************/
non_cpu_pd_node_t psci_non_cpu_pd_nodes[PSCI_NUM_NON_CPU_PWR_DOMAINS];

/* Lock for PSCI state coordination */
DEFINE_PSCI_LOCK(psci_locks[PSCI_NUM_NON_CPU_PWR_DOMAINS]);

cpu_pd_node_t psci_cpu_pd_nodes[PLATFORM_CORE_COUNT];

/*
 * The plat_local_state used by the platform is one of these types: RUN,
 * RETENTION and OFF. The platform can define further sub-states for each type
 * apart from RUN. This categorization is done to verify the sanity of the
 * psci_power_state passed by the platform and to print debug information. The
 * categorization is done on the basis of the following conditions:
 *
 * 1. If (plat_local_state == 0) then the category is STATE_TYPE_RUN.
 *
 * 2. If (0 < plat_local_state <= PLAT_MAX_RET_STATE), then the category is
 *    STATE_TYPE_RETN.
 *
 * 3. If (plat_local_state > PLAT_MAX_RET_STATE), then the category is
 *    STATE_TYPE_OFF.
 */
typedef enum plat_local_state_type {
        STATE_TYPE_RUN = 0,
        STATE_TYPE_RETN,
        STATE_TYPE_OFF
} plat_local_state_type_t;

/* Function used to categorize plat_local_state. */
plat_local_state_type_t find_local_state_type(plat_local_state_t state)
{
        if (state != 0U) {
                if (state > PLAT_MAX_RET_STATE) {
                        return STATE_TYPE_OFF;
                } else {
                        return STATE_TYPE_RETN;
                }
        } else {
                return STATE_TYPE_RUN;
        }
}

/*******************************************************************************
 * PSCI helper function to get the parent nodes corresponding to a cpu_index.
 ******************************************************************************/
void psci_get_parent_pwr_domain_nodes(unsigned int cpu_idx,
				      unsigned int end_lvl,
				      unsigned int *node_index)
{
	unsigned int parent_node = psci_cpu_pd_nodes[cpu_idx].parent_node;
	unsigned int i;
	unsigned int *node = node_index;

	for (i = PSCI_CPU_PWR_LVL + 1U; i <= end_lvl; i++) {
		*node = parent_node;
		node++;
		parent_node = psci_non_cpu_pd_nodes[parent_node].parent_node;
	}
}

/******************************************************************************
 * This function initializes the psci_req_local_pwr_states.
 *****************************************************************************/
void psci_init_req_local_pwr_states(void)
{
	/* Initialize the requested state of all non CPU power domains as OFF */
	unsigned int pwrlvl;
	unsigned int core;

	for (pwrlvl = 0U; pwrlvl < PLAT_MAX_PWR_LVL; pwrlvl++) {
		for (core = 0; core < psci_plat_core_count; core++) {
			psci_req_local_pwr_states[pwrlvl][core] =
				PLAT_MAX_OFF_STATE;
		}
	}
}

void set_non_cpu_pd_node_local_state(unsigned int parent_idx,
		plat_local_state_t state)
{
	psci_non_cpu_pd_nodes[parent_idx].local_state = state;
}

/******************************************************************************
 * Helper function to update the requested local power state array. This array
 * does not store the requested state for the CPU power level. Hence an
 * assertion is added to prevent us from accessing the CPU power level.
 *****************************************************************************/
void psci_set_req_local_pwr_state(unsigned int pwrlvl,
					 unsigned int cpu_idx,
					 plat_local_state_t req_pwr_state)
{
	if ((pwrlvl > PSCI_CPU_PWR_LVL) && (pwrlvl <= PLAT_MAX_PWR_LVL) &&
			(cpu_idx < psci_plat_core_count)) {
		psci_req_local_pwr_states[pwrlvl - 1U][cpu_idx] = req_pwr_state;
	}
}

/*
 * Helper functions to get/set the fields of PSCI per-cpu data.
 */
void psci_set_aff_info_state(aff_info_state_t aff_state)
{
	psci_cpu_data_t *svc_cpu_data;
	unsigned int hartid = current_hartid();
	struct sbi_scratch *scratch = sbi_hartid_to_scratch(hartid);

	svc_cpu_data = sbi_scratch_offset_ptr(scratch, psci_delta_off);

	svc_cpu_data->aff_info_state = aff_state;
}

void psci_set_cpu_local_state(plat_local_state_t state)
{
	psci_cpu_data_t *svc_cpu_data;
	unsigned int hartid = current_hartid();
	struct sbi_scratch *scratch = sbi_hartid_to_scratch(hartid);

	svc_cpu_data = sbi_scratch_offset_ptr(scratch, psci_delta_off);

	svc_cpu_data->local_state = state;
}

static inline plat_local_state_t psci_get_cpu_local_state_by_idx(
                unsigned int idx)
{
	psci_cpu_data_t *svc_cpu_data;
	const struct sbi_platform *sbi = sbi_platform_thishart_ptr();
	unsigned int hartid = sbi->hart_index2id[idx];
	struct sbi_scratch *scratch = sbi_hartid_to_scratch(hartid);

	svc_cpu_data = sbi_scratch_offset_ptr(scratch, psci_delta_off);

	return svc_cpu_data->local_state;
}

/******************************************************************************
 * This function is invoked post CPU power up and initialization. It sets the
 * affinity info state, target power state and requested power state for the
 * current CPU and all its ancestor power domains to RUN.
 *****************************************************************************/
void psci_set_pwr_domains_to_run(unsigned int end_pwrlvl)
{
	unsigned int parent_idx, lvl;
	unsigned int cpu_idx;
	unsigned int hartid = current_hartid();
	const struct sbi_platform *sbi = sbi_platform_thishart_ptr();

        for (cpu_idx = 0; cpu_idx < sbi->hart_count; ++cpu_idx) {
		if (sbi->hart_index2id[cpu_idx] == hartid)
			break;
        }

	parent_idx = psci_cpu_pd_nodes[cpu_idx].parent_node;

	/* Reset the local_state to RUN for the non cpu power domains. */
	for (lvl = PSCI_CPU_PWR_LVL + 1U; lvl <= end_pwrlvl; lvl++) {
		set_non_cpu_pd_node_local_state(parent_idx,
				PSCI_LOCAL_STATE_RUN);
		psci_set_req_local_pwr_state(lvl,
					     cpu_idx,
					     PSCI_LOCAL_STATE_RUN);
		parent_idx = psci_non_cpu_pd_nodes[parent_idx].parent_node;
	}

	/* Set the affinity info state to ON */
	psci_set_aff_info_state(AFF_STATE_ON);

	psci_set_cpu_local_state(PSCI_LOCAL_STATE_RUN);
}

/*******************************************************************************
 * This function prints the state of all power domains present in the
 * system
 ******************************************************************************/
void psci_print_power_domain_map(void)
{
        unsigned int idx;
        plat_local_state_t state;
        plat_local_state_type_t state_type;

        /* This array maps to the PSCI_STATE_X definitions in psci.h */
        static const char * const psci_state_type_str[] = {
                "ON",
                "RETENTION",
                "OFF",
        };

        sbi_printf("PSCI Power Domain Map:\n");
        for (idx = 0; idx < (PSCI_NUM_PWR_DOMAINS - psci_plat_core_count);
                                                        idx++) {
                state_type = find_local_state_type(
                                psci_non_cpu_pd_nodes[idx].local_state);
                sbi_printf("  Domain Node : Level %u, parent_node %u,"
                                " State %s (0x%x)\n",
                                psci_non_cpu_pd_nodes[idx].level,
                                psci_non_cpu_pd_nodes[idx].parent_node,
                                psci_state_type_str[state_type],
                                psci_non_cpu_pd_nodes[idx].local_state);
        }

        for (idx = 0; idx < psci_plat_core_count; idx++) {
                state = psci_get_cpu_local_state_by_idx(idx);
                state_type = find_local_state_type(state);
                sbi_printf("  CPU Node : MPID 0x%llx, parent_node %u,"
                                " State %s (0x%x)\n",
                                (unsigned long long)psci_cpu_pd_nodes[idx].mpidr,
                                psci_cpu_pd_nodes[idx].parent_node,
                                psci_state_type_str[state_type],
                                psci_get_cpu_local_state_by_idx(idx));
        }
}
