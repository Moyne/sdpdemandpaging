#include <types.h>
#include <addrspace.h>
#define INSWAPFILE 0x80000001
struct ptpage {
    struct addrspace* pageas;
    vaddr_t vaddr;
    paddr_t paddr;
    bool swapped;
    struct ptpage* next;
};
void pagetbinit(void);
int addentry(struct addrspace* as,vaddr_t vaddr,paddr_t paddr);
struct ptpage* getentry(struct addrspace* as,vaddr_t vaddr);
int entryswapped(struct ptpage* ent,off_t swapoff);
int entrymem(struct ptpage* ent,paddr_t paddr);
struct ptpage* pagetbcreateentry(struct addrspace* as,vaddr_t vaddr,paddr_t paddr);
int rementry(struct addrspace* as,vaddr_t vaddr);
void ptdes(void);