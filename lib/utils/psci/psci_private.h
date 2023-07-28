#ifndef __PSCI_PRIVATE_H__
#define __PSCI_PRIVATE_H__

#include <sbi/riscv_locks.h>
#include <sbi_utils/psci/psci.h>

/*******************************************************************************
 * The following two data structures implement the power domain tree. The tree
 * is used to track the state of all the nodes i.e. power domain instances
 * described by the platform. The tree consists of nodes that describe CPU power
 * domains i.e. leaf nodes and all other power domains which are parents of a
 * CPU power domain i.e. non-leaf nodes.
 ******************************************************************************/
typedef struct non_cpu_pwr_domain_node {
	/*
	 * Index of the first CPU power domain node level 0 which has this node
	 * as its parent.
	 */
	unsigned int cpu_start_idx;

	/*
	 * Number of CPU power domains which are siblings of the domain indexed
	 * by 'cpu_start_idx' i.e. all the domains in the range 'cpu_start_idx
	 * -> cpu_start_idx + ncpus' have this node as their parent.
	 */
	unsigned int ncpus;

	/*
	 * Index of the parent power domain node.
	 * TODO: Figure out whether to whether using pointer is more efficient.
	 */
	unsigned int parent_node;

	plat_local_state_t local_state;

	unsigned char level;

	/* For indexing the psci_lock array*/
	unsigned short lock_index;
} non_cpu_pd_node_t;

typedef struct cpu_pwr_domain_node {
	u_register_t mpidr;

	/*
	 * Index of the parent power domain node.
	 * TODO: Figure out whether to whether using pointer is more efficient.
	 */
	unsigned int parent_node;

	/*
	 * A CPU power domain does not require state coordination like its
	 * parent power domains. Hence this node does not include a bakery
	 * lock. A spinlock is required by the CPU_ON handler to prevent a race
	 * when multiple CPUs try to turn ON the same target CPU.
	 */
	spinlock_t cpu_lock;
} cpu_pd_node_t;

#define DEFINE_PSCI_LOCK(_name)         spinlock_t _name

static inline void psci_lock_init(non_cpu_pd_node_t *non_cpu_pd_node, unsigned short idx)
{
        non_cpu_pd_node[idx].lock_index = idx;
}



extern non_cpu_pd_node_t psci_non_cpu_pd_nodes[PSCI_NUM_NON_CPU_PWR_DOMAINS];
extern cpu_pd_node_t psci_cpu_pd_nodes[PLATFORM_CORE_COUNT];
extern unsigned int psci_plat_core_count;
extern unsigned long psci_delta_off;

void psci_get_parent_pwr_domain_nodes(unsigned int cpu_idx,
				      unsigned int end_lvl,
				      unsigned int *node_index);

void psci_init_req_local_pwr_states(void);
void set_non_cpu_pd_node_local_state(unsigned int parent_idx,
		plat_local_state_t state);
void psci_set_req_local_pwr_state(unsigned int pwrlvl,
					 unsigned int cpu_idx,
					 plat_local_state_t req_pwr_state);
void psci_set_aff_info_state(aff_info_state_t aff_state);
void psci_set_cpu_local_state(plat_local_state_t state);
void psci_set_pwr_domains_to_run(unsigned int end_pwrlvl);

const unsigned char *plat_get_power_domain_tree_desc(void);

#endif
