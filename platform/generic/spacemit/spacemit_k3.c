/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libfdt.h>
#include <platform_override.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_timer.h>
#include <sbi/sbi_trap_ldst.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <spacemit/spacemit_config.h>
#include <sbi_utils/cache/cache.h>
#include <sbi_utils/cci/cci.h>

/* Sv39 page-table walk constants */
#define SATP64_MODE_SHIFT	60
#define SV39_LEVELS		3
#define SV39_VPN_BITS		9
#define SV39_VPN_MASK		((1UL << SV39_VPN_BITS) - 1)
#define SV39_VPN2_SHIFT		30
#define SV39_VPN1_SHIFT		21
#define SV39_VPN0_SHIFT		PAGE_SHIFT
#define PTE_V			(1UL << 0)	/* valid */
#define PTE_RWX			(0xeUL)		/* leaf: R|W|X any set */
#define PTE_PPN_SHIFT		10

static const struct fdt_match spacemit_k3_mach[] = {
	{ .compatible = "spacemit,k3" },
	{ .compatible = "riscv-spacemit" },
	{ },
};

PLAT_CCI_MAP;
extern struct sbi_platform platform;
extern void _start_warm(unsigned long);
extern void _start_warm_dummy(unsigned long);

void boot_entry_dummy(unsigned long sc)
{
	unsigned int hartid = current_hartid();
	/* Only hart8 (C2) and hart12 (C3) enter the dummy path. */
	unsigned long boot_addr = hartid == 8 ? C2_RVBADDR_LO_ADDR :
					      C3_RVBADDR_LO_ADDR;

	/* set the vector load instructions to bypass L1 cache,only cached in the L2 cache */
	csr_set(CSR_PERF_CTRL, VEC_L1BYPASS);
	/* Increase the L2 prefetch distance to 56 entries */
	csr_set(CSR_PREFETCH_CTRL, L2_PERF_DIST);
	/* Turn off full address correlation check to improve L2 performance */
	csr_clear(CSR_ML2HINT, CIU_DEPD_DIS);
	csr_set(CSR_ML2HINT, CIU_MER_DIS);

	/* devote early */
	spacemit_devote_pwrdown_cluster(current_hartid());

	/* de-vote core acpr */
	spacemit_devote_core_apcr(current_hartid());

	/* Restore the normal warm-boot entry for this cluster. */
	writel((unsigned long)_start_warm & 0xffffffff, (void *)boot_addr);
	writel((unsigned long)_start_warm >> 32, (void *)(boot_addr + 4));

	spacemit_vote_powrdown_core(hartid);

	/* disable local timer */
	csr_write(CSR_STIMECMP, 0xffffffffffffffff);
	/* disable all irq */
	csr_clear(CSR_MIE, MIP_SSIP | MIP_MSIP | MIP_STIP | MIP_MTIP | MIP_SEIP | MIP_MEIP);
	/* disable prefetch */
	csi_disable_data_preftch();
	asm volatile ("fence iorw, iorw");
	/* flush dcache all */
	csi_flush_dcache_all();
	asm volatile ("fence iorw, iorw");
	/* disable i/d cache */
	csi_disable_cache();
	asm volatile ("fence iorw, iorw");
	/* disable core snoop */
	csr_clear(CSR_ML2SETUP, 1 << (current_hartid() % PLATFORM_MAX_CPUS_PER_CLUSTER));
	asm volatile ("fence iorw, iorw");

	while (1)
		wfi();
}

#define CPU_TO_CLUSTER(cpu)    ((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

