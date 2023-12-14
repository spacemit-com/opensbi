#ifndef __K1X_EVB_CONFIG_H__
#define __K1X_EVB_CONFIG_H__

/***************************cci******************************/
#define PLATFORM_CCI_ADDR           (0xD8500000)

#define PLAT_CCI_CLUSTER0_IFACE_IX  0
#define PLAT_CCI_CLUSTER1_IFACE_IX  1
#define PLAT_CCI_CLUSTER2_IFACE_IX  2
#define PLAT_CCI_CLUSTER3_IFACE_IX  3

#define PLAT_CCI_MAP static const int cci_map[] = { \
	PLAT_CCI_CLUSTER0_IFACE_IX,	\
	PLAT_CCI_CLUSTER1_IFACE_IX,	\
	PLAT_CCI_CLUSTER2_IFACE_IX,	\
	PLAT_CCI_CLUSTER3_IFACE_IX,	\
};

/***************************cpu******************************/
#define CPU_RESET_BASE_ADDR         (0xD428292C)
#define C0_RVBADDR_LO_ADDR          (0xD4282DB0)
#define C0_RVBADDR_HI_ADDR          (0xD4282DB4)

#define C1_RVBADDR_LO_ADDR          (0xD4282C00 + 0x2B0)
#define C1_RVBADDR_HI_ADDR          (0xD4282C00 + 0X2B4)

/***************************mailbox***************************/
#define SCMI_MAILBOX_SHARE_MEM		(0x2f902080)
#define PLAT_MAILBOX_REG_BASE		(0x2f824000)

/****************************scmi*****************************/
#define PLAT_SCMI_DOMAIN_MAP		{0, 1, 2, 3}

/*************************cpu topology************************/
#define ARM_SYSTEM_COUNT		(1U)
/* this is the max cluster count of this platform */
#define PLATFORM_CLUSTER_COUNT		(2U)
/* this is the max core count of this platform */
#define PLATFORM_CORE_COUNT		(8U)
/* this is the max NUN CPU power domains */
#define PSCI_NUM_NON_CPU_PWR_DOMAINS	(3U)
/* this is the max cpu cores per cluster*/
#define PLATFORM_MAX_CPUS_PER_CLUSTER	(4U)

#define CLUSTER_INDEX_IN_CPU_TOPOLOGY	(1U)
#define CLUSTER0_INDEX_IN_CPU_TOPOLOGY	(2U)
#define CLUSTER1_INDEX_IN_CPU_TOPOLOGY	(3U)

#define PSCI_NUM_PWR_DOMAINS	\
	(ARM_SYSTEM_COUNT + plat_get_power_domain_tree_desc()[CLUSTER_INDEX_IN_CPU_TOPOLOGY] \
	 + plat_get_power_domain_tree_desc()[CLUSTER0_INDEX_IN_CPU_TOPOLOGY] + \
	 plat_get_power_domain_tree_desc()[CLUSTER1_INDEX_IN_CPU_TOPOLOGY])

/***************************psci pwr level********************/
/* This is the power level corresponding to a CPU */
#define PSCI_CPU_PWR_LVL                0U
#define PLAT_MAX_PWR_LVL                2U

/***************************cpu affin*************************/
#define MPIDR_AFFINITY0_MASK		0x3U
#define MPIDR_AFFINITY1_MASK		0xfU
#define MPIDR_AFF0_SHIFT		0U
#define MPIDR_AFF1_SHIFT		2U

/**************************cluster power domain***************/
#define CLUSTER0_L2_CACHE_FLUSH_REG_BASE	(0xD84401B0)
#define CLUSTER1_L2_CACHE_FLUSH_REG_BASE	(0xD84401B4)

#define L2_CACHE_FLUSH_REQUEST_BIT_OFFSET	(0x1)
#define L2_CACHE_FLUSH_DONE_BIT_OFFSET		(0x3)

#endif /* __K1X_EVB_CONFIG_H__ */
