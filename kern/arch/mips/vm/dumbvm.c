/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "opt-dumbvm.h"
#if OPT_DUMBVM


#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

unsigned long int *bitmap;
// unsigned int* sizeMap;
int totalPages;
int isAct = 0;
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
#define DUMBVM_STACKPAGES 18

/*
 * Wrap ram_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static struct spinlock freemem_lock = SPINLOCK_INITIALIZER;
static int isActive()
{
	int active;
	spinlock_acquire(&freemem_lock);
	active = isAct;
	spinlock_release(&freemem_lock);
	return active;
}
void vm_bootstrap(void)
{
	/* Do nothing. */
	/*int bitLen=(int)(ram_getsize()/(PAGE_SIZE*sizeof(unsigned long int)*8));
	totalPages=(int)ram_getsize()/PAGE_SIZE;
	if(!bitLen) bitLen=1;
	bitmap=kmalloc(bitLen*sizeof(unsigned long int));
	if(bitmap==NULL) return;
	sizeMap=kmalloc(totalPages*sizeof(int));
	if(sizeMap==NULL){
		bitmap=NULL;
		return;
	}
	int firstFree=ram_getfirstfree()/PAGE_SIZE;
	int mask=1;
	for(int i=0;i<bitLen;i++)	bitmap[i]=0;
	for(int i=0,j=0;i<firstFree;i++){
		if(!(i%(sizeof(unsigned long int)*8)) && i>0){
			j++;
			mask=1;
		}
		bitmap[j]|=mask;
		mask<<=1;
	}
	for(int i=0;i<totalPages;i++){
		if(i>=firstFree)	sizeMap[i]=0;
		else sizeMap[i]=1;
	}
	spinlock_acquire(&freemem_lock);
	isAct=1;
	spinlock_release(&freemem_lock);*/
	int bitLen = (int)(ram_getsize() / (PAGE_SIZE * sizeof(unsigned long int) * 4));
	totalPages = (int)ram_getsize() / PAGE_SIZE;
	if (!bitLen)
		bitLen = 1;
	bitmap = kmalloc(bitLen * sizeof(unsigned long int));
	if (bitmap == NULL)
		return;
	int firstFree = ram_getfirstfree() / PAGE_SIZE;
	int mask = 2;
	for (int i = 0; i < bitLen; i++)
		bitmap[i] = 0;
	for (int i = 0, j = 0; i < firstFree; i++)
	{
		if (!((i * 2) % (sizeof(unsigned long int) * 8)) && i > 0)
		{
			j++;
			mask = 2;
		}
		bitmap[j] |= mask;
		mask <<= 2;
	}
	spinlock_acquire(&freemem_lock);
	isAct = 1;
	spinlock_release(&freemem_lock);
}

/*
 * Check if we're in a context that can sleep. While most of the
 * operations in dumbvm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */
