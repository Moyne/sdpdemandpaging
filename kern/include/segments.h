#include "opt-paging.h"
#if OPT_PAGING

#include <types.h>
#define RDFLAG 4
#define WRFLAG 2
#define EXFLAG 1
struct segment {
        size_t numpages;        //total amount of pages in this segment
        struct vnode* elfdata;  //pointer to the elf file
        unsigned char* loadedelf;       //this array tells us if the page has already been loaded once
        off_t elfoffset;
        vaddr_t startaddr;      //starting virtual address of segment
        bool stackseg;          //is this segment a stack one
        char permissions;
};
//create and define new segment
struct segment* segcreate(void);
void segdef(struct segment* seg,vaddr_t vaddr,size_t npages,
			off_t offset,struct vnode* elfnode,bool stack,int readable,int writable,int executable);
//copy two segments
struct segment* segcopy(struct segment* seg);
//destroy the segment freeing resources
void segdes(struct segment* seg);

#endif