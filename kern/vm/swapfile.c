#include <swapfile.h>
#include <uio.h>
#include <vnode.h>
#include <spinlock.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <vm.h>

struct vnode* swapfile;
unsigned char swappagesmap[SWAPPAGES];
struct spinlock swapspin=SPINLOCK_INITIALIZER;

void getswapspin(void){
    if(spinlock_do_i_hold(&swapspin)) return;
    else spinlock_acquire(&swapspin);
    KASSERT(spinlock_do_i_hold(&swapspin));
}

void releaseswapspin(void){
    KASSERT(spinlock_do_i_hold(&swapspin));
    spinlock_release(&swapspin);
    KASSERT(!spinlock_do_i_hold(&swapspin));
}


void swapinit(void){
    char path[]="emu0:/SWAPFILE";
    int res=vfs_open(path,O_CREAT | O_RDWR | O_TRUNC,0,&swapfile);
    if(res) panic("Could not open swapfile");
    KASSERT(swapfile!=NULL);
    getswapspin();
    for(int i=0;i<SWAPPAGES;i++) swappagesmap[i]=0;
    releaseswapspin();
}

off_t swapinpage(paddr_t readfrom){
    int ind=-1;
    getswapspin();
    for(int i=0;i<SWAPPAGES;i++){
        if(swappagesmap[i]==0){
            ind=i;
            swappagesmap[i]=1;
            break;
        }
    }
    releaseswapspin();
    if(ind==-1) panic("Swapfile has reached the maximum memory allowed");
    struct iovec iov;
    struct uio io;
    off_t offset=ind*PAGE_SIZE;
    uio_kinit(&iov,&io,(void*) PADDR_TO_KVADDR(readfrom),PAGE_SIZE,offset,UIO_WRITE);
    int res=VOP_WRITE(swapfile,&io);
    if(res) panic("Write in swapfile didnt complete");
    return offset;
}

void swapoutpage(off_t offset,paddr_t readin){
    int ind=offset/PAGE_SIZE;
    struct iovec iov;
    struct uio io;
    uio_kinit(&iov,&io,(void*) PADDR_TO_KVADDR(readin),PAGE_SIZE,offset,UIO_READ);
    int res=VOP_READ(swapfile,&io);
    if(res) panic("Error while swapping a page out");
    getswapspin();
    swappagesmap[ind]=0;
    releaseswapspin();
}

void swapdest(void){
    vfs_close(swapfile);
    spinlock_cleanup(&swapspin);
}

void copytofromswap(paddr_t to,off_t from){
    struct iovec iov;
    struct uio io;
    uio_kinit(&iov,&io,(void*) PADDR_TO_KVADDR(to),PAGE_SIZE,from,UIO_READ);
    int res=VOP_READ(swapfile,&io);
    if(res) panic("Error while copying a page from swapfile");
}