#include <types.h>
#include <pt.h>
#define RDFLAG 1
#define WRFLAG 2
#define EXFLAG 4
struct segment {
        size_t numpages;
        struct vnode* elfdata;
        size_t filesize;
        size_t memsize;
        off_t offset;
        vaddr_t vaddr;
        struct pagetable* pagetable;
        char permissions;
};

struct segment* segcreate(void);
void segdef(struct segment* seg,vaddr_t vaddr,size_t npages,size_t filesize,size_t memsize,
			off_t offset,struct vnode* elfnode,int readable,int writable,int executable);
struct segment* segcopy(struct segment* seg);
void segdes(struct segment* seg);
struct ptpage* seggetpageat(struct segment* seg, vaddr_t vaddr);