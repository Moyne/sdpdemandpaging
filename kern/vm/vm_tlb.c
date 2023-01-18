#include <vm_tlb.h>
#include <current.h>
#include <proc.h>
#include <vmstats.h>
//tlb writes will work in the following way:
//The tlb will work as a circular buffer, I use two variables just to have a simpler management of statistics
static unsigned int tlbindex=0;
static int tlbinvalidatedindex=-1;
static pid_t lastpid=(pid_t) 0;
void vmtlb_invalidatepage(pid_t pid,vaddr_t addr){
    (void)pid;
    int ind=tlb_probe(addr&TLBHI_VPAGE,0);
    if(ind>=0){
        tlb_write(TLBHI_INVALID(ind), TLBLO_INVALID(), ind);
        tlbinvalidatedindex=ind;
    }
}
void vmtlb_invalidate(pid_t pid){
    if(pid==lastpid) return;
    for(unsigned int i=0;i<NUM_TLB;i++) tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    tlbinvalidatedindex=-1;
    tlbindex=0;
    lastpid=(pid_t) 0;
    vmstatincr(TLBINVALIDATION);
}
int vmtlb_write(pid_t pid,vaddr_t addr,paddr_t paddr,bool writePriv){
    uint32_t lo,hi;
    lastpid=pid;
    hi=(addr&TLBHI_VPAGE);//|(pid&TLBHI_PID);
    lo=(paddr&TLBLO_PPAGE)|TLBLO_VALID|TLBLO_GLOBAL;
    if(writePriv) lo|=TLBLO_DIRTY;
    if(!(lo & TLBLO_VALID)){
        panic("Writing INVALID entry");
        return -1;
    }
    if(tlbinvalidatedindex>=0){
        tlb_write(hi,lo,tlbinvalidatedindex);
        tlbinvalidatedindex=-1;
        vmstatincr(tlbindex<NUM_TLB?TLBFAULTWITHFREE:TLBFAULTWITHREPLACE);
    }
    else if(tlbindex<NUM_TLB){
        tlb_write(hi,lo,tlbindex);
        tlbindex++;
        vmstatincr(TLBFAULTWITHFREE);
    }
    else{
        tlb_write(hi,lo,tlbindex%NUM_TLB);
        tlbindex++;
        vmstatincr(TLBFAULTWITHREPLACE);
    }
    return 0;
}