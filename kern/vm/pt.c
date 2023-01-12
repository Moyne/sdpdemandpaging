#include <pt.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <swapfile.h>
#include <coremap.h>
void ptdef(struct pagetable* pt,size_t size,vaddr_t vaddr){
	KASSERT(pt!=NULL);
	pt->size=size;
	pt->startingvaddr=vaddr;
	pt->pages=kmalloc(size*sizeof(struct ptpage));
	if(pt->pages==NULL) panic("Out of memory for page table");
	for(unsigned int i=0;i<size;i++){
        pt->pages[i].paddr=INELFFILE;
        pt->pages[i].swapoffset=0;
    }
}

struct pagetable* pagetbcreate(void){
	struct pagetable* pt=kmalloc(sizeof(struct pagetable));
	if(pt==NULL) return NULL;
	pt->pages=NULL;
	pt->size=0;
	pt->startingvaddr=0;
	return pt;
}

struct pagetable* ptcopy(struct pagetable* pt){
	struct pagetable* newpt=pagetbcreate();
	newpt->startingvaddr=pt->startingvaddr;
	newpt->size=pt->size;
	newpt->pages=kmalloc(pt->size*sizeof(struct ptpage));
	if(pt->pages==NULL) return NULL;
	for(unsigned int i=0;i<pt->size;i++){
        if(pt->pages[i].paddr==INELFFILE){
		newpt->pages[i].paddr=INELFFILE;
        newpt->pages[i].swapoffset=0;
		}
		else{
			//copy
			paddr_t paddr=alloc_user_page(newpt->startingvaddr+i*PAGE_SIZE);
			if(pt->pages[i].paddr==INSWAPFILE)	copytofromswap(paddr,pt->pages[i].swapoffset);
			else	memcpy((void*)paddr,(void*)pt->pages[i].paddr,PAGE_SIZE);
		}
    }
	return newpt;
}

void ptdes(struct pagetable* pt){
    KASSERT(pt!=NULL);
    pt->size=0;
    pt->startingvaddr=0;
    kfree(pt->pages);
    kfree(pt);
}