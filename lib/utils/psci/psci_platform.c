#include <sbi_utils/psci/psci.h>

static const unsigned char plat_power_domain_tree_desc[] = {
	/* No of root nodes */
        ARM_SYSTEM_COUNT,
	/* No of children for the root node */
        SPACEMIT_CLUSTER_COUNT,
	/* No of children for the first cluster node */
        SPACEMIT_CLUSTER0_CORE_COUNT,
	/* No of children for the second cluster node */
	SPACEMIT_CLUSTER1_CORE_COUNT,
};

int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	return 0;
}

const unsigned char *plat_get_power_domain_tree_desc(void)
{
        return plat_power_domain_tree_desc;
}

