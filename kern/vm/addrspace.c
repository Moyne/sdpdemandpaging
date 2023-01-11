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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

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

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */

	(void)old;

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */

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
	vmtlb_invalidate();
	
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
#if OPT_DEMANDPAGING
void ptdef(struct pagetable* pt,size_t size,vaddr_t vaddr){
	KASSERT(pt!==NULL);
	pt->size=size;
	pt->vaddr=vaddr;
	pt->pages=kmalloc(size*sizeof(paddr_t));
	if(pt->pages==NULL) return ENOMEM;
	for(int i=0;i<size;i++) pt->pages[i]=DEMANDPAGEFROMELF;
}

struct pagetable* pagetbcreate(){
	struct pagetb* pt=kmalloc(sizeof(struc pagetable));
	if(pt==NULL) return ENOMEM;
	pt->pages=NULL;
	pt->size=0;
	pt->vaddr=0;
	return pt;
}



struct segment* segcreate(){
	struct segment* seg=kmalloc(sizeof(struct segment));
	if(segment==NULL) return NULL;
	segment->numpages=0;
	segment->elfdata=NULL;
	segment->filesize=0;
	segment->memsize=0;
	segment->offset=0;
	segment->vaddr=0;
	segment->pagetable=NULL;
	segment->permissions=0;
	return segment;
}

void segdef(struct segment* seg,vaddr_t vaddr,size_t npages,size_t filesize,size_t memsize,
			off_t offset,struct vnode* elfnode,int readable,int writable,int executable){
	KASSERT(seg!==NULL);
	seg->vaddr=vaddr;
	seg->numpages=npages;
	seg->filesize=filesize;
	seg->memsize=memsize;
	seg->offset=offset;
	seg->elfnode=elfnode;
	seg->permissions=(char) (readable&RDFLAG) | (writable&WRFLAG) | (executable&EXFLAG);
	seg->pagetable=pagetbcreate();
	if(seg->pagetable==NULL) panic('Not enough memory for pagetable');
	ptdef(seg->pagetable,npages,vaddr);
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize, size_t filesize,
		 off_t offset,struct vnode* vode,
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
	//(void)writeable;
	//(void)executable;
	/*My bad machine dependant code, move all this into as*/
	if (as->seg1 == NULL) {
		as->seg1=segcreate();
		if(as->seg1==NULL){
			return ENOMEM;
		}
		segdef(as->seg1,vaddr,npages,filesize,memsize,offset,v,
					readable,writable,executable);
	}

	if (as->seg2 == NULL) {
		as->seg2=segcreate();
		if(as->seg2==NULL){
			return ENOMEM;
		}
		segdef(as->seg2,vaddr,npages,filesize,memsize,offset,v,
					readable,writable,executable);
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return ENOSYS;
}
#else
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */

	(void)as;
	(void)vaddr;
	(void)memsize;
	(void)readable;
	(void)writeable;
	(void)executable;
	return ENOSYS;
}
#endif
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

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}

