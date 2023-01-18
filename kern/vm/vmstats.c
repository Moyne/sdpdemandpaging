#include "opt-paging.h"
#if OPT_PAGING

#include <vmstats.h>
#include <spinlock.h>
#include <lib.h>
unsigned int counter[VMSTATSLEN];
struct spinlock statspin=SPINLOCK_INITIALIZER;
const char* VMSTATNAME[]={
    "Tlb faults",
    "Tlb faults with free",
    "Tlb faults with replace",
    "Tlb invalidations",
    "Tlb reloads",
    "Page faults (Zeroed)",
    "Page faults (Disk)",
    "Page faults from ELF",
    "Page faults from swapfile",
    "Swapfile writes"};
void vmstatsetup(void){
    spinlock_acquire(&statspin);
    for(int i=0;i<VMSTATSLEN;i++) counter[i]=0;
    spinlock_release(&statspin);
}

void vmstatincr(unsigned int type){
    spinlock_acquire(&statspin);
    counter[type]++;
    spinlock_release(&statspin);
}

int printvmstats(int nargs, char **args){
    (void)nargs;
	(void)args;
    spinlock_acquire(&statspin);
    for(int i=0;i<VMSTATSLEN;i++) kprintf("%s = %d\n",VMSTATNAME[i],counter[i]);
    if(counter[TLBFAULT]!=(counter[TLBFAULTWITHFREE]+counter[TLBFAULTWITHREPLACE]))  kprintf("WARNING:    The sum between the amount of TLB faults with free and TLB faults with replace should be equal to the total amount of TLB faults\n");
    if(counter[TLBFAULT]!=(counter[TLBFAULTWITHFREE]+counter[TLBFAULTWITHREPLACE]))  kprintf("WARNING:    The sum between the amount of TLB reloads, page faults from disk and page faults with zeroing should be equal to the total amount of TLB faults\n");
    if(counter[TLBFAULT]!=(counter[TLBFAULTWITHFREE]+counter[TLBFAULTWITHREPLACE]))  kprintf("WARNING:    The sum between the amount of page faults from ELF and page faults from the swapfile should be equal to the total amount of page faults from disk\n");
    spinlock_release(&statspin);
    return 0;
}

#endif