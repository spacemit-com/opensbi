#ifndef __K1PRO_SIM_CONFIG_H__
#define __K1PRO_SIM_CONFIG_H__

/***************************cci******************************/
#define PLATFORM_CCI_ADDR           (0x0FE00000)

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
#define CPU_RESET_BASE_ADDR         (0x2F024000)


/***************************mailbox***************************/
#define SCMI_MAILBOX_SHARE_MEM		(0x2f902080)
#define PLAT_MAILBOX_REG_BASE		(0x2f824000)

/****************************scmi*****************************/
#define PLAT_SCMI_DOMAIN_MAP		{0, 1, 2, 3}

/*************************cpu topology************************/
#define ARM_SYSTEM_COUNT			1U
#define SPACEMIT_CLUSTER_COUNT			1U
#define SPACEMIT_CLUSTER0_CORE_COUNT		4U
#define SPACEMIT_CLUSTER1_CORE_COUNT		0U

/***************************psci pwr level********************/
/* This is the power level corresponding to a CPU */
#define PSCI_CPU_PWR_LVL                0U
#define PLAT_MAX_PWR_LVL                2U

/***************************cpu affin*************************/
#define MPIDR_AFFINITY0_MASK		0x3U
#define MPIDR_AFFINITY1_MASK		0xfU
#define MPIDR_AFF0_SHIFT		0U
#define MPIDR_AFF1_SHIFT		2U

#endif /* __K1PRO_SIM_CONFIG_H__ */
