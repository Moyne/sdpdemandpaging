#include <types.h>
#define SWAPFILESIZE 13*1024
#define SWAPPAGES SWAPFILESIZE/PAGE_SIZE

void getswapspin(void);
void releaseswapspin(void);
void swapinit(void);
off_t swapinpage(paddr_t readfrom);
void swapoutpage(off_t offset,paddr_t readin);
void swapdest(void);
void copytofromswap(paddr_t to,off_t from);