#include "opt-tlb.h"
#if OPT_TLB
#include <types.h>
#include <mips/tlb.h>
#include <lib.h>
//invalidate tlb, done simply by removing entries when there is a context switch
void vmtlb_invalidate(void);
//write on the TLB
int vmtlb_write(vaddr_t addr,paddr_t paddr,bool writePriv);
#endif