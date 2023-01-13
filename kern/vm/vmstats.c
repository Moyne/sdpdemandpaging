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
    spinlock_release(&statspin);
    return 0;
}