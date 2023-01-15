#include "opt-paging.h"
#if OPT_PAGING

#include <types.h>
#define SWAPFILESIZE 13*1024*1024
#define SWAPPAGES SWAPFILESIZE/PAGE_SIZE
struct swappage{
    pid_t pid;      //pid and vaddr are used as IDs to identify a single page
    vaddr_t addr;
};

//simple function that wraps spinlock logic for acquiring
void getswapspin(void);
//simple function that wraps spinlock logic for release
void releaseswapspin(void);
//init of swapspace
void swapinit(void);
//insert into the swapfile this entry from readfrom
int swapinpage(pid_t pid,vaddr_t addr,paddr_t readfrom);
//read out of the swapfile into readin this entry of the swapfile
int swapoutpage(pid_t pid,vaddr_t addr,paddr_t readin);
//destroy swapspace
void swapdest(void);
//remove from the swapfile every entry with pid equal to pid
void removeswapentries(pid_t pid);

#endif