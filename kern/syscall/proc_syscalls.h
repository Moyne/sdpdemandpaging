#include <types.h>
#include <kern/unistd.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <synch.h>
void sys__exit(int status);
