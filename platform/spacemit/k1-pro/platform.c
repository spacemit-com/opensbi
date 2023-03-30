/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Space-T.
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>

/*
 * Include these files as needed.
 * See objects.mk PLATFORM_xxx configuration parameters.
 */
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

int qemu_mode;

static struct plic_data plic = {
    .addr = PLATFORM_PLIC_ADDR,
    .num_src = PLATFORM_PLIC_NUM_SOURCES,
};

static struct aclint_mswi_data mswi = {
    .addr = PLATFORM_ACLINT_MSWI_ADDR,
    .size = ACLINT_MSWI_SIZE,
    .first_hartid = 0,
    .hart_count = PLATFORM_HART_COUNT,
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
    .hart_count = PLATFORM_HART_COUNT,
    .has_64bit_mmio = TRUE,
};

unsigned long fw_platform_init(unsigned long arg0, unsigned long arg1,
                               unsigned long arg2, unsigned long arg3,
                               unsigned long arg4)
{
    const char *compatible;
    void *fdt = (void *)arg1;
    int root_offset, len;
    struct plic_data plic_data;
    unsigned long aclint_freq;
    uint64_t clint_addr;
    int rc;

    root_offset = fdt_path_offset(fdt, "/");
    if (root_offset >= 0) {
        compatible = fdt_getprop(fdt, root_offset, "compatible", &len);
        if (compatible &&
            (0 == sbi_strncmp(compatible, "riscv-virtio", 12)))
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

    /* Return original FDT pointer */
    return arg1;
}

static struct c910_regs_struct c910_regs;

static int c910_early_init(bool cold_boot)
{
    if (cold_boot) {
        if (!qemu_mode) {
            c910_regs.mcor = csr_read(CSR_MCOR);
            c910_regs.mhcr = csr_read(CSR_MHCR);
            c910_regs.mccr2 = csr_read(CSR_MCCR2);
            c910_regs.mhint = csr_read(CSR_MHINT);
            c910_regs.mxstatus = csr_read(CSR_MXSTATUS);
        }
    } else {
        if (!qemu_mode) {
            csr_write(CSR_MCOR, c910_regs.mcor);
            csr_write(CSR_MHCR, c910_regs.mhcr);
            csr_write(CSR_MHINT, c910_regs.mhint);
            csr_write(CSR_MXSTATUS, c910_regs.mxstatus);
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
    .timer_init = platform_timer_init
};

const struct sbi_platform platform = {
    .opensbi_version = OPENSBI_VERSION,
    .platform_version = SBI_PLATFORM_VERSION(0x0, 0x01),
    .name = "k1-pro",
    .features = SBI_PLATFORM_DEFAULT_FEATURES,
    .hart_count = PLATFORM_HART_COUNT,
    .hart_stack_size = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
    .platform_ops_addr = (unsigned long)&platform_ops
};
