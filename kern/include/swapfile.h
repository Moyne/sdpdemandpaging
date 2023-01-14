#include <types.h>
#define SWAPFILESIZE 13*1024*1024
#define SWAPPAGES SWAPFILESIZE/PAGE_SIZE
struct swappage{
    pid_t pid;
    vaddr_t addr;
};


void getswapspin(void);
void releaseswapspin(void);
void swapinit(void);
int swapinpage(pid_t pid,vaddr_t addr,paddr_t readfrom);
int swapoutpage(pid_t pid,vaddr_t addr,paddr_t readin);
void swapdest(void);
void copytofromswap(paddr_t to,off_t from);