#include "opt-paging.h"
#if OPT_PAGING

#include <types.h>
#define VMSTATSLEN 10
//types of stats
typedef enum{
    TLBFAULT,
    TLBFAULTWITHFREE,
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
//update the counter of a given type
void vmstatincr(unsigned int type);
//print statistics
int printvmstats(int nargs, char **args);

#endif