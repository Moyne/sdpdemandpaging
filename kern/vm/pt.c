#include <pt.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <swapfile.h>
#include <coremap.h>
#include <spinlock.h>
struct ptpage** pagetable;
struct spinlock pagespin=SPINLOCK_INITIALIZER;
void pagetbinit(void){
	pagetable=kmalloc(PTSIZE*sizeof(struct ptpage*));
	spinlock_acquire(&pagespin);
	for(unsigned int i=0;i<PTSIZE;i++) pagetable[i]=NULL;
	spinlock_release(&pagespin);
}

struct ptpage* pagetbcreateentry(struct addrspace* as,vaddr_t vaddr,paddr_t paddr){
	struct ptpage* new=kmalloc(sizeof(struct ptpage));
	new->paddr=paddr;
	new->vaddr=vaddr;
	new->pageas=as;
	new->swapped=false;
	new->next=NULL;
	return new;
}

int addentry(struct addrspace* as,vaddr_t vaddr,paddr_t paddr){
	int ind=(vaddr/PAGE_SIZE)%PTSIZE;
	struct ptpage* new=pagetbcreateentry(as,vaddr,paddr);
	spinlock_acquire(&pagespin);
	if(pagetable[ind]){
		for(struct ptpage* node=pagetable[ind];node;node=node->next){
			if(node->next==NULL){
				node->next=new;
				break;
			}
		}
	}
	else pagetable[ind]=new;
	spinlock_release(&pagespin);
	return 0;
}

int rementry(struct addrspace* as,vaddr_t vaddr){
	int ind=(vaddr/PAGE_SIZE)%PTSIZE;
	spinlock_acquire(&pagespin);
	struct ptpage* prevnode=NULL;
	for(struct ptpage* node=pagetable[ind];node;node=node->next){
		if(node->pageas==as && node->vaddr==vaddr){
			if(prevnode==NULL)	pagetable[ind]=node->next;
			else	prevnode->next=node->next;
			kfree(node);
			break;
		}
		else prevnode=node;
	}
	spinlock_release(&pagespin);
	return 0;
}

struct ptpage* getentry(struct addrspace* as,vaddr_t vaddr){
	int ind=(vaddr/PAGE_SIZE)%PTSIZE;
	for(struct ptpage* node=pagetable[ind];node;node=node->next)	if(node->pageas==as && node->vaddr==vaddr)	return node;
	return NULL;
}

int entryswapped(struct ptpage* ent,off_t swapoff){
	spinlock_acquire(&pagespin);
	KASSERT(ent!=NULL);
	ent->paddr=(paddr_t) swapoff;
	ent->swapped=true;
	spinlock_release(&pagespin);
	return 0;
}

int entrymem(struct ptpage* ent,paddr_t paddr){
	spinlock_acquire(&pagespin);
	ent->paddr=paddr;
	ent->swapped=false;
	spinlock_release(&pagespin);
	return 0;
}

void ptdes(void){
    kfree(pagetable);
}