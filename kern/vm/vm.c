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

void vm_bootstrap(void){
    mapinit();
    swapinit();
	vmstatsetup();
}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("bvm tried to do tlb shootdown?!\n");
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
		panic("vm: got VM_FAULT_READONLY\n");
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

	vbase1 = as->seg1->vaddr;
	vtop1 = vbase1 + as->seg1->numpages * PAGE_SIZE;
	vbase2 = as->seg2->vaddr;
	vtop2 = vbase2 + as->seg2->numpages * PAGE_SIZE;
	stackbase = USERSTACK - STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
	struct segment* seg=NULL;
	unsigned int ind=0;
	bool stack=false;
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
		stack=true;
	}
	else {
		return EFAULT;
	}

	/* make sure it's page-aligned */
	//KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	paddr=seg->pagetable->pages[ind].paddr;
	vmstatincr(TLBFAULT);
	if(paddr==INSWAPFILE || paddr==INELFFILE){
		paddr_t respaddr=alloc_user_page(faultaddress);
		if(paddr==INSWAPFILE){
			swapoutpage(seg->pagetable->pages[ind].swapoffset,respaddr);
			vmstatincr(PAGEFAULTDISK);
			vmstatincr(PAGEFAULTSWAP);
		}
		else if(paddr==INELFFILE && !stack){
			readfromelfto(as,faultaddress,respaddr);
			vmstatincr(PAGEFAULTDISK);
			vmstatincr(PAGEFAULTELF);
		}
		seg->pagetable->pages[ind].paddr=respaddr;
		seg->pagetable->pages[ind].swapoffset=0;
	}
	else vmstatincr(TLBRELOAD);
	if(vmtlb_write(faultaddress,seg->pagetable->pages[ind].paddr,seg->permissions&WRFLAG)==-1)	kprintf("vm: Ran out of TLB entries - cannot handle page fault\n");
	else {
		splx(spl);
		return 0;
	}
	splx(spl);
	return EFAULT;
}