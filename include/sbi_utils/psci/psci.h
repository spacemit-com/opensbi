#ifndef __PSCI_H__
#define __PSCI_H__

/*
 *  Macros for local power states in ARM platforms encoded by State-ID field
 *  within the power-state parameter.
 */
/* Local power state for power domains in Run state. */
#define ARM_LOCAL_STATE_RUN     0U
/* Local power state for retention. Valid only for CPU power domains */
#define ARM_LOCAL_STATE_RET     1U
/* Local power state for OFF/power-down. Valid for CPU and cluster power
   domains */
#define ARM_LOCAL_STATE_OFF     2U


/*
 * This macro defines the deepest retention state possible. A higher state
 * id will represent an invalid or a power down state.
 */
#define PLAT_MAX_RET_STATE              ARM_LOCAL_STATE_RET

/*
 * This macro defines the deepest power down states possible. Any state ID
 * higher than this is invalid.
 */
#define PLAT_MAX_OFF_STATE              ARM_LOCAL_STATE_OFF

/*
 * Type for representing the local power state at a particular level.
 */
typedef unsigned char plat_local_state_t;

/* The local state macro used to represent RUN state. */
#define PSCI_LOCAL_STATE_RUN    0U

typedef unsigned long u_register_t;

#define PSCI_INVALID_MPIDR      ~((u_register_t)0)


/*
 * These are the states reported by the PSCI_AFFINITY_INFO API for the specified
 * CPU. The definitions of these states can be found in Section 5.7.1 in the
 * PSCI specification (ARM DEN 0022C).
 */
typedef enum {
	AFF_STATE_ON = 0U,
	AFF_STATE_OFF = 1U,
	AFF_STATE_ON_PENDING = 2U
} aff_info_state_t;

/*******************************************************************************
 * Structure used to store per-cpu information relevant to the PSCI service.
 * It is populated in the per-cpu data array. In return we get a guarantee that
 * this information will not reside on a cache line shared with another cpu.
 ******************************************************************************/
typedef struct psci_cpu_data {
	/* State as seen by PSCI Affinity Info API */
	aff_info_state_t aff_info_state;

	/*
	 * Highest power level which takes part in a power management
	 * operation.
	 */
	unsigned int target_pwrlvl;

	/* The local power state of this CPU */
	plat_local_state_t local_state;
} psci_cpu_data_t;

/* This is the power level corresponding to a CPU */
#define PSCI_CPU_PWR_LVL		0U

#define PLAT_MAX_PWR_LVL                2U

/*
 * Macro to represent invalid affinity level within PSCI.
 */
#define PSCI_INVALID_PWR_LVL    (PLAT_MAX_PWR_LVL + 1U)


#define ARM_SYSTEM_COUNT                1U

/*******************************************************************************
 * spacemit topology related constants
 ******************************************************************************/
#define SPACEMIT_CLUSTER_COUNT              2U
#define SPACEMIT_CLUSTER0_CORE_COUNT        4U
#define SPACEMIT_CLUSTER1_CORE_COUNT        4U

#define PLATFORM_CORE_COUNT	(SPACEMIT_CLUSTER0_CORE_COUNT + \
					SPACEMIT_CLUSTER1_CORE_COUNT)

#define PLAT_NUM_PWR_DOMAINS	(ARM_SYSTEM_COUNT + \
				SPACEMIT_CLUSTER_COUNT + \
					PLATFORM_CORE_COUNT)

#define PSCI_NUM_PWR_DOMAINS		PLAT_NUM_PWR_DOMAINS

#define PSCI_NUM_NON_CPU_PWR_DOMAINS    (PSCI_NUM_PWR_DOMAINS - \
                                         PLATFORM_CORE_COUNT)

#endif
