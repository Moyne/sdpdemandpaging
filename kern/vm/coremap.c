#include <coremap.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <vm.h>
#include "../../../kern/vm/vm_tlb.h"
unsigned long int* bitmap;
paddr_t* map;
//unsigned int* sizeMap;
int totalPages;
int isAct=0;
/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 *
 * NOTE: it's been found over the years that students often begin on
 * the VM assignment by copying dumbvm.c and trying to improve it.
 * This is not recommended. dumbvm is (more or less intentionally) not
 * a good design reference. The first recommendation would be: do not
 * look at dumbvm at all. The second recommendation would be: if you
 * do, be sure to review it from the perspective of comparing it to
 * what a VM system is supposed to do, and understanding what corners
 * it's cutting (there are many) and why, and more importantly, how.
 */

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define VM_STACKPAGES    18

/*
 * Wrap ram_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static struct spinlock freemem_lock= SPINLOCK_INITIALIZER;
static int isActive(){
	int active;
	spinlock_acquire(&freemem_lock);
	active=isAct;
	spinlock_release(&freemem_lock);
	return active;
}
void
vm_bootstrap(void)
{
	totalPages=(int)ram_getsize()/PAGE_SIZE;
	bitmap=kmalloc(bitLen*sizeof(unsigned long int));
    map=kmalloc(totalPages*sizeof(struct page));
	if(map==NULL) return;
	int firstFree=ram_getfirstfree()/PAGE_SIZE;
	for(int i=0;i<firstFreei++){
        map[i].type=KERNEL;
        map[i].numpages=1;
        map[i].fifonext=NOTINIT;
        map[i].as=NULL;
    }
	for(int i=firstFree;i<totalPages;i++){
		map[i].type=FREE;
        map[i].numpages=0;
        map[i].fifonext=NOTINIT;
        map[i].as=NULL;
	}
	spinlock_acquire(&freemem_lock);
	isAct=1;
	spinlock_release(&freemem_lock);
}

/*
 * Check if we're in a context that can sleep. While most of the
 * operations in dumbvm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */
static
void
vm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

static paddr_t getfreeppages(unsigned long int npages,char type,struct addrspace* as,vaddr_t vaddr){
	if(!isActive()) return 0;
	spinlock_acquire(&freemem_lock);
	for(int i=0;i<totalPages;i++){
		if(map[i].type==FREE){
            if(start>0 && i-start>=npages-1){
                spinlock_release(&freemem_lock);
                for(int j=i;j>=start;j--){
                    map[j].type=type;
                    map[j].numpages=npages;
                    map[j].fifonext=NOTINIT;
                    map[j].as=as;
                }
                if(type==USER){
                    if(lastuserallocated!=NOTINIT)  map[lastuserallocated].next=i;
                    if(swapvictim==NOTINIT) swapvictim=i;
                    lastuserallocated=i;
                }
                return (paddr_t) PAGE_SIZE*i;
            }
            else if(start<0) start=i;
        }
        else i+=map[i].numpages;
	}
	spinlock_release(&freemem_lock);
	return 0;
}
static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;
	addr= getfreeppages(npages,KERNEL,NULL);
	if(addr!=0) return addr;
	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);

	spinlock_release(&stealmem_lock);
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;

	vm_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
	if(!isActive()) return;
	unsigned long int* bitmap_t=bitmap;
	paddr_t paddr=addr-MIPS_KSEG0;
	int i=paddr/PAGE_SIZE;
	spinlock_acquire(&freemem_lock);
	unsigned int numpages=map[i].numpages;
    for(int j=i;j<i+numpages;j++){
        map[j].type=FREE;
        map[j].numpages=0;
        map[j].as=NULL;
        map[j].fifonext=NOTINIT;
    }
	spinlock_release(&freemem_lock);
}

paddr_t alloc_user_page(vaddr_t vaddr){
    struct addrspace* as=proc_getas();
    paddr_t res=getfreeppages(1,USER,as,vaddr);
    if(res!=0){
        //found physical frame available
        return res;
    }
    else{
        //swapping needed
        if(swapvictim!=NOTINIT) panic("Don't have any user process to swap out!");
        unsigned int newvict=map[swapvictim].fifonext;
        lastallocateduser=swapvictim;
        
    }
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("bvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	struct addrspace *as;
	int spl;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "vm: fault: 0x%x\n", faultaddress);

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
	/*KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);*/

	vbase1 = as->seg1->vaddr;
	vtop1 = vbase1 + as->seg1->numpages * PAGE_SIZE;
	vbase2 = as->seg2->vaddr;
	vtop2 = vbase2 + as->seg2->numpages * PAGE_SIZE;
	stackbase = USERSTACK - VM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
	bool readonly=false;
	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
		readonly=as->as_readOnlyBase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
		readonly=as->as_readOnlyBase1;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
		readonly=as->as_readOnlyBase1;
	}
	else {
		return EFAULT;
	}

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	/*for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}*/
	if(vmtlb_write(faultaddress,paddr,!readonly)==-1)	kprintf("vm: Ran out of TLB entries - cannot handle page fault\n");
	else {
		splx(spl);
		return 0;
	}
	splx(spl);
	return EFAULT;
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;
	/*My bad machine dependant code, move all this into as*/
	as->as_readOnlyBase1=false;
	as->as_readOnlyBase2=false;
	as->as_readOnlyBase3=false;
	return as;
}

void
as_destroy(struct addrspace *as)
{
	vm_can_sleep();
	kfree(as);
}

void
as_activate(void)
{
	int spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	vmtlb_invalidate();

	splx(spl);
}

void
as_deactivate(void)
{
	/* nothing */
}


static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int
as_prepare_load(struct addrspace *as)
{
	KASSERT(as->as_pbase1 == 0);
	KASSERT(as->as_pbase2 == 0);
	KASSERT(as->as_stackpbase == 0);

	vm_can_sleep();

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(VM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}

	as_zero_region(as->as_pbase1, as->as_npages1);
	as_zero_region(as->as_pbase2, as->as_npages2);
	as_zero_region(as->as_stackpbase, VM_STACKPAGES);

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	vm_can_sleep();
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	vm_can_sleep();

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
	/*My bad machine dependant code, move all this into as*/
	new->as_readOnlyBase1 = old->as_readOnlyBase1;
	new->as_readOnlyBase2 = old->as_readOnlyBase2;
	new->as_readOnlyBase3 = old->as_readOnlyBase3;
	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT(new->as_pbase1 != 0);
	KASSERT(new->as_pbase2 != 0);
	KASSERT(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		VM_STACKPAGES*PAGE_SIZE);

	*ret = new;
	return 0;
}
