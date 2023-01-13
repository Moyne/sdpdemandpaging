#include <vm_tlb.h>
#include <current.h>
#include <proc.h>
//tlb writes will work in the following way:
//The tlb will work as a circular buffer, I use two variables just to have a simpler management of statistics
static unsigned int tlbvictim=0;
static unsigned int tlbindex=0;
void vmtlb_invalidate(void){
    for(unsigned int i=0;i<NUM_TLB;i++)	tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    tlbvictim=0;
    tlbindex=0;
}
int vmtlb_write(vaddr_t addr,paddr_t paddr,bool writePriv){
    uint32_t lo,hi;
    if(tlbindex==0){
        for(int i=0;i<NUM_TLB;i++){
            tlb_read(&hi,&lo,i);
            if(lo & TLBLO_VALID) continue;
            else{
                tlbindex=i;
                break;
            }
        }
    }
    /*if(tlbindex){
        tlb_read(&hi,&lo,tlbindex-1);
        int oldas=hi&TLBHI_PID;
        uint32_t oldpid= curproc->p_pid<<6;
        if((curproc->p_pid<<6)!=oldas){
            panic("This address space isn't the same that is present in the old TLB entries");
            vmtlb_invalidate();
            return -1;
        }
        else tlb_write(hi,lo,tlbindex-1);
        oldpid++;
    }*/
    //uint32_t pid=curproc->p_pid<<6;
    hi=(addr&TLBHI_VPAGE);//|(pid&TLBHI_PID);
    lo=(paddr&TLBLO_PPAGE)|TLBLO_VALID;
    if(writePriv) lo|=TLBLO_DIRTY;//| TLBLO_GLOBAL;
    if(!(lo & TLBLO_VALID)){
        kprintf("Writing INVALID entry");
    }
    if(tlbindex<NUM_TLB){
        tlb_write(hi,lo,tlbindex);
        /*uint32_t ah,al;
        tlb_read(&ah,&al,tlbindex);
        if(ah!=hi || al!=lo){
            panic("Couldn't write good tlb entry");
        }
        tlb_write(hi,lo,tlbindex);*/
        tlbindex++;
    }
    else{
        tlb_write(hi,lo,tlbvictim);
        /*uint32_t ah,al;
        tlb_read(&ah,&al,tlbvictim);
        if(ah!=hi || al!=lo){
            panic("Couldn't write good tlb entry");
        }
        tlb_write(hi,lo,tlbvictim);*/
        tlbvictim=(tlbvictim+1)%NUM_TLB;
    }
    return 0;
}