static int spacemit_k3_early_init(bool cold_boot, const void *fdt, const struct fdt_match *match)
{
	int i, rc;
	unsigned int hartid;
	unsigned long cluster_id;
	struct sbi_scratch *scratch;

	if (cold_boot) {
		/* initiaze the cci */
		cci_init(PLATFORM_CCI_ADDR, cci_map, array_size(cci_map));

		/* set the bootv of each cluster */
		for (i = 0; i < platform.hart_count; i += PLATFORM_MAX_CPUS_PER_CLUSTER) {

			hartid = platform.hart_index2id[i];
			scratch = sbi_hartid_to_scratch(hartid);

			cluster_id = CPU_TO_CLUSTER(hartid);

                        switch (cluster_id) {
                        case 0:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C0_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C0_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C0_L2_FLUSH_CTRL);
				break;
                       case 1:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C1_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C1_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C1_L2_FLUSH_CTRL);
				break;
                       case 2:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C2_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C2_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C2_L2_FLUSH_CTRL);
				break;
                       case 3:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C3_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C3_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C3_L2_FLUSH_CTRL);
				break;
			default:
				break;
			}

		}

		/* enable the cci */
		cci_enable_snoop_dvm_reqs(0);
		cci_enable_snoop_dvm_reqs(1);
		cci_enable_snoop_dvm_reqs(2);
		cci_enable_snoop_dvm_reqs(3);
		cci_enable_snoop_dvm_reqs(4);
		cci_enable_snoop_dvm_reqs(5);
		cci_enable_snoop_dvm_reqs(6);

		for (i = 0; i < platform.hart_count; ++i)
			/* devote the cluster */
			spacemit_devote_pwrdown_cluster(i);

		/* Pre-warm C2/C3 so their CPU clock switches can complete. */
		writel(((unsigned long)_start_warm_dummy) & 0xffffffff, (unsigned int *)(C2_RVBADDR_LO_ADDR));
		writel((((unsigned long)_start_warm_dummy) >> 32) & 0xffffffff, (unsigned int*)(C2_RVBADDR_HI_ADDR));
		writel((1 << 8), (unsigned int *)PMU_CAP_CORE8_WAKEUP);

		writel((unsigned long)_start_warm_dummy & 0xffffffff,
		       (void *)C3_RVBADDR_LO_ADDR);
		writel((unsigned long)_start_warm_dummy >> 32,
		       (void *)C3_RVBADDR_HI_ADDR);
		writel(1 << 12, (void *)PMU_CAP_CORE12_WAKEUP);

		/* deassert dmasys reset for cpus reach all tcm range */
		writel(1, (unsigned int *)DMASYS_RESET);
		/* enable dmasys clk for cpus reach all tcm range */
		writel(1, (unsigned int *)DMASYS_CLK_EN);

		/*
		 * Protect reserved memory regions: no R/W/X for M/S/U modes.
		 * ENF_PERMISSIONS locks the PMP entry so M-mode is also denied.
		 * Protect the rcpu runtime environment from corruption
		 */
		rc = sbi_domain_root_add_memrange(RCPU0_RUNTIME_SPACE_BASE_ADDR, RCPU0_RUNTIME_SPACE_SIZE, RCPU0_MEMRANGE_GRAN,
						  SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS);
		if (rc)
			return rc;

		rc = sbi_domain_root_add_memrange(RCPU1_RUNTIME_SPACE_BASE_ADDR, RCPU1_RUNTIME_SPACE_SIZE, RCPU1_MEMRANGE_GRAN,
						  SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS);
		if (rc)
			return rc;

		rc = sbi_domain_root_add_memrange(RCPU_DTB_SPACE_BASE_ADDR, RCPU_DTB_SPACEMI_SIZE, RCPU_DTB_MEMRANGE_GRAN,
						  SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS);
		if (rc)
			return rc;

		rc = sbi_domain_root_add_memrange(REGISTER_PRESERVATION_BASE, REGISTER_PRESERVATION_SIZE, PAGE_SIZE,
						  SBI_DOMAIN_MEMREGION_M_RWX);
		if (rc)
			return rc;
	} else {
		unsigned int current_hartid = current_hartid();

		cluster_id = CPU_TO_CLUSTER(current_hartid);
	}

	return 0;
}


unsigned long hart_imisc_save_offset;

static int spacemit_k3_final_init(bool cold_boot, void *fdt, const struct fdt_match *match)
{
	if (cold_boot)
		hart_imisc_save_offset = sbi_scratch_alloc_offset(sizeof(struct imsic_config));

	return 0;
}

