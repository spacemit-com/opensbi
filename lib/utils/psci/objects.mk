#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

libsbiutils-objs-$(CONFIG_ARM_PSCI_SUPPORT) += psci/psci_common.o

libsbiutils-objs-$(CONFIG_ARM_PSCI_SUPPORT) += psci/psci_setup.o

libsbiutils-objs-$(CONFIG_ARM_PSCI_SUPPORT) += psci/psci_main.o

libsbiutils-objs-$(CONFIG_ARM_PSCI_SUPPORT) += psci/psci_on.o

libsbiutils-objs-$(CONFIG_ARM_PSCI_SUPPORT) += psci/psci_off.o

libsbiutils-objs-$(CONFIG_ARM_PSCI_SUPPORT) += psci/spacemit/spacemit_topology.o

ifeq ($(CONFIG_ARM_NON_SCMI_SUPPORT), y)
libsbiutils-objs-$(CONFIG_PLATFORM_SPACEMIT_K1X) += psci/spacemit/plat/k1x/plat_pm.o
endif
