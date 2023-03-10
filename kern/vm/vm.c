#include "opt-paging.h"
#if OPT_PAGING

#include <vm.h>
#include <types.h>
#include <kern/errno.h>
#include <current.h>
#include <cpu.h>
#include <proc.h>
#include <spl.h>
#include <mips/tlb.h>
#include <coremap.h>
#include <vm_tlb.h>
#include <swapfile.h>
#include <vmstats.h>
#include <pt.h>
#include "../syscall/proc_syscalls.h"
void vm_bootstrap(void){
    ptinit();
    swapinit();
	vmstatsetup();
}

void vm_shutdown(void){
#if OPT_PAGING
	swapdest();
#endif
	ptdest();
#if OPT_PAGING
	printvmstats(0,NULL);
#endif
}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("vm tried to do tlb shootdown?!\n");
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	struct addrspace *as;
	int spl;

	faultaddress &= PAGE_FRAME;

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		sys__exit(VM_FAULT_READONLY);
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->seg1 != NULL);
	KASSERT(as->seg1->numpages != 0);
	KASSERT(as->seg2 != NULL);
	KASSERT(as->seg2->numpages != 0);
	KASSERT(as->segstack != NULL);
	KASSERT(as->segstack->numpages != 0);

	vbase1 = as->seg1->startaddr;
	vtop1 = vbase1 + as->seg1->numpages * PAGE_SIZE;
	vbase2 = as->seg2->startaddr;
	vtop2 = vbase2 + as->seg2->numpages * PAGE_SIZE;
	stackbase = USERSTACK - STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
	struct segment* seg=NULL;
	unsigned int ind=0;
	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		seg=as->seg1;
		ind=(faultaddress-vbase1)/PAGE_SIZE;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		seg=as->seg2;
		ind=(faultaddress-vbase2)/PAGE_SIZE;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		seg=as->segstack;
		ind=(faultaddress-stackbase)/PAGE_SIZE;
	}
	else {
		return EFAULT;
	}


	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();
	vmstatincr(TLBFAULT);
	paddr=pageaddr(curproc->p_pid,faultaddress);
	if(paddr==0){
		paddr_t respaddr=allocuserpage(curproc->p_pid,faultaddress);
		if(seg->loadedelf[ind]==1){
			swapoutpage(curproc->p_pid,faultaddress,respaddr);
			vmstatincr(PAGEFAULTDISK);
			vmstatincr(PAGEFAULTSWAP);
		}
		else if(!seg->stackseg && seg->loadedelf[ind]==0){
			readfromelfto(as,faultaddress,respaddr);
			seg->loadedelf[ind]=1;
			vmstatincr(PAGEFAULTDISK);
			vmstatincr(PAGEFAULTELF);
		}
		else if(seg->stackseg && seg->loadedelf[ind]==0){
			as_zero_region(respaddr,PAGE_SIZE);
			seg->loadedelf[ind]=1;
			vmstatincr(PAGEFAULTZERO);
		}
		paddr=respaddr;
	}
	else	vmstatincr(TLBRELOAD);
	if(vmtlb_write(curproc->p_pid,faultaddress,paddr,seg->permissions & WRFLAG)==-1)	panic("vm: error while writing in tlb\n");
	else {
		splx(spl);
		return 0;
	}
	splx(spl);
	return EFAULT;
}
#endif