static bool spacemit_k3_cold_boot_allowed(u32 hartid, const struct fdt_match *match)
{
	/* enable core snoop ,iprf and tprf*/
	csr_set(CSR_ML2SETUP, 1 << (hartid % PLATFORM_MAX_CPUS_PER_CLUSTER) | IPRF | TPRF);

	if (hartid >= 8) {
		/* Set the vector load instructions to bypass L1 cache,only cached in the L2 cache */
		csr_set(CSR_PERF_CTRL, VEC_L1BYPASS);
		/* Increase the L2 prefetch distance to 56 entries */
		csr_set(CSR_PREFETCH_CTRL, L2_PERF_DIST);
		/* Turn off full address correlation check to improve L2 performance */
		csr_clear(CSR_ML2HINT, CIU_DEPD_DIS);
		/* Disbale read and prefetch transaction merge */
		csr_set(CSR_ML2HINT, CIU_MER_DIS);
	} else {
		/* Disable ifu fusion type5 */
		csr_set(CSR_MVMD1, IFU_FUSION_TYPE5_DIS);
	}

	/* enable the rvtrace clk by default */
	csr_set(CSR_ML2HINT, TRACE_TOP_ICGEN);

	/* devote early */
	spacemit_devote_pwrdown_cluster(hartid);

	/* de-vote core acpr */
	spacemit_devote_core_apcr(hartid);

	/* dealing with resuming process */
	if ((__sbi_hsm_hart_get_state(hartid) == SBI_HSM_STATE_SUSPENDED) && (hartid == 0))
		return false;

	return ((hartid == 0) ? true : false);
}

/*
 * Translate the address from stval to a physical address.
 * If S-mode MMU is off (satp.MODE == Bare), stval already is a physical address.
 * Otherwise walk the Sv39/Sv48 page tables, which M-mode can read directly.
 * Returns 0 on translation failure.
 */
static unsigned long s_addr_to_pa(unsigned long addr)
{
	unsigned long satp = csr_read(CSR_SATP);
	unsigned long mode = (satp & SATP64_MODE) >> SATP64_MODE_SHIFT;

	/* Bare mode: no translation, addr is already physical */
	if (mode == SATP_MODE_OFF)
		return addr;

	if (mode != SATP_MODE_SV39)
		return 0;

	unsigned long ppn = satp & SATP64_PPN;
	unsigned long vpn[SV39_LEVELS] = {
		(addr >> SV39_VPN2_SHIFT) & SV39_VPN_MASK,
		(addr >> SV39_VPN1_SHIFT) & SV39_VPN_MASK,
		(addr >> SV39_VPN0_SHIFT) & SV39_VPN_MASK,
	};

	for (int i = 0; i < SV39_LEVELS; i++) {
		unsigned long *ptep = (unsigned long *)((ppn << PAGE_SHIFT) + vpn[i] * sizeof(unsigned long));
		unsigned long pte = *ptep;

		if (!(pte & PTE_V))
			return 0; /* invalid PTE */

		ppn = (pte >> PTE_PPN_SHIFT) & SATP64_PPN;

		if (pte & PTE_RWX) { /* leaf PTE: R|W|X set */
			unsigned long pg_off_bits = PAGE_SHIFT + SV39_VPN_BITS * (2 - i);
			unsigned long offset_mask = (1UL << pg_off_bits) - 1;
			return (ppn << PAGE_SHIFT) | (addr & offset_mask);
		}
	}
	return 0;
}

/*
 * Registers within REGISTER_PRESERVATION that are strictly M-mode only.
 * S-mode accesses to these must NOT be emulated — return SBI_ENODEV so the
 * fault is redirected back to S-mode as a real access error.
 *
 * Covers: C0-C3 RVBADDR LO/HI, PMU_CAP_CORE*_WAKEUP,
 *         PMU_CAP_CORE*_IDLE_CFG, PMU_CX_CAPMP_IDLE_CFG*.
 */
struct addr_range {
	unsigned long base;
	unsigned long size;
};

