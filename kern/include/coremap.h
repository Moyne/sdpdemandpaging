#include "opt-paging.h"
#if OPT_PAGING

#include <addrspace.h>
#include <types.h>
//inner function to get page table status
int isptactive(void);
//initialize the page table and also hashed page table
int ptinit(void);
//destroy resources
void ptdest(void);
void pt_can_sleep(void);
//kernel allocation of npages
paddr_t getfreeppages(unsigned long int npages);
paddr_t getppages(unsigned long npages);
//wrapper for kernel page allocation
vaddr_t alloc_kpages(unsigned npages);
//free kernel pages
void free_kpages(vaddr_t addr);
//allocate user page with pid and addr
paddr_t allocuserpage(pid_t pid,vaddr_t addr);
//free a user page
int freeuserpage(pid_t pid,vaddr_t addr);
//get physical address of page
paddr_t pageaddr(pid_t pid,vaddr_t addr);
//remove every entry from page table and also hashed page table that contain pid
void removeptentries(pid_t pid);
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

#endif