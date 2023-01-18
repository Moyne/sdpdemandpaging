## ***SDP Project c1: PAGING***

For this project I tried to implement a fully new virtual memory system, so I started out by working on the TLB module, working on the `vm_tlb.c` file, as requested I implemented a very simple round robin TLB entry replacement algorithm in the function
```c
vmtlb_write(pid_t pid,vaddr_t addr,paddr_t paddr,bool writePriv)
```
in this function it's also implemented the read only text area specification, in fact `writePriv` activates or not the TLB dirty bit.

Following this I started working on the on-demand page loading by initially working on a per-process page table, I was not too satisfied with the outcome so I changed everything to work for an inverted page table, to speed things up finally there is a hashing table.

An address space now is composed of 3 segments, one for the code, one for the text and one for the stack, where each time ***STACKPAGES*** are used for that segment.

Each segment is composed as follows:
```c
struct segment {
        size_t numpages;        //total amount of pages in this segment
        struct vnode* elfdata;  //pointer to the elf file
        unsigned char* loadedelf;       //this array tells us if the page has already been loaded once
        off_t elfoffset;
        vaddr_t startaddr;      //starting virtual address of segment
        bool stackseg;          //is this segment a stack one
        char permissions;
};
```
The loadedelf array is useful to keep track of pages that have been already loaded from the elf file or simply just loaded once for stack pages.

The paging functions and implementations are all in `pt.h` and `pt.c` ,they work as a library for `coremap.h` and `coremap.c`.

The page table is an array of pages, while the hashed table is a set of pointers to the real pages with the following structure:
```c
struct ptpage {
    pid_t pid;      //pid and vaddr are used as IDs for a single page when a page
    vaddr_t vaddr;  //is free it has pid -1 and if the page is owned by the kernel it has pid 0
    unsigned int numpages;  //field useful only for kernel pages because for user it's always 1
    struct ptpage* fifonext;    //pointer to the next element in the fifo queue
    struct ptpage* next;    //pointer for the next element that has the same hash as this one
};
```
So each page is identified by its pid and virtual address, for kernel pages the pid is set to 1, in this structure there are to pointers that makes the hashed inverted page table algorithm works, next stands for the next entry in the page table that had the same hash as this page, this is important to manage collisions in the hashed page table.

The other pointer fifonext is used to manage the queue of page replacement victim policy, as the name suggest I opted out for a fifo algorithm, initially I wanted to work with an LRU algorithm using the modify bit but at the end I opted out for a simpler alternative, the fifonext so takes care of ordering when freeing a page or allocating a new one, to completely manage the queue two other pointers are needed:
```c
struct ptpage* lastallocateduser=NULL;
struct ptpage* firstvictim=NULL;
```
These pointer are passed to the library functions and are updated each time it's needed.

The most important library functions are
```c
int setptentry(struct ptpage* pagetable,struct ptpage** hashedpt,unsigned int hashedptsize,unsigned int index,unsigned int numpages,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr)
```
to set up a new entry in the page table, `lastallocateduser` and `firstvictim` are passed as a pointer of pointer of ptpage to make them updatable in the function, `hashedptsize` is used to calculate the hash to access the rigth entry in the `struct ptpage** hashedpt` hashed table, `index` instead is the index where the page will be setted at, calculated before by the
```c
int getfreeframes(struct ptpage* pagetable,unsigned int ptsize,unsigned int numpages)
```
function.
Lastly to free up user pages
```c
int freeuser(struct ptpage** hashedpt,unsigned int hashedptsize,struct ptpage** firstvictim,struct ptpage** lastallocateduser,pid_t pid,vaddr_t vaddr)
```

Important functions in the *coremap* module are
```c
paddr_t allocuserpage(pid_t pid,vaddr_t addr);
```
to allocate a single user page identified by its pid and virtual address, and
```c
int freeuserpage(pid_t pid,vaddr_t addr);
```
to free a single user page identified by its pid and virtual address, and finally
```c
void removeptentries(pid_t pid);
```
to free up every page owned by a certain process, this is called during `as_destroy`.


To implement page replacement obviously we need a swapfile, logically the swapfile is an array of pages so to manage it I developed all of its features into `swapfile.c` and `swapfile.h` ,similarly to the pages also swapfile pages are identified by the pid and virtual address of the page.
```c
struct swappage{
    pid_t pid;      //pid and vaddr are used as IDs to identify a single page
    vaddr_t addr;
};
```
The main functions for the swapfile are
```c
int swapinpage(pid_t pid,vaddr_t addr,paddr_t readfrom)
```
used to insert a page into the swapfile, and
```c
int swapoutpage(pid_t pid,vaddr_t addr,paddr_t readin)
```
used to read from the swapfile into physical memory a page, and lastly 
```c
void removeswapentries(pid_t pid)
```
to remove every page from the swapfile that is possessed by a certain pid, this is used when `as_destroy` is called to free up resources.

### ***Conclusion***

In conclusion this project was really helpful to understand more in detail how the virtual memory works and its challenges like memory constraints, speed constraints and others.

### ***Tests***

To finish off I built the tlbreplace test because I noticed that because of the huge amount of syscalls there were too many context switches, so the tlb was invalidated too many times without ever letting it arrive to fill, I changed how this works by checking the pid of the user deactivating the tlb with the last user accessing it, if they match we don't deactivate the tlb, and also, because of the fact that each time a page is saved onto the swapfile its eventual tlb entry must be invalidated, so tlb faults with free in this project are then everytime the tlb should have at least one free slot, even if the tlb entry is put onto the same of a just swapped entry which tlb entry was deleted, while tlb faults with replace happen every time we fill the whole tlb, or the tlb is filled and I put the entry on a swapped in page.

To sum it up here are the results:

|Test name\Data(now vs then)| TLB Faults | TLB Faults with free | TLB faults with replace | TLB invalidations |  Tlb reloads | Page faults (Zeroed) | Page faults (Disk) | Page faults from ELF | Page faults from swapfile | Swapfile writes |
| ------ | ------ | ------- | ------ | ------ | ----- | ------ | ------- | ----- | ------- | ------ |
| Palin  | 5 vs 13917 | 5 vs 13917 | 0 vs 0 | 6 vs 7792 | 0 vs 13912 | 1 vs 1 | 4 vs 4 | 4 vs 4 | 0 vs 0 | 0 vs 0 |
| Huge   | 3597 vs 7538 | 3597 vs 7538 | 0 vs 0 | 6 vs 7307 | 0 vs 3941 | 1 vs 1 | 3596 vs 3596 | 514 vs 514 | 3082 vs 3082 | 3534 vs 3534 |
| Matmult | 821 vs 4902 | 64 vs 4902 | 757 vs 0 | 6 vs 1647 | 0 vs 4081 | 1 vs 1 | 820 vs 820 | 382 vs 382 | 438 vs 438 | 757 vs 757 |
| Ctest | 125246 vs 248943 | 64 vs 248943 | 125182 vs 0 | 6 vs 250465 | 0 vs 123697 | 1 vs 1 | 125245 vs 125245 | 259 vs 259 | 124986 vs 124986 | 125182 vs 125182 |
| tlbreplace | 651 vs 1291 | 64 vs 1291 | 587 vs 0 | 6 vs 1244 | 0 vs 640 | 0 vs 0 | 651 vs 651 | 321 vs 321 | 330 vs 330 | 587 vs 587 |

### ***Possible imporvements***

A possible improvement would be using an LRU algorithm for page replacement in some way, in OS161 the memory accesses data are not so accessible.
Another thing would be making the system resiliant to address spaces with segments not page aligned, in fact now because of how structured the sort program is it could not run.