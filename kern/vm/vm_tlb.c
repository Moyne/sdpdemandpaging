#include <vm_tlb.h>
#include <current.h>
#include <proc.h>
#include <vmstats.h>
//tlb writes will work in the following way:
//The tlb will work as a circular buffer, I use two variables just to have a simpler management of statistics
static unsigned int tlbvictim=0;
static unsigned int tlbindex=0;
void vmtlb_invalidate(void){
    for(unsigned int i=0;i<NUM_TLB;i++)	tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    tlbvictim=0;
    tlbindex=0;
    vmstatincr(TLBINVALIDATION);
}
int vmtlb_write(vaddr_t addr,paddr_t paddr,bool writePriv){
    uint32_t lo,hi;
    //uint32_t pid=curproc->p_pid<<6;
    hi=(addr&TLBHI_VPAGE);//|(pid&TLBHI_PID);
    lo=(paddr&TLBLO_PPAGE)|TLBLO_VALID|TLBLO_GLOBAL;
    if(writePriv) lo|=TLBLO_DIRTY;
    if(!(lo & TLBLO_VALID)){
        kprintf("Writing INVALID entry");
    }
    if(tlbindex<NUM_TLB){
        tlb_write(hi,lo,tlbindex);
        tlbindex++;
        vmstatincr(TLFAULTWITHFREE);
    }
    else{
        tlb_write(hi,lo,tlbvictim);
        tlbvictim=(tlbvictim+1)%NUM_TLB;
        vmstatincr(TLBFAULTWITHREPLACE);
    }
    return 0;
}