static void
dumbvm_can_sleep(void)
{
	if (CURCPU_EXISTS())
	{
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

static paddr_t getfreeppages(unsigned long int npages)
{
	/*if(!isActive()) return 0;
	unsigned long int* bitmap_t=bitmap;
	unsigned long int mask=1;
	unsigned long n=0;
	spinlock_acquire(&freemem_lock);
	for(int i=0;i<totalPages;i++){
		if(!(i%(sizeof(unsigned long int)*8)) && i>0){
			bitmap_t++;
			mask=1;
		}
		if((mask&*bitmap_t)==0){
			n++;
			if(n==npages){
				for(unsigned long k=0;k<npages;k++){
					*bitmap_t|=mask;
					mask>>=1;
					if(!mask){
						mask=1;
						mask<<=(sizeof(unsigned long int)*8)-1;
						bitmap_t--;
					}
				}
				sizeMap[i-npages+1]=npages;
				spinlock_release(&freemem_lock);
				return (paddr_t) PAGE_SIZE*i;
			}
		}
		else n=0;
		mask<<=1;
	}
	spinlock_release(&freemem_lock);
	return 0;*/
	if (!isActive())
		return 0;
	unsigned long int *bitmap_t = bitmap;
	unsigned long int mask = 3;
	unsigned long n = 0;
	spinlock_acquire(&freemem_lock);
	for (int i = 0; i < totalPages; i++)
	{
		if (!((i * 2) % (sizeof(unsigned long int) * 8)) && i > 0)
		{
			bitmap_t++;
			mask = 3;
		}
		if ((mask & *bitmap_t) == 0)
		{
			n++;
			if (n == npages)
			{
				for (unsigned long k = 0; k < npages; k++)
				{
					if (!k)
						*bitmap_t |= 2 << ((i - k) * 2) % (sizeof(unsigned long int) * 8);
					else if (k == npages - 1 && k > 0)
						*bitmap_t |= mask;
					else
						*bitmap_t |= 1 << ((i - k) * 2) % (sizeof(unsigned long int) * 8);
					mask >>= 2;
					if (!mask && k != npages - 1)
					{
						mask = 3;
						mask <<= ((sizeof(unsigned long int) * 8) / 2) - 1;
						bitmap_t--;
					}
				}
				spinlock_release(&freemem_lock);
				return (paddr_t)PAGE_SIZE * i;
			}
		}
		else
			n = 0;
		mask <<= 2;
	}
	spinlock_release(&freemem_lock);
	return 0;
}
static paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;
	addr = getfreeppages(npages);
	if (addr != 0)
		return addr;
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

	dumbvm_can_sleep();
	pa = getppages(npages);
	if (pa == 0)
	{
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */
	/*if(!isActive()) return;
	unsigned long int* bitmap_t=bitmap;
	paddr_t paddr=addr-MIPS_KSEG0;
	int i=paddr/PAGE_SIZE;
	unsigned long int mask=1;
	bitmap_t+=(int)i/(sizeof(unsigned long int)*8);
	mask<<=(i%(sizeof(unsigned long int)*8));
	mask=~mask;
	spinlock_acquire(&freemem_lock);
	for(unsigned int j=0;j<sizeMap[i];j++){
		if(!((j+i)%(sizeof(unsigned long int)*8)) && j>0){
			bitmap_t++;
			mask=~1;
		}
		*bitmap_t&=mask;
		mask=~mask;
		mask<<=1;
		mask=~mask;
	}
	sizeMap[i]=0;
	spinlock_release(&freemem_lock);*/
	if (!isActive())
		return;
	unsigned long int *bitmap_t = bitmap;
	paddr_t paddr = addr - MIPS_KSEG0;
	int i = paddr / PAGE_SIZE;
	unsigned long int mask = 3;
	unsigned long int endMask = 2;
	bitmap_t += (int)i / (sizeof(unsigned long int) * 4);
	mask <<= ((i * 2) % (sizeof(unsigned long int) * 8));
	endMask <<= ((i * 2) % (sizeof(unsigned long int) * 8));
	mask = ~mask;
	spinlock_acquire(&freemem_lock);
	unsigned int j = 0;
	while (1)
	{
		if (!(((j + i) * 2) % (sizeof(unsigned long int) * 8)) && j > 0)
		{
			bitmap_t++;
			mask = ~3;
			endMask = 2;
		}
		unsigned long int res = *bitmap_t & (~mask);
		*bitmap_t &= mask;
		mask = ~mask;
		mask <<= 2;
		mask = ~mask;
		j++;
		if (res == endMask)
			break;
		endMask <<= 2;
	}
	spinlock_release(&freemem_lock);
}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	// int i;
	// uint32_t ehi, elo;
	struct addrspace *as;
	int spl;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype)
	{
	case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	case VM_FAULT_READ:
	case VM_FAULT_WRITE:
		break;
	default:
		return EINVAL;
	}

	if (curproc == NULL)
	{
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL)
	{
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
	if (faultaddress >= vbase1 && faultaddress < vtop1)
	{
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2)
	{
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop)
	{
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else
	{
		return EFAULT;
	}

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
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
	}
	splx(spl);
	return EFAULT;
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as == NULL)
	{
		return NULL;
	}

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;
	return as;
}

void as_destroy(struct addrspace *as)
{
	dumbvm_can_sleep();
	kfree(as);
}

void as_activate(void)
{
	int spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL)
	{
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	vmtlb_invalidate();

	splx(spl);
}

void as_deactivate(void)
{
	/* nothing */
}

int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
					 int readable, int writeable, int executable)
{
	size_t npages;

	dumbvm_can_sleep();

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;
	if (as->as_vbase1 == 0)
	{
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}

	if (as->as_vbase2 == 0)
	{
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return ENOSYS;
}

static void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int as_prepare_load(struct addrspace *as)
{
	KASSERT(as->as_pbase1 == 0);
	KASSERT(as->as_pbase2 == 0);
	KASSERT(as->as_stackpbase == 0);

	dumbvm_can_sleep();

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0)
	{
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0)
	{
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0)
	{
		return ENOMEM;
	}

	as_zero_region(as->as_pbase1, as->as_npages1);
	as_zero_region(as->as_pbase2, as->as_npages2);
	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);

	return 0;
}

int as_complete_load(struct addrspace *as)
{
	dumbvm_can_sleep();
	(void)as;
	return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

int as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	dumbvm_can_sleep();

	new = as_create();
	if (new == NULL)
	{
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
	/*My bad machine dependant code, move all this into as*/
	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new))
	{
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT(new->as_pbase1 != 0);
	KASSERT(new->as_pbase2 != 0);
	KASSERT(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
			(const void *)PADDR_TO_KVADDR(old->as_pbase1),
			old->as_npages1 * PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
			(const void *)PADDR_TO_KVADDR(old->as_pbase2),
			old->as_npages2 * PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
			(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
			DUMBVM_STACKPAGES * PAGE_SIZE);

	*ret = new;
	return 0;
}
#endif