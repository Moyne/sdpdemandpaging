#include <types.h>
#include <kern/unistd.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
int sys_write(int fd,userptr_t data,size_t len);
int sys_read(int fd,userptr_t data,size_t len);
