#include "opt-paging.h"
#if OPT_PAGING

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
	seg->elfoffset=0;
	seg->startaddr=0;
	seg->stackseg=false;
	seg->loadedelf=NULL;
	seg->permissions=0;
	return seg;
}

void segdef(struct segment* seg,vaddr_t vaddr,size_t npages,
			off_t offset,struct vnode* elfnode,bool stack,int readable,int writable,int executable){
	KASSERT(seg!=NULL);
	seg->startaddr=vaddr;
	seg->numpages=npages;
	seg->elfoffset=offset;
	seg->elfdata=elfnode;
	seg->permissions=(char) readable | writable | executable;
	seg->stackseg=stack;
	seg->loadedelf=kmalloc(npages*sizeof(unsigned char));
	for(unsigned int i=0;i<npages;i++) seg->loadedelf[i]=0;
}

struct segment* segcopy(struct segment* seg){
	struct segment* newseg=segcreate();
	newseg->elfdata=seg->elfdata;
	newseg->numpages=seg->numpages;
	newseg->elfoffset=seg->elfoffset;
	newseg->permissions=seg->permissions;
	newseg->startaddr=seg->startaddr;
	newseg->stackseg=seg->stackseg;
	newseg->loadedelf=kmalloc(newseg->numpages*sizeof(unsigned char));
	for(unsigned int i=0;i<newseg->numpages;i++) newseg->loadedelf[i]=0;
	return newseg;
}
void segdes(struct segment* seg){
    seg->numpages=0;
	seg->elfdata=NULL;
	seg->elfoffset=0;
	seg->startaddr=0;
	seg->permissions=0;
    seg->stackseg=false;
	kfree(seg->loadedelf);
    kfree(seg);
}
#endif