/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Space-T.
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_platform.h>

/*
 * Include these files as needed.
 * See objects.mk PLATFORM_xxx configuration parameters.
 */
#include <sbi_utils/cci/cci.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/ipi/aclint_mswi.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#include "platform.h"
#include <libfdt.h>

#define PLATFORM_PLIC_ADDR 0xB0000000
#define PLATFORM_PLIC_NUM_SOURCES 128
#define PLATFORM_HART_COUNT 4
#define PLATFORM_CLINT_ADDR 0xB4000000
#define PLATFORM_ACLINT_MTIMER_FREQ 25000000
#define PLATFORM_ACLINT_MSWI_ADDR (PLATFORM_CLINT_ADDR + \
                                   CLINT_MSWI_OFFSET)
#define PLATFORM_ACLINT_MTIMER_ADDR (PLATFORM_CLINT_ADDR + \
                                     CLINT_MTIMER_OFFSET)

extern struct sbi_platform platform;

static struct plic_data plic = {
    .addr = PLATFORM_PLIC_ADDR,
    .num_src = PLATFORM_PLIC_NUM_SOURCES,
};

static struct aclint_mswi_data mswi = {
    .addr = PLATFORM_ACLINT_MSWI_ADDR,
    .size = ACLINT_MSWI_SIZE,
    .first_hartid = 0,
    .hart_count = SBI_HARTMASK_MAX_BITS,
};

static struct aclint_mtimer_data mtimer = {
    .mtime_freq = PLATFORM_ACLINT_MTIMER_FREQ,
    .mtime_addr = PLATFORM_ACLINT_MTIMER_ADDR +
                  ACLINT_DEFAULT_MTIME_OFFSET,
    .mtime_size = ACLINT_DEFAULT_MTIME_SIZE,
    .mtimecmp_addr = PLATFORM_ACLINT_MTIMER_ADDR +
                     ACLINT_DEFAULT_MTIMECMP_OFFSET,
    .mtimecmp_size = ACLINT_DEFAULT_MTIMECMP_SIZE,
    .first_hartid = 0,
    .hart_count = SBI_HARTMASK_MAX_BITS,
    .has_64bit_mmio = TRUE,
};

static const int cci_map[] = {
    PLAT_CCI_CLUSTER0_IFACE_IX,
    PLAT_CCI_CLUSTER1_IFACE_IX,
    PLAT_CCI_CLUSTER2_IFACE_IX,
    PLAT_CCI_CLUSTER3_IFACE_IX,
};

static struct c910_regs_struct c910_regs;
static u32 generic_hart_index2id[SBI_HARTMASK_MAX_BITS] = {0};
static int qemu_mode;

u32 platform_hart_index(u32 hartid)
{
    u32 i;

    if (platform.hart_index2id) {
        for (i = 0; i < platform.hart_count; i++) {
            if (platform.hart_index2id[i] == hartid)
                return i;
        }
        return -1U;
    }

    return hartid;
}

unsigned long fw_platform_init(unsigned long arg0, unsigned long arg1,
                               unsigned long arg2, unsigned long arg3,
                               unsigned long arg4)
{
    const char *compatible;
    void *fdt = (void *)arg1;
    u32 hartid, hart_count = 0, clusterid, cluster_enabled = 0;
    int rc, root_offset, cpus_offset, cpu_offset, len;
    struct plic_data plic_data;
    unsigned long aclint_freq;
    uint64_t clint_addr;

    root_offset = fdt_path_offset(fdt, "/");
    if (root_offset >= 0) {
        compatible = fdt_getprop(fdt, root_offset, "compatible", &len);
        if (compatible && (0 == sbi_strncmp(compatible, "riscv-virtio", 12)))
            qemu_mode = 1;
    }

    rc = fdt_parse_plic(fdt, &plic_data, "riscv,plic0");
    if (!rc)
        plic = plic_data;

    rc = fdt_parse_timebase_frequency(fdt, &aclint_freq);
    if (!rc)
        mtimer.mtime_freq = aclint_freq;

    rc = fdt_parse_compat_addr(fdt, &clint_addr, "riscv,clint0");
    if (!rc) {
        mswi.addr = clint_addr;
        mtimer.mtime_addr = clint_addr + CLINT_MTIMER_OFFSET +
                            ACLINT_DEFAULT_MTIME_OFFSET;
        mtimer.mtimecmp_addr = clint_addr + CLINT_MTIMER_OFFSET +
                               ACLINT_DEFAULT_MTIMECMP_OFFSET;
    }

    cpus_offset = fdt_path_offset(fdt, "/cpus");
    if (cpus_offset < 0)
        sbi_hart_hang();

    /* initiate cci */
    cci_init(PLATFORM_CCI_ADDR, cci_map, array_size(cci_map));

    fdt_for_each_subnode(cpu_offset, fdt, cpus_offset)
    {
        rc = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
        if (rc)
            continue;

        if (SBI_HARTMASK_MAX_BITS <= hartid)
            continue;

        if (!fdt_node_is_enabled(fdt, cpu_offset))
            continue;

        generic_hart_index2id[hart_count++] = hartid;

        clusterid = (hartid & CLUSTER_ID_MASK) >> CLUSTER_ID_BITSHIFT;
        if (!qemu_mode && (0 == (cluster_enabled & (1 << clusterid)))) {
            /* enable cci for current cluster */
            cci_enable_snoop_dvm_reqs(clusterid);
            cluster_enabled |= 1 << clusterid;
        }
    }

    platform.hart_count = hart_count;
    mswi.hart_count = hartid + 1;
    mtimer.hart_count = hartid + 1;

    // mswi.hart2index = platform_hart_index;
    // mtimer.hart2index = platform_hart_index;

    /* Return original FDT pointer */
    return arg1;
}

