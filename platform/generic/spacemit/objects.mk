#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_SPACEMIT_K1PRO) += spacemit_k1pro
platform-objs-$(CONFIG_PLATFORM_SPACEMIT_K1PRO) += spacemit/k1pro.o

carray-platform_override_modules-$(CONFIG_PLATFORM_SPACEMIT_K1X) += spacemit_k1x
platform-objs-$(CONFIG_PLATFORM_SPACEMIT_K1X) += spacemit/k1x.o
