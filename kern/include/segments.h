#include <types.h>
#define RDFLAG 4
#define WRFLAG 2
#define EXFLAG 1
struct segment {
        size_t numpages;
        struct vnode* elfdata;
        off_t elfoffset;
        vaddr_t startaddr;
        bool stackseg;
        char permissions;
};

struct segment* segcreate(void);
void segdef(struct segment* seg,vaddr_t vaddr,size_t npages,
			off_t offset,struct vnode* elfnode,bool stack,int readable,int writable,int executable);
struct segment* segcopy(struct segment* seg);
void segdes(struct segment* seg);