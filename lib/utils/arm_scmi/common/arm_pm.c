/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sbi_utils/psci/psci.h>
#include <sbi_utils/psci/plat/arm/common/plat_arm.h>

/*******************************************************************************
 * The ARM Standard platform definition of platform porting API
 * `plat_setup_psci_ops`.
 ******************************************************************************/
int plat_setup_psci_ops(uintptr_t sec_entrypoint,
				const plat_psci_ops_t **psci_ops)
{
	*psci_ops = plat_arm_psci_override_pm_ops(&plat_arm_psci_pm_ops);

	return 0;
}
