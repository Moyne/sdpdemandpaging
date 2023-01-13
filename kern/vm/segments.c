#include <segments.h>
#include <vm.h>
#include <kern/errno.h>
#include <lib.h>
#include <vnode.h>
#include <proc.h>
#include <uio.h>
struct segment* segcreate(void){
	struct segment* seg=kmalloc(sizeof(struct segment));
	if(seg==NULL) return NULL;
	seg->numpages=0;
	seg->elfdata=NULL;
	seg->offset=0;
	seg->vaddr=0;
	seg->pagetable=NULL;
	seg->permissions=0;
	return seg;
}

void segdef(struct segment* seg,vaddr_t vaddr,size_t npages,
			off_t offset,struct vnode* elfnode,int readable,int writable,int executable){
	KASSERT(seg!=NULL);
	seg->vaddr=vaddr;
	seg->numpages=npages;
	seg->offset=offset;
	seg->elfdata=elfnode;
	seg->permissions=(char) readable | writable | executable;
	seg->pagetable=pagetbcreate();
	if(seg->pagetable==NULL) panic("Not enough memory for pagetable");
	ptdef(seg->pagetable,npages,vaddr);
}

struct segment* segcopy(struct segment* seg){
	struct segment* newseg=segcreate();
	newseg->elfdata=seg->elfdata;
	newseg->numpages=seg->numpages;
	newseg->offset=seg->offset;
	newseg->pagetable=ptcopy(seg->pagetable);
	newseg->permissions=seg->permissions;
	newseg->vaddr=seg->vaddr;
	return newseg;
}
void segdes(struct segment* seg){
    seg->numpages=0;
	seg->elfdata=NULL;
	seg->offset=0;
	seg->vaddr=0;
	seg->permissions=0;
    ptdes(seg->pagetable);
    kfree(seg);
}
struct ptpage* seggetpageat(struct segment* seg, vaddr_t vaddr){
	return &seg->pagetable->pages[(vaddr-seg->vaddr)/PAGE_SIZE];
}