static struct c910_regs_struct c910_regs;

static void cache_enable(void)
{
    // change DDR delay counter
    // write32(0x1001200C, 200);
    /* disable theadisaee and MAEE */
    csr_clear(CSR_MXSTATUS, 1 << 22 | 1 << 21);
    // invalid all memory for IBP,BTB,BHT,DCACHE,ICACHE
    // csr_set(CSR_MCOR, 0x70013);
    // enable lbuf,way_pred,data_cache_prefetch,amr
    csr_set(CSR_MHCR, 0x11ff);
    // csr_set(CSR_MHINT, 0x6e30c);
    // enable l2cache TLB prefetch
    csr_set(CSR_MCCR2, 0xe0000009);
}

static void wakeup_other_core(void)
{
    int i;
    u32 hartid, clusterid, coreid;
    u32 *cpu_reset_reg;

    // hart0 is already boot up
    for (i = 1; i < platform.hart_count; i++) {
        hartid = generic_hart_index2id[i];

        // cluster0 had release reset
        clusterid = (hartid & CLUSTER_ID_MASK) >> CLUSTER_ID_BITSHIFT;
        coreid = (hartid & CORE_ID_MASK) >> CORE_ID_BITSHIFT;

        cpu_reset_reg = (u32 *)CPU_RESET_BASE_ADDR + clusterid;
        if ((clusterid > 0) && (0x0F != (readl(cpu_reset_reg) & 0x0F))) {
            // de-assert cluster 1 APB, L2C, PIC, ACEM/LLP
            writel(0x0F, cpu_reset_reg);
        }

        writel(readl(cpu_reset_reg) | 1 << (coreid + 4), cpu_reset_reg);
    }
}

static int c910_early_init(bool cold_boot)
{
    if (cold_boot) {
        if (!qemu_mode) {
            cache_enable();

            c910_regs.mcor = csr_read(CSR_MCOR);
            c910_regs.mhcr = csr_read(CSR_MHCR);
            c910_regs.mccr2 = csr_read(CSR_MCCR2);
            c910_regs.mhint = csr_read(CSR_MHINT);
            c910_regs.mxstatus = csr_read(CSR_MXSTATUS);

            wakeup_other_core();
        }
    } else {
        if (!qemu_mode) {
            cache_enable();
        }
    }

    return 0;
}

/*
 * Platform early initialization.
 */
static int platform_early_init(bool cold_boot)
{
    int ret = 0;

    ret = c910_early_init(cold_boot);

    return ret;
}

/*
 * Platform final initialization.
 */
static int platform_final_init(bool cold_boot)
{
    return 0;
}

/*
 * Initialize the platform interrupt controller for current HART.
 */
static int platform_irqchip_init(bool cold_boot)
{
    /* Delegate plic enable into S-mode */
    writel(C910_PLIC_DELEG_ENABLE,
           (void *)plic.addr + C910_PLIC_DELEG_OFFSET);

    return 0;
}

/*
 * Initialize IPI for current HART.
 */
static int platform_ipi_init(bool cold_boot)
{
    int ret;

    /* Example if the generic ACLINT driver is used */
    if (cold_boot) {
        ret = aclint_mswi_cold_init(&mswi);
        if (ret)
            return ret;
    }

    return aclint_mswi_warm_init();
}

/*
 * Initialize platform timer for current HART.
 */
static int platform_timer_init(bool cold_boot)
{
    int ret;

    /* Example if the generic ACLINT driver is used */
    if (cold_boot) {
        ret = aclint_mtimer_cold_init(&mtimer, NULL);
        if (ret)
            return ret;
    }

    return aclint_mtimer_warm_init();
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
    .early_init = platform_early_init,
    .final_init = platform_final_init,
    .console_init = fdt_serial_init,
    .irqchip_init = platform_irqchip_init,
    .ipi_init = platform_ipi_init,
    .timer_init = platform_timer_init};

struct sbi_platform platform = {
    .opensbi_version = OPENSBI_VERSION,
    .platform_version = SBI_PLATFORM_VERSION(0x0, 0x01),
    .name = "k1-pro",
    .features = SBI_PLATFORM_DEFAULT_FEATURES,
    .hart_count = SBI_HARTMASK_MAX_BITS,
    .hart_index2id = generic_hart_index2id,
    .hart_stack_size = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
    .platform_ops_addr = (unsigned long)&platform_ops};
