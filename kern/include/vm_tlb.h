#include "opt-tlb.h"
#if OPT_TLB
#include <types.h>
#include <mips/tlb.h>
#include <lib.h>
void vmtlb_invalidatepage(pid_t pid,vaddr_t addr);
//invalidate tlb, done simply by removing entries when there is a context switch
void vmtlb_invalidate(pid_t pid);
//write on the TLB
int vmtlb_write(pid_t pid,vaddr_t addr,paddr_t paddr,bool writePriv);
#endif