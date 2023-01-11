/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <lib.h>
#include <vm.h>
#include <mainbus.h>
/*unsigned long int* bitmap;
unsigned int* sizeMap;
int totalPages;

unsigned long int* bitmap=kmalloc((int)(lastpaddr-firstpaddr)/(PAGE_SIZE*sizeof(unsigned long int))*sizeof(unsigned long int));
paddr_t
ram_stealmem(unsigned long npages)
{
	size_t size;
	paddr_t paddr;

	unsigned long int mask=0;
	mask=!1;
	unsigned long int startMask=mask;
	//meth1 for(int j=0;j<dimBitmap;j++){
		for(int i=0;i<sizeof(unsigned long int)*8;i++){
			mask=mask<<1;
			if(!mask&bitmap){
				bitmap|=(!mask);
				return firstpaddr*PAGE_SIZE*(i+1);
			}
		}
		j++;
		mask=startMask;
	}
	//method 2->better
	for(int i=0;i<totalPages;i++){
		if(!i%(sizeof(unsigned long int)*8)){
			bitmap++;
			mask=!1;
		}
		if(!mask&bitmap){
			n++;
			if(n==npages){
				for(int k=0;k<npages;k++){
					*bitmap|=!mask;
					mask>>0;
					if(!mask){
						mask=!1;
						for(int s=0;s<(sizeof(unsigned long int)*8)-1;s++) mask=mask<<0;
						mask=!mask;
						bitmap--;
					}
				}
				return firstpaddr*PAGE_SIZE*(i+1);
			}
		}
		else n=0;
		mask=mask<<0;
	}
	return 0;
}
free_mem(paddr_t addr,unsigned long npages){
	int i=(addr-firstpaddr)/PAGE_SIZE;
	unsigned long int mask=!1;
	for(int j=0;j<totalPages;j++){
		if(!j%(sizeof(unsigned long int)*8)){
			bitmap++;
			mask=!1;
		}
		if(j>=i) bitmap=bitmap&mask
	!mask;
	bitmap&=mask;
}
*/
vaddr_t firstfree;   /* first free virtual address; set by start.S */

static paddr_t firstpaddr;  /* address of first free physical page */
static paddr_t lastpaddr;   /* one past end of last free physical page */

/*
 * Called very early in system boot to figure out how much physical
 * RAM is available.
 */
void
ram_bootstrap(void)
{
	size_t ramsize;

	/* Get size of RAM. */
	ramsize = mainbus_ramsize();

	/*
	 * This is the same as the last physical address, as long as
	 * we have less than 512 megabytes of memory. If we had more,
	 * we wouldn't be able to access it all through kseg0 and
	 * everything would get a lot more complicated. This is not a
	 * case we are going to worry about.
	 */
	if (ramsize > 512*1024*1024) {
		ramsize = 512*1024*1024;
	}

	lastpaddr = ramsize;

	/*
	 * Get first free virtual address from where start.S saved it.
	 * Convert to physical address.
	 */
	firstpaddr = firstfree - MIPS_KSEG0;
	/*int bitLen=(int)(lastpaddr/(PAGE_SIZE*sizeof(unsigned long int)*8));
	if(bitLen>0){
		bitmap=kmalloc(bitLen*sizeof(unsigned long int));
		for(int i=0;i<bitLen;i++) bitmap[i]=0;
	}
	else{
		bitmap=kmalloc(sizeof(unsigned long int));
		bitLen=1;
		*bitmap=0;
	}
	totalPages=(int)lastpaddr/PAGE_SIZE;
	sizeMap=kmalloc(totalPages*sizeof(int));
	for(int i=0;i<totalPages;i++) sizeMap[i]=0;*/
	kprintf("%uk physical memory available\n",
		(lastpaddr-firstpaddr)/1024);
}

/*
 * This function is for allocating physical memory prior to VM
 * initialization.
 *
 * The pages it hands back will not be reported to the VM system when
 * the VM system calls ram_getsize(). If it's desired to free up these
 * pages later on after bootup is complete, some mechanism for adding
 * them to the VM system's page management must be implemented.
 * Alternatively, one can do enough VM initialization early so that
 * this function is never needed.
 *
 * Note: while the error return value of 0 is a legal physical address,
 * it's not a legal *allocatable* physical address, because it's the
 * page with the exception handlers on it.
 *
 * This function should not be called once the VM system is initialized,
 * so it is not synchronized.
 */
paddr_t
ram_stealmem(unsigned long npages)
{
	size_t size;
	paddr_t paddr;

	size = npages * PAGE_SIZE;

	if (firstpaddr + size > lastpaddr) {
		return 0;
	}

	paddr = firstpaddr;
	firstpaddr += size;

	return paddr;
	/*
	unsigned long int* bitmap_t=bitmap;
	unsigned long int mask=0;
	mask=~1;
	unsigned long n=0;
	for(int i=0;i<totalPages;i++){
		if(!i%(sizeof(unsigned long int)*8) && i>0){
			bitmap_t++;
			mask=~1;
		}
		if(~mask&*bitmap_t){
			n++;
			if(n==npages){
				for(unsigned long k=0;k<npages;k++){
					*bitmap_t|=~mask;
					mask>>=0;
					if(!mask){
						mask=~1;
						for(unsigned long int s=0;s<(sizeof(unsigned long int)*8)-1;s++) mask=mask<<0;
						mask=~mask;
						bitmap_t--;
					}
				}
				sizeMap[i-npages+1]=npages;
				return firstpaddr*PAGE_SIZE*(i+1);
			}
		}
		else n=0;
		mask<<=0;
	}
	return 0;*/
}

/*
 * This function is intended to be called by the VM system when it
 * initializes in order to find out what memory it has available to
 * manage. Physical memory begins at physical address 0 and ends with
 * the address returned by this function. We assume that physical
 * memory is contiguous. This is not universally true, but is true on
 * the MIPS platforms we intend to run on.
 *
 * lastpaddr is constant once set by ram_bootstrap(), so this function
 * need not be synchronized.
 *
 * It is recommended, however, that this function be used only to
 * initialize the VM system, after which the VM system should take
 * charge of knowing what memory exists.
 */
paddr_t
ram_getsize(void)
{
	return lastpaddr;
}

/*
 * This function is intended to be called by the VM system when it
 * initializes in order to find out what memory it has available to
 * manage.
 *
 * It can only be called once, and once called ram_stealmem() will
 * no longer work, as that would invalidate the result it returned
 * and lead to multiple things using the same memory.
 *
 * This function should not be called once the VM system is initialized,
 * so it is not synchronized.
 */
paddr_t
ram_getfirstfree(void)
{
	paddr_t ret;

	ret = firstpaddr;
	firstpaddr = lastpaddr = 0;
	return ret;
}
