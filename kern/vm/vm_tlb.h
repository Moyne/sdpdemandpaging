#include "../compile/TLB/opt-tlb.h"
#if OPT_TLB
#include <types.h>
#include <mips/tlb.h>
#include <lib.h>
void vmtlb_invalidate(void);
int vmtlb_write(vaddr_t addr,paddr_t paddr,bool writePriv);
#endif