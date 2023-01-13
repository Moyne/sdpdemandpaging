#include <types.h>
#define VMSTATSLEN 10
typedef enum{
    TLBFAULT,
    TLFAULTWITHFREE,
    TLBFAULTWITHREPLACE,
    TLBINVALIDATION,
    TLBRELOAD,
    PAGEFAULTZERO,
    PAGEFAULTDISK,
    PAGEFAULTELF,
    PAGEFAULTSWAP,
    SWAPWRITE
}vmstats;
void vmstatsetup(void);

void vmstatincr(unsigned int type);

int printvmstats(int nargs, char **args);