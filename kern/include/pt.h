#include <types.h>
#define INSWAPFILE 2
#define INELFFILE 1
#define INMEMORY 0
#ifndef _PT_H_
struct ptpage {
    //vaddr_t addr;
    paddr_t paddr;
    off_t swapoffset;
};

struct pagetable{
    struct ptpage* pages;
    unsigned int size;
    vaddr_t startingvaddr;
};

void ptdef(struct pagetable* pt,size_t size,vaddr_t vaddr);
struct pagetable* pagetbcreate(void);
struct pagetable* ptcopy(struct pagetable* pt);
void ptdes(struct pagetable* pt);
#define _PT_H_
#endif