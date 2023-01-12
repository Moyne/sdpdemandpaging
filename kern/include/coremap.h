#include <addrspace.h>
#include <types.h>
#define KERNEL 1
#define USER 2
#define FREE 0
#define NOTINIT 99999
struct page {
    char type;
    bool locked;
    unsigned int numpages;
    unsigned int fifonext;
    struct addrspace* as;
    vaddr_t vaddr;
};

void mapinit(void);
int isActive(void);
void map_can_sleep(void);
paddr_t getfreeppages(unsigned long int npages,char type,struct addrspace* as,vaddr_t vaddr);
paddr_t getppages(unsigned long npages);
vaddr_t alloc_kpages(unsigned npages);
void free_kpages(vaddr_t addr);
paddr_t alloc_user_page(vaddr_t vaddr);
/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 *
 * NOTE: it's been found over the years that students often begin on
 * the VM assignment by copying dumbvm.c and trying to improve it.
 * This is not recommended. dumbvm is (more or less intentionally) not
 * a good design reference. The first recommendation would be: do not
 * look at dumbvm at all. The second recommendation would be: if you
 * do, be sure to review it from the perspective of comparing it to
 * what a VM system is supposed to do, and understanding what corners
 * it's cutting (there are many) and why, and more importantly, how.
 */

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define VM_STACKPAGES    18

/*
 * Wrap ram_stealmem in a spinlock.
 */