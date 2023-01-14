#include <types.h>
struct ptpage {
    pid_t pid;
    vaddr_t vaddr;
    unsigned int numpages;
    struct ptpage* fifonext;
    struct ptpage* next;
};
struct fifo{
    struct ptpage* node;
    struct fifo* next;
};
struct ptpage* gethashedentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr);
void clearhashedptentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr);
void sethashedptentry(struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr,struct ptpage* node);
int getfreeframes(struct ptpage* pagetable,unsigned int ptsize,unsigned int numpages);
int setptentry(struct ptpage* pagetable,struct ptpage** hashedpt,unsigned int hashedptsize,unsigned int index,unsigned int numpages,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr);
int freeuser(struct ptpage** hashedpt,unsigned int hashedptsize,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr);
int freekernel(struct ptpage* pagetable,unsigned int index);
paddr_t getpageaddr(struct ptpage* pagetable,struct ptpage** hashedpt,unsigned int hashedptsize,pid_t pid,vaddr_t vaddr);