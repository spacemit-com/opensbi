#include <sbi/riscv_io.h>
#include <sbi/sbi_types.h>
#include <sbi/riscv_asm.h>
#include <sbi_utils/psci/psci.h>
#include <sbi/sbi_console.h>
#include <spacemit/spacemit_config.h>

#define C1_CPU_RESET_BASE_ADDR		(0xD4282B24)

#define PMU_CAP_CORE0_IDLE_CFG		(0xd4282924)
#define PMU_CAP_CORE1_IDLE_CFG		(0xd4282928)
#define PMU_CAP_CORE2_IDLE_CFG		(0xd4282960)
#define PMU_CAP_CORE3_IDLE_CFG		(0xd4282964)
#define PMU_CAP_CORE4_IDLE_CFG		(0xd4282b04)
#define PMU_CAP_CORE5_IDLE_CFG		(0xd4282b08)
#define PMU_CAP_CORE6_IDLE_CFG		(0xd4282b0c)
#define PMU_CAP_CORE7_IDLE_CFG		(0xd4282b10)

#define PMU_C0_CAPMP_IDLE_CFG0		(0xd4282920)
#define PMU_C0_CAPMP_IDLE_CFG1		(0xd42828e4)
#define PMU_C0_CAPMP_IDLE_CFG2		(0xd4282950)
#define PMU_C0_CAPMP_IDLE_CFG3		(0xd4282954)
#define PMU_C1_CAPMP_IDLE_CFG0		(0xd4282b14)
#define PMU_C1_CAPMP_IDLE_CFG1		(0xd4282b18)
#define PMU_C1_CAPMP_IDLE_CFG2		(0xd4282b1c)
#define PMU_C1_CAPMP_IDLE_CFG3		(0xd4282b20)

#define CPU_PWR_DOWN_VALUE		(0x3)
#define CLUSTER_PWR_DOWN_VALUE		(0x3)

struct pmu_cap_wakeup {
	unsigned int pmu_cap_core0_wakeup;
	unsigned int pmu_cap_core1_wakeup;
	unsigned int pmu_cap_core2_wakeup;
	unsigned int pmu_cap_core3_wakeup;
};

void spacemit_cluster_on(u_register_t mpidr)
{
	/* for k1x, we do nothing */
}

void spacemit_cluster_off(u_register_t mpidr)
{

}

void spacemit_de_assert_cpu(u_register_t mpidr)
{
	unsigned int *cpu_reset_base;
	struct pmu_cap_wakeup *pmu_cap_wakeup;
	unsigned int cur_cluster, cur_cpu;
	unsigned int target_cpu_idx;
	unsigned int cur_hartid = current_hartid();

	cur_cluster = MPIDR_AFFLVL1_VAL(cur_hartid);
	cur_cpu = MPIDR_AFFLVL0_VAL(cur_hartid);

	pmu_cap_wakeup = (struct pmu_cap_wakeup *)((cur_cluster == 0) ? (unsigned int *)CPU_RESET_BASE_ADDR :
			(unsigned int *)C1_CPU_RESET_BASE_ADDR);

	switch (cur_cpu) {
		case 0:
			cpu_reset_base = &pmu_cap_wakeup->pmu_cap_core0_wakeup;
			break;
		case 1:
			cpu_reset_base = &pmu_cap_wakeup->pmu_cap_core1_wakeup;
			break;
		case 2:
			cpu_reset_base = &pmu_cap_wakeup->pmu_cap_core2_wakeup;
			break;
		case 3:
			cpu_reset_base = &pmu_cap_wakeup->pmu_cap_core3_wakeup;
			break;
	}

	target_cpu_idx = MPIDR_AFFLVL1_VAL(mpidr) * PLATFORM_MAX_CPUS_PER_CLUSTER
			+ MPIDR_AFFLVL0_VAL(mpidr);

	writel(1 << target_cpu_idx, cpu_reset_base);
}

void spacemit_assert_cpu(u_register_t mpidr)
{
	unsigned int target_cpu_idx;
	unsigned int *cpu_assert_base = NULL;
	unsigned int *cluster_assert_base = NULL;

	target_cpu_idx = MPIDR_AFFLVL1_VAL(mpidr) * PLATFORM_MAX_CPUS_PER_CLUSTER
			+ MPIDR_AFFLVL0_VAL(mpidr);

	switch (target_cpu_idx) {
		case 0:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE0_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG0;
			break;
		case 1:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE1_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG1;
			break;
		case 2:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE2_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG2;
			break;
		case 3:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE3_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG3;
			break;
		case 4:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE4_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG0;
			break;
		case 5:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE5_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG1;
			break;
		case 6:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE6_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG2;
			break;
		case 7:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE7_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG3;
			break;
	}

	/* cpu vote */
	unsigned int value = readl(cpu_assert_base);
	value |= CPU_PWR_DOWN_VALUE;

	writel(value, cpu_assert_base);

	/* cluster vote */
	value = readl(cluster_assert_base);
	value |= CLUSTER_PWR_DOWN_VALUE;

	writel(value, cluster_assert_base);
}

void spacemit_clr_cpu_idle(void)
{
	unsigned int mpidr = current_hartid();

	/* clear the idle bit */
	unsigned int target_cpu_idx;
	unsigned int *cpu_assert_base = NULL;
	unsigned int *cluster_assert_base = NULL;

	target_cpu_idx = MPIDR_AFFLVL1_VAL(mpidr) * PLATFORM_MAX_CPUS_PER_CLUSTER
			+ MPIDR_AFFLVL0_VAL(mpidr);

	switch (target_cpu_idx) {
		case 0:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE0_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG0;
			break;
		case 1:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE1_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG1;
			break;
		case 2:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE2_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG2;
			break;
		case 3:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE3_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C0_CAPMP_IDLE_CFG3;
			break;
		case 4:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE4_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG0;
			break;
		case 5:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE5_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG1;
			break;
		case 6:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE6_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG2;
			break;
		case 7:
			cpu_assert_base = (unsigned int *)PMU_CAP_CORE7_IDLE_CFG;
			cluster_assert_base = (unsigned int *)PMU_C1_CAPMP_IDLE_CFG3;
			break;
	}

	/* de-vote cpu */
	unsigned int value = readl(cpu_assert_base);
	value &= ~CPU_PWR_DOWN_VALUE;

	writel(value, cpu_assert_base);

	/* de-vote cluster */
	value = readl(cluster_assert_base);
	value &= ~CLUSTER_PWR_DOWN_VALUE;

	writel(value, cluster_assert_base);
}
