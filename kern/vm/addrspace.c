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
#include "opt-paging.h"
#if OPT_PAGING
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <current.h>
#include <swapfile.h>
#include <coremap.h>
#include <uio.h>
#include <vnode.h>
#include <vm.h>
#include <proc.h>
#include <spl.h>
#include <vm_tlb.h>
#include <vfs.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

void as_zero_region(paddr_t paddr,size_t len){
	bzero((void *)PADDR_TO_KVADDR(paddr),len);
}

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */
	as->seg1=NULL;as->seg2=NULL;as->segstack=NULL;

	return as;
}

int as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */

	newas->seg1=segcopy(old->seg1);
	newas->seg2=segcopy(old->seg2);
	newas->segstack=segcopy(old->segstack);

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as,pid_t pid)
{
	/*
	 * Clean up as needed.
	 */
	if(as->seg1->elfdata!=NULL)	vfs_close(as->seg1->elfdata);
	segdes(as->seg1);segdes(as->seg2);segdes(as->segstack);
	removeptentries(pid);
	removeswapentries(pid);
	kfree(as);
}

void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/*
	 * Write this.
	 */
	//tlb invalidation
	int spl = splhigh();
	vmtlb_invalidate();
	splx(spl);
	
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 off_t offset,struct vnode* vnode,
		 int readable, int writable, int executable)
{
	size_t npages;

	pt_can_sleep();

	/* Align the region. First, the base... */
	size_t sz=memsize;
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	//(void)writeable;
	//(void)executable;
	/*My bad machine dependant code, move all this into as*/
	if (as->seg1 == NULL) {
		as->seg1=segcreate();
		if(as->seg1==NULL){
			return ENOMEM;
		}
		segdef(as->seg1,vaddr,npages,offset,vnode,
					false,readable,writable,executable);
		return 0;
	}

	else if (as->seg2 == NULL) {
		as->seg2=segcreate();
		if(as->seg2==NULL){
			return ENOMEM;
		}
		segdef(as->seg2,vaddr,npages,offset,vnode,
					false,readable,writable,executable);
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	else{
		kprintf("dumbvm: Warning: too many regions\n");
		return ENOSYS;
	}
}
int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	(void)as;
	as->segstack=segcreate();
	segdef(as->segstack,USERSTACK - STACKPAGES*PAGE_SIZE,STACKPAGES,0,NULL,true,RDFLAG,WRFLAG,0);
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}

void readfromelfto(struct addrspace* as, vaddr_t vaddr,paddr_t paddr){
	struct iovec iov;
	struct uio io;
	int res=-1;
	paddr&=PAGE_FRAME;
	if(as->seg1->startaddr<=vaddr && ((as->seg1->numpages*PAGE_SIZE)+as->seg1->startaddr)>=vaddr){
		uio_kinit(&iov,&io,(void*) PADDR_TO_KVADDR(paddr),PAGE_SIZE,vaddr-as->seg1->startaddr+as->seg1->elfoffset,UIO_READ);
		res=VOP_READ(as->seg1->elfdata,&io);
	}
	else if(as->seg2->startaddr<=vaddr && ((as->seg2->numpages*PAGE_SIZE)+as->seg2->startaddr)>=vaddr){
		uio_kinit(&iov,&io,(void*) PADDR_TO_KVADDR(paddr),PAGE_SIZE,vaddr-as->seg2->startaddr+as->seg2->elfoffset,UIO_READ);
		res=VOP_READ(as->seg2->elfdata,&io);
	}
	else if(as->segstack->startaddr<=vaddr && ((as->segstack->numpages*PAGE_SIZE)+as->segstack->startaddr)>=vaddr){
		uio_kinit(&iov,&io,(void*) PADDR_TO_KVADDR(paddr),PAGE_SIZE,vaddr-as->segstack->startaddr+as->segstack->elfoffset,UIO_READ);
		res=VOP_READ(as->segstack->elfdata,&io);
	}
	if(res) panic("Could not read from elf");
}
#endif