static const struct addr_range m_only_ranges[] = {
	{ C0_RVBADDR_LO_ADDR, 2 * sizeof(u32) },
	{ C1_RVBADDR_LO_ADDR, 2 * sizeof(u32) },
	{ C2_RVBADDR_LO_ADDR, 2 * sizeof(u32) },
	{ C3_RVBADDR_LO_ADDR, 2 * sizeof(u32) },
	{ PMU_CX_CAPMP_IDLE_CFG1,  sizeof(u32) },		/* CFG1 */
	{ PMU_CX_CAPMP_IDLE_CFG0, 7 * sizeof(u32) },		/* CFG0, IDLE_CFG0/1, WAKEUP0-3 */
	{ PMU_CX_CAPMP_IDLE_CFG2, 2 * sizeof(u32) },		/* CFG2/3 */
	{ PMU_CAP_CORE2_IDLE_CFG, 2 * sizeof(u32) },		/* IDLE_CFG2/3 */
	{ PMU_CAP_CORE12_IDLE_CFG, 12 * sizeof(u32) },		/* IDLE_CFG12-15, CX_CFG12-15, WAKEUP12-15 */
	{ PMU_CAP_CORE4_IDLE_CFG,  12 * sizeof(u32) },		/* IDLE_CFG4-7, CX_CFG4-7, WAKEUP4-7 */
	{ PMU_CAP_CORE8_IDLE_CFG,  12 * sizeof(u32) },		/* IDLE_CFG8-11, CX_CFG8-11, WAKEUP8-11 */
};

static bool pa_is_m_only(unsigned long pa, int len)
{
	for (int i = 0; i < array_size(m_only_ranges); i++) {
		if (pa >= m_only_ranges[i].base &&
		    pa + len <= m_only_ranges[i].base + m_only_ranges[i].size)
			return true;
	}
	return false;
}

static int spacemit_k3_emulate_load(int rlen, unsigned long addr,
				    union sbi_ldst_data *out_val,
				    const struct fdt_match *match)
{
	unsigned long pa = s_addr_to_pa(addr);

	if (!pa || pa < REGISTER_PRESERVATION_BASE ||
	    pa + rlen > REGISTER_PRESERVATION_BASE + REGISTER_PRESERVATION_SIZE)
		return SBI_ENODEV;

	/* M-mode-only registers: refuse S-mode emulation */
	if (pa_is_m_only(pa, rlen))
		return SBI_ENODEV;

	/*
	 * Serialize the emulated MMIO access against any prior/next access.
	 * OpenSBI's readl()/writel() only emit "fence i,r" / "fence w,o", which
	 * do NOT order device-read before device-read (or device-write before
	 * device-read). The Spacemit PMU/APMU bus stalls on back-to-back
	 * unsynchronized M-mode reads (as issued by the kernel CCU driver's
	 * recalc_rate loop), so add a full I/O barrier on both sides of each
	 * emulated access.
	 */
	mb();
	switch (rlen) {
	case 1: out_val->data_bytes[0] = readb((volatile void *)pa); break;
	case 2: out_val->data_u32 = readw((volatile void *)pa); break;
	case 4: out_val->data_u32 = readl((volatile void *)pa); break;
	case 8: out_val->data_u64 = readq((volatile void *)pa); break;
	default: return SBI_EINVAL;
	}
	mb();
	return 0;
}

static int spacemit_k3_emulate_store(int wlen, unsigned long addr,
				     union sbi_ldst_data in_val,
				     const struct fdt_match *match)
{
	unsigned long pa = s_addr_to_pa(addr);

	if (!pa || pa < REGISTER_PRESERVATION_BASE ||
	    pa + wlen > REGISTER_PRESERVATION_BASE + REGISTER_PRESERVATION_SIZE)
		return SBI_ENODEV;

	/* M-mode-only registers: refuse S-mode emulation */
	if (pa_is_m_only(pa, wlen))
		return SBI_ENODEV;

	/* See comment in spacemit_k3_emulate_load(): full I/O barrier. */
	mb();
	switch (wlen) {
	case 1: writeb(in_val.data_bytes[0], (volatile void *)pa); break;
	case 2: writew(in_val.data_u32, (volatile void *)pa); break;
	case 4: writel(in_val.data_u32, (volatile void *)pa); break;
	case 8: writeq(in_val.data_u64, (volatile void *)pa); break;
	default: return SBI_EINVAL;
	}
	mb();
	return 0;
}

const struct platform_override spacemit_k3 = {
	.match_table = spacemit_k3_mach,
	.early_init = spacemit_k3_early_init,
	.final_init = spacemit_k3_final_init,
	.cold_boot_allowed = spacemit_k3_cold_boot_allowed,
	.emulate_load = spacemit_k3_emulate_load,
	.emulate_store = spacemit_k3_emulate_store,
};
