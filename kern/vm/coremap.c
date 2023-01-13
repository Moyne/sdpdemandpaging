#include <coremap.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include "../../../kern/vm/vm_tlb.h"
#include <swapfile.h>
#include <vm.h>
unsigned long int* bitmap;
struct page* map;
static unsigned int lastuserallocated=NOTINIT;
static unsigned int swapvictim=NOTINIT;
unsigned int totalPages;
int isAct=0;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static struct spinlock freemem_lock= SPINLOCK_INITIALIZER;

int isActive(void){
	int active;
	spinlock_acquire(&freemem_lock);
	active=isAct;
	spinlock_release(&freemem_lock);
	return active;
}
void mapinit(void)
{
	totalPages=(unsigned int)ram_getsize()/PAGE_SIZE;
    map=kmalloc(totalPages*sizeof(struct page));
	if(map==NULL) return;
	unsigned int firstFree=ram_getfirstfree()/PAGE_SIZE; // if you want to try swapfile: totalPages-4;
	for(unsigned int i=0;i<firstFree;i++){
        map[i].type=KERNEL;
        map[i].numpages=1;
        map[i].fifonext=NOTINIT;
        map[i].as=NULL;
		map[i].locked=true;
    }
	for(unsigned int i=firstFree;i<totalPages;i++){
		map[i].type=FREE;
        map[i].numpages=0;
        map[i].fifonext=NOTINIT;
        map[i].as=NULL;
		map[i].locked=false;
	}
	spinlock_acquire(&freemem_lock);
	isAct=1;
	spinlock_release(&freemem_lock);
}

/*
 * Check if we're in a context that can sleep. While most of the
 * operations in vm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */
void map_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

paddr_t getfreeppages(unsigned long int npages,char type,struct addrspace* as,vaddr_t vaddr){
	if(!isActive()) return 0;
	spinlock_acquire(&freemem_lock);
	unsigned int start=NOTINIT;
	for(unsigned int i=0;i<totalPages;i++){
		if(map[i].type==FREE){
			if(start==NOTINIT) start=i;
            if(i-start>=npages-1){
                spinlock_release(&freemem_lock);
                for(unsigned int j=i;j>=start;j--){
                    map[j].type=type;
                    map[j].numpages=npages;
                    map[j].fifonext=NOTINIT;
                    map[j].as=as;
					map[j].locked=false;
					map[j].vaddr=vaddr;
                }
                if(type==USER){
                    if(lastuserallocated!=NOTINIT)  map[lastuserallocated].fifonext=start;
                    if(swapvictim==NOTINIT) swapvictim=start;
                    lastuserallocated=start;
                }
                return (paddr_t) PAGE_SIZE*start;
            }
        }
        else{
			i+=map[i].numpages;
			start=NOTINIT;
		}
	}
	spinlock_release(&freemem_lock);
	return 0;
}
paddr_t getppages(unsigned long npages)
{
	paddr_t addr;
	addr= getfreeppages(npages,KERNEL,NULL,0);
	if(addr!=0) return addr;
	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);

	spinlock_release(&stealmem_lock);
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t alloc_kpages(unsigned npages)
{
	paddr_t pa;

	map_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr)
{
	if(!isActive()) return;
	paddr_t paddr=addr-MIPS_KSEG0;
	unsigned int i=paddr/PAGE_SIZE;
	spinlock_acquire(&freemem_lock);
	unsigned int numpages=map[i].numpages;
    for(unsigned int j=i;j<i+numpages;j++){
        map[j].type=FREE;
        map[j].numpages=0;
        map[j].as=NULL;
        map[j].fifonext=NOTINIT;
		map[j].locked=false;
    }
	spinlock_release(&freemem_lock);
}

paddr_t alloc_user_page(vaddr_t vaddr){
    struct addrspace* as=proc_getas();
    paddr_t res=getfreeppages(1,USER,as,vaddr);
    if(res!=0){
        //found physical frame available
		as_zero_region(res,PAGE_SIZE);
        return res;
    }
    else{
        //swapping needed
        if(swapvictim==NOTINIT) panic("Don't have any user process to swap out!");
        unsigned int newvict=map[swapvictim].fifonext;
		struct ptpage* pagetoswap=getpageat(map[swapvictim].as,map[swapvictim].vaddr);
		off_t swapinoff= swapinpage((paddr_t) swapvictim*PAGE_SIZE);
		pagetoswap->paddr=INSWAPFILE;
		pagetoswap->swapoffset=swapinoff;
		as_zero_region((paddr_t) swapvictim*PAGE_SIZE,PAGE_SIZE);
		map[swapvictim].as=as;
		map[swapvictim].numpages=1;
		map[swapvictim].type=USER;
		map[swapvictim].vaddr=vaddr;
		map[swapvictim].fifonext=NOTINIT;
		map[swapvictim].locked=false;
		map[lastuserallocated].fifonext=swapvictim;
		lastuserallocated=swapvictim;
		swapvictim=newvict;
		return (paddr_t)   lastuserallocated*PAGE_SIZE;      
    }
}