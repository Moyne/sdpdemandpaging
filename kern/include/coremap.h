#include <types.h>
#include <addrspace.h>
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