#include <coremap.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <vm_tlb.h>
#include <swapfile.h>
#include <vm.h>
#include <pt.h>
struct ptpage* pt;
struct ptpage* lastallocateduser=NULL;
struct ptpage* firstvictim=NULL;
struct ptpage** hashedpt;
unsigned int ptsize,hashedptsize;
int isAct=0;
//static struct lock* ptlock;
struct spinlock ptspin=SPINLOCK_INITIALIZER;
struct spinlock stealmem_lock=SPINLOCK_INITIALIZER;

int isptactive(void){
	int active;
	spinlock_acquire(&ptspin);
	active=isAct;
	spinlock_release(&ptspin);
	return active;
}

int ptinit(void){
	//ptlock=lock_create("Page table lock");
	ptsize=(unsigned int)ram_getsize()/PAGE_SIZE;
	hashedptsize=ptsize/32;
    pt=kmalloc(ptsize*sizeof(struct ptpage));
	if(pt==NULL) return ENOMEM;
	hashedpt=kmalloc(hashedptsize*sizeof(struct ptpage*));
	if(hashedpt==NULL)	return ENOMEM;
	unsigned int firstfree=ram_getfirstfree()/PAGE_SIZE; // if you want to try swapfile: ptsize-5;
	spinlock_acquire(&ptspin);
	for(unsigned int i=0;i<hashedptsize;i++) hashedpt[i]=NULL;
	for(unsigned int i=0;i<firstfree;i++)	setptentry(pt,hashedpt,hashedptsize,i,1,&firstvictim,&lastallocateduser,(pid_t)0,PADDR_TO_KVADDR(i*PAGE_SIZE));
	for(unsigned int i=firstfree;i<ptsize;i++)	setptentry(pt,hashedpt,hashedptsize,i,0,&firstvictim,&lastallocateduser,(pid_t)-1,PADDR_TO_KVADDR(i*PAGE_SIZE));
	isAct=1;
	spinlock_release(&ptspin);
	return 0;
}

void ptdest(void){
	spinlock_acquire(&ptspin);
	isAct=0;
	spinlock_release(&ptspin);
	kfree(hashedpt);
	kfree(pt);
}

//alloc npages for kernel purposes
paddr_t getfreeppages(unsigned long int npages){
	if(!isptactive()) return 0;
	spinlock_acquire(&ptspin);
	int ind=getfreeframes(pt,ptsize,npages);
	if(ind==-1){
		if(firstvictim==NULL){
			spinlock_release(&ptspin);
			panic("No space left on memory to allocate to kernel!!");
		}
		else{
			spinlock_release(&ptspin);
			//disk operation require not to have any other spinlock owned
			swapinpage(firstvictim->pid,firstvictim->vaddr,getpageaddr(pt,hashedpt,hashedptsize,firstvictim->pid,firstvictim->vaddr));
			freeuser(hashedpt,hashedptsize,&firstvictim,&lastallocateduser,firstvictim->pid,firstvictim->vaddr);
		}
	}
	setptentry(pt,hashedpt,hashedptsize,ind,npages,&firstvictim,&lastallocateduser,0,PADDR_TO_KVADDR(ind*PAGE_SIZE));
	spinlock_release(&ptspin);
	return ind*PAGE_SIZE;
}


/*
 * Check if we're in a context that can sleep. While most of the
 * operations in vm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */
void pt_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

paddr_t getppages(unsigned long npages)
{
	paddr_t addr;
	addr= getfreeppages(npages);
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

	pt_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr)
{
	if(!isptactive()) return;
	paddr_t paddr=addr-MIPS_KSEG0;
	unsigned int i=paddr/PAGE_SIZE;
	spinlock_acquire(&ptspin);
	freekernel(pt,i);
	spinlock_release(&ptspin);
}


paddr_t allocuserpage(pid_t pid,vaddr_t addr){
	if(!isptactive()) return 0;
	spinlock_acquire(&ptspin);
	int ind=getfreeframes(pt,ptsize,1);
	if(ind==-1){
		if(firstvictim==NULL){
			spinlock_release(&ptspin);
			return 0;
		}
		else{
			spinlock_release(&ptspin);
			//disk operation require not to have any other spinlock owned
			ind=firstvictim-pt;
			swapinpage(firstvictim->pid,firstvictim->vaddr,getpageaddr(pt,hashedpt,hashedptsize,firstvictim->pid,firstvictim->vaddr));
			spinlock_acquire(&ptspin);
			freeuser(hashedpt,hashedptsize,&firstvictim,&lastallocateduser,firstvictim->pid,firstvictim->vaddr);
		}
	}
	setptentry(pt,hashedpt,hashedptsize,ind,1,&firstvictim,&lastallocateduser,pid,addr);
	spinlock_release(&ptspin);
	//as_zero_region(ind*PAGE_SIZE,PAGE_SIZE);
	return ind*PAGE_SIZE;
}

int freeuserpage(pid_t pid,vaddr_t addr){
	if(!isptactive()) return EFAULT;
	spinlock_acquire(&ptspin);
	int res=freeuser(hashedpt,hashedptsize,&firstvictim,&lastallocateduser,pid,addr);
	spinlock_release(&ptspin);
	return res;
}

paddr_t pageaddr(pid_t pid,vaddr_t addr){
	if(!isptactive()) return 0;
	spinlock_acquire(&ptspin);
	paddr_t pagephysicaladdr=getpageaddr(pt,hashedpt,hashedptsize,pid,addr);
	spinlock_release(&ptspin);
	return pagephysicaladdr;
}

void removeptentries(pid_t pid){
	if(!isptactive()) return;
	spinlock_acquire(&ptspin);
	for(unsigned int i=0;i<ptsize;i++)	if(pt[i].pid==pid)	freeuser(hashedpt,hashedptsize,&firstvictim,&lastallocateduser,pid,pt[i].vaddr);
	spinlock_release(&ptspin);
}