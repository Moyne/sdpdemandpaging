#include <pt.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <swapfile.h>
#include <coremap.h>

struct ptpage* gethashedentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr){
	return hashedpt[(vaddr|pid)%hashedptsize];
}

int getfreeframes(struct ptpage* pagetable,unsigned int ptsize,unsigned int numpages){
	for(unsigned int i=0;i<ptsize;i++){
		if(pagetable[i].pid==-1){
			for(unsigned int j=i;j<ptsize;j++){
				if(pagetable[j].pid!=-1)	break;
				else if(j-i+1==numpages)	return i;
			}
		}
		else if(pagetable[i].numpages>0) i+=pagetable[i].numpages-1;
	}
	return -1;
}

void sethashedptentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr,struct ptpage* node){
	hashedpt[(vaddr|pid)%hashedptsize]=node;
	return;
}

void clearhashedptentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr){
	hashedpt[(vaddr|pid)%hashedptsize]=NULL;
	return;
}

int setptentry(struct ptpage* pagetable,struct ptpage** hashedpt,unsigned int hashedptsize,unsigned int index,unsigned int numpages,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr){
	pagetable[index].pid=pid;
	pagetable[index].vaddr=vaddr;
	pagetable[index].fifonext=NULL;
	pagetable[index].next=NULL;
	pagetable[index].numpages=numpages;
	if(pid>0){
		struct ptpage* node=&pagetable[index];
		struct ptpage* hashednode=gethashedentry(hashedpt,hashedptsize,pid,vaddr);
		//set hashed pt entry
		if(hashednode==NULL)	sethashedptentry(hashedpt,hashedptsize,pid,vaddr,node);
		else{
			for(struct ptpage* n=hashednode;n;n=n->next){
				if(n->next==NULL){
					n->next=node;
					break;
				}
			}
		}
		//set lastallocateduser and fix firstvictim if not setted yet
		if(*lastallocateduser!=NULL)	(*lastallocateduser)->fifonext=node;
		if(*firstvictim==NULL)	*firstvictim=node;
		*lastallocateduser=node;
	}
	return 0;
}

int freeuser(struct ptpage** hashedpt,unsigned int hashedptsize,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr){
	struct ptpage* nodetoremove=NULL;
	struct ptpage* hashedentry=gethashedentry(hashedpt,hashedptsize,pid,vaddr);
	struct ptpage* prec=NULL;
	for(struct ptpage* node=hashedentry;node;node=node->next){
		if(node->pid==pid && node->vaddr==vaddr){
			//fix linked list
			if(prec!=NULL)	prec->next=node->next;
			nodetoremove=node;
			break;
		}
		prec=node;
	}
	if(nodetoremove==NULL) return -1;
	//clear hashed entry as well
	if(hashedentry==nodetoremove)	sethashedptentry(hashedpt,hashedptsize,pid,vaddr,nodetoremove->next);
	//fix fifo linked list, this is needed because fifo list is not double linked
	if(*firstvictim!=nodetoremove){
		for(struct ptpage* node=*firstvictim;node;node=node->fifonext){
			if(node->fifonext->pid==pid && node->fifonext->vaddr==vaddr){
				//if we are trying to free the last allocated user fix the last allocated user
				//to the node immediately before
				//if(*lastallocateduser==nodetoremove)	*lastallocateduser=node;
				node->fifonext=node->fifonext->fifonext;
				break;
			}
		}
	}
	else	*firstvictim=nodetoremove->fifonext;
	if(*lastallocateduser==nodetoremove){
		if(*firstvictim==NULL)	*lastallocateduser=NULL;
		else{
			for(struct ptpage* n=*firstvictim;n;n=n->fifonext){
				if(n->fifonext==NULL){
					*lastallocateduser=n;
					break;
				}
			}
		}
	}
	nodetoremove->fifonext=NULL;
	nodetoremove->next=NULL;
	nodetoremove->numpages=0;
	nodetoremove->pid=(pid_t) -1;
	return 0;
}

int freekernel(struct ptpage* pagetable,unsigned int index){
	pagetable[index].fifonext=NULL;
	pagetable[index].next=NULL;
	pagetable[index].numpages=0;
	pagetable[index].pid=(pid_t) -1;
	return 0;
}

paddr_t getpageaddr(struct ptpage* pagetable,struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr){
	struct ptpage* hashedentry=gethashedentry(hashedpt,hashedptsize,pid,vaddr);
	if(hashedentry==NULL) return (paddr_t) 0;
	for(struct ptpage* node=hashedentry;node;node=node->next)	if(node->pid==pid && node->vaddr==vaddr)	return (paddr_t) (node-pagetable)*PAGE_SIZE;
	return (paddr_t) 0;
}