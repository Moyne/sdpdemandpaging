#include "opt-paging.h"
#if OPT_PAGING

#include <types.h>
/*
*This file can be seen as a library, these functions are used by coremap that wraps these function
*and do more business logic
*/


struct ptpage {
    pid_t pid;      //pid and vaddr are used as IDs for a single page when a page
    vaddr_t vaddr;  //is free it has pid -1 and if the page is owned by the kernel it has pid 0
    unsigned int numpages;  //field useful only for kernel pages because for user it's always 1
    struct ptpage* fifonext;    //pointer to the next element in the fifo queue
    struct ptpage* next;    //pointer for the next element that has the same hash as this one
};
//not used but would have been used to implement the fifo queue for the page replacement algo
struct fifo{
    struct ptpage* node;
    struct fifo* next;
};
//returns what the hashed page table points to for the hash of pid and vaddr
struct ptpage* gethashedentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr);
//not used anymore
void clearhashedptentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr);
//sets an entry of the hashed page table, so whenever we update the hashed page table we call
//this function, both on clears and sets
void sethashedptentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr,struct ptpage* node);
//returns the starting index of numpages free contiguous frames
int getfreeframes(struct ptpage* pagetable,unsigned int ptsize,unsigned int numpages);
//set a page table entry, works for both kernel and user pages, this sets also the hashed page table
int setptentry(struct ptpage* pagetable,struct ptpage** hashedpt,unsigned int hashedptsize,unsigned int index,unsigned int numpages,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr);
//free a user page, so clears the hashed page table, the victim fifo queue, last allocated etc.
int freeuser(struct ptpage** hashedpt,unsigned int hashedptsize,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr);
//free a kernel frame
int freekernel(struct ptpage* pagetable,unsigned int index);
//get physical address for page with pid and vaddr
paddr_t getpageaddr(struct ptpage* pagetable,struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr);

#endif