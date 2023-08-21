#ifndef __UNDERLY_IMPLEMENT__H__
#define __UNDERLY_IMPLEMENT__H__

#include <sbi/sbi_types.h>

void spacemit_cluster_on(u_register_t mpidr);
void spacemit_cluster_off(u_register_t mpidr);
void spacemit_de_assert_cpu(u_register_t mpidr);
void spacemit_assert_cpu(u_register_t mpidr);
void spacemit_clr_cpu_idle(void);

#endif
