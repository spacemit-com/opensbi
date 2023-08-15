/*
 * Copyright (c) 2018-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sbi_utils/psci/drivers/arm/css/scmi.h>
#include <sbi_utils/psci/psci.h>
#include <sbi_utils/psci/plat/arm/common/plat_arm.h>
#include <sbi_utils/psci/drivers/arm/css/scmi.h>
#include <sbi_utils/psci/drivers/arm/css/css_mhu_doorbell.h>

const plat_psci_ops_t *plat_arm_psci_override_pm_ops(plat_psci_ops_t *ops)
{
	return css_scmi_override_pm_ops(ops);
}

static scmi_channel_plat_info_t juno_scmi_plat_info = {
	.scmi_mbx_mem = 0x2f902080,
	.db_reg_addr = 0x2f824000,
	/* no used */
	.db_preserve_mask = 0xfffffffe,
	/* no used */
	.db_modify_mask = 0x1,
	.ring_doorbell = &mhu_ring_doorbell,
};

scmi_channel_plat_info_t *plat_css_get_scmi_info(unsigned int channel_id)
{
        return &juno_scmi_plat_info;
}

/*
 * The array mapping platform core position (implemented by plat_my_core_pos())
 * to the SCMI power domain ID implemented by SCP.
 */
const uint32_t plat_css_core_pos_to_scmi_dmn_id_map[PLATFORM_CORE_COUNT] = {
                        0, 1, 2, 3 };

