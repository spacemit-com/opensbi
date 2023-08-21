#include <sbi/riscv_io.h>
#include <sbi/sbi_types.h>
#include <sbi/riscv_asm.h>
#include <sbi_utils/psci/psci.h>
#include <sbi/sbi_console.h>
#include <spacemit/spacemit_config.h>

#define C1_CPU_RESET_BASE_ADDR		(0xD4282B24)

#define PMU_CAP_IDLE_CFG_BASE_ADDR	(0xd4282924)

#define CPU_PWR_DOWN_VALUE		(0x3)

struct pmu_cap_wakeup {
	unsigned int pmu_cap_core0_wakeup;
	unsigned int pmu_cap_core1_wakeup;
	unsigned int pmu_cap_core2_wakeup;
	unsigned int pmu_cap_core3_wakeup;
};

struct pmu_cap_idle {
	unsigned int pmu_cap_core0_idle_cfg; /* 0x124 */
	unsigned int pmu_cap_core1_idle_cfg; /* 0x128 */
	unsigned int reserved0[13]; /* 0x12c 0x130 0x134 0x138 0x13c 0x140 0x144 0x148 0x14c 0x150 0x154 0x158 0x15c */
	unsigned int pmu_cap_core2_idle_cfg; /* 0x160 */
	unsigned int pmu_cap_core3_idle_cfg; /* 0x164 */
	unsigned int reserved1[103];
	unsigned int pmu_cap_core4_idle_cfg; /* 0x304 */
	unsigned int pmu_cap_core5_idle_cfg; /* 0x308 */
	unsigned int pmu_cap_core6_idle_cfg; /* 0x30c */
	unsigned int pmu_cap_core7_idle_cfg; /* 0x310 */
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
	struct pmu_cap_idle *pmu_cap_idle;

	pmu_cap_idle = (struct pmu_cap_idle *)PMU_CAP_IDLE_CFG_BASE_ADDR;

	target_cpu_idx = MPIDR_AFFLVL1_VAL(mpidr) * PLATFORM_MAX_CPUS_PER_CLUSTER
			+ MPIDR_AFFLVL0_VAL(mpidr);

	switch (target_cpu_idx) {
		case 0:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core0_idle_cfg;
			break;
		case 1:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core1_idle_cfg;
			break;
		case 2:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core2_idle_cfg;
			break;
		case 3:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core3_idle_cfg;
			break;
		case 4:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core4_idle_cfg;
			break;
		case 5:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core5_idle_cfg;
			break;
		case 6:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core6_idle_cfg;
			break;
		case 7:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core7_idle_cfg;
			break;
	}

	unsigned int value = readl(cpu_assert_base);
	value |= CPU_PWR_DOWN_VALUE;

	writel(value, cpu_assert_base);
}

void spacemit_clr_cpu_idle(void)
{
	unsigned int mpidr = current_hartid();

	/* clear the idle bit */
	unsigned int target_cpu_idx;
	unsigned int *cpu_assert_base = NULL;
	struct pmu_cap_idle *pmu_cap_idle;

	pmu_cap_idle = (struct pmu_cap_idle *)PMU_CAP_IDLE_CFG_BASE_ADDR;

	target_cpu_idx = MPIDR_AFFLVL1_VAL(mpidr) * PLATFORM_MAX_CPUS_PER_CLUSTER
			+ MPIDR_AFFLVL0_VAL(mpidr);

	switch (target_cpu_idx) {
		case 0:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core0_idle_cfg;
			break;
		case 1:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core1_idle_cfg;
			break;
		case 2:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core2_idle_cfg;
			break;
		case 3:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core3_idle_cfg;
			break;
		case 4:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core4_idle_cfg;
			break;
		case 5:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core5_idle_cfg;
			break;
		case 6:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core6_idle_cfg;
			break;
		case 7:
			cpu_assert_base = &pmu_cap_idle->pmu_cap_core7_idle_cfg;
			break;
	}

	unsigned int value = readl(cpu_assert_base);
	value &= ~CPU_PWR_DOWN_VALUE;

	writel(value, cpu_assert_